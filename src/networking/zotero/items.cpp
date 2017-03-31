/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "items.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QTimer>

#include <KDebug>

#include "file.h"
#include "fileimporterbibtex.h"
#include "api.h"

#include "internalnetworkaccessmanager.h"

using namespace Zotero;

class Zotero::Items::Private
{
private:
    Zotero::Items *p;

public:
    QSharedPointer<Zotero::API> api;
    KUrl queuedRequestZoteroUrl;

    Private(QSharedPointer<Zotero::API> a, Zotero::Items *parent)
            : p(parent), api(a) {
        /// nothing
    }

    QNetworkReply *requestZoteroUrl(const KUrl &url) {
        KUrl internalUrl = url;
        api->addKeyToUrl(internalUrl);
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingItems()));
        return reply;
    }

    void retrieveItems(const KUrl &url, int start) {
        KUrl internalUrl = url;

        static const QString queryItemStart = QLatin1String("start");
        internalUrl.removeQueryItem(queryItemStart);
        internalUrl.addQueryItem(queryItemStart, QString::number(start));

        if (api->inBackoffMode() && queuedRequestZoteroUrl.isEmpty()) {
            /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
            queuedRequestZoteroUrl = internalUrl;
            QTimer::singleShot((api->backoffSecondsLeft() + 1) * 1000, p, SLOT(singleShotRequestZoteroUrl()));
        } else
            requestZoteroUrl(internalUrl);
    }
};

Items::Items(QSharedPointer<Zotero::API> api, QObject *parent)
        : QObject(parent), d(new Zotero::Items::Private(api, this))
{
    /// nothing
}

Items::~Items()
{
    delete d;
}

void Items::retrieveItemsByCollection(const QString &collection)
{
    KUrl url = d->api->baseUrl();
    if (collection.isEmpty())
        url.addPath(QLatin1String("items"));
    else
        url.addPath(QString(QLatin1String("/collections/%1/items")).arg(collection));
    url.addQueryItem(QLatin1String("format"), QLatin1String("bibtex"));
    if (d->api->inBackoffMode() && d->queuedRequestZoteroUrl.isEmpty()) {
        /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
        d->queuedRequestZoteroUrl = url;
        QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, this, SLOT(singleShotRequestZoteroUrl()));
    } else
        d->retrieveItems(url, 0);
}

void  Items::retrieveItemsByTag(const QString &tag)
{
    KUrl url = d->api->baseUrl();
    if (!tag.isEmpty())
        url.addQueryItem(QLatin1String("tag"), tag);
    url.addPath(QLatin1String("items"));
    url.addQueryItem(QLatin1String("format"), QLatin1String("bibtex"));
    if (d->api->inBackoffMode() && d->queuedRequestZoteroUrl.isEmpty()) {
        /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
        d->queuedRequestZoteroUrl = url;
        QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, this, SLOT(singleShotRequestZoteroUrl()));
    } else
        d->retrieveItems(url, 0);
}

void Items::finishedFetchingItems()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    static const QString queryItemStart = QLatin1String("start");
    bool ok = false;
    const int start = reply->url().queryItemValue(queryItemStart).toInt(&ok);

    if (reply->hasRawHeader("Backoff")) {
        bool ok = false;
        int time = QString::fromLatin1(reply->rawHeader("Backoff").constData()).toInt(&ok);
        if (!ok) time = 10; ///< parsing argument of raw header 'Backoff' failed? 10 seconds is fallback
        d->api->startBackoff(time);
    } else if (reply->hasRawHeader("Retry-After")) {
        bool ok = false;
        int time = QString::fromLatin1(reply->rawHeader("Retry-After").constData()).toInt(&ok);
        if (!ok) time = 10; ///< parsing argument of raw header 'Retry-After' failed? 10 seconds is fallback
        d->api->startBackoff(time);
    }

    if (reply->error() == QNetworkReply::NoError && ok) {
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());
        /// Non-empty result?
        if (!bibTeXcode.isEmpty()) {
            static FileImporterBibTeX importer;
            /// Parse text into bibliography object
            File *bibtexFile = importer.fromString(bibTeXcode);

            /// Perform basic sanity checks ...
            if (bibtexFile != NULL && !bibtexFile->isEmpty()) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    emit foundElement(*it); ///< ... and publish result
                }
            }

            delete bibtexFile;

            /// Non-empty result means there may be more ...
            d->retrieveItems(reply->url(), start + Zotero::API::limit);
        } else {
            /// Done retrieving BibTeX code
            emit stoppedSearch(0); // TODO proper error codes
        }
    } else {
        kWarning() << reply->errorString(); ///< something went wrong
        emit stoppedSearch(1); // TODO proper error codes
    }
}

void Items::singleShotRequestZoteroUrl() {
    d->retrieveItems(d->queuedRequestZoteroUrl, 0);
    d->queuedRequestZoteroUrl.clear();
}
