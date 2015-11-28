/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
    API *api;
    KUrl queuedRequestZoteroUrl;

    Private(API *a, Zotero::Items *parent)
            : p(parent), api(a) {
        /// nothing
    }

    QNetworkReply *requestZoteroUrl(const KUrl &url) {
        KUrl internalUrl = url;
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

        requestZoteroUrl(internalUrl);
    }
};

Items::Items(API *api, QObject *parent)
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

    if (reply->hasRawHeader("Backoff"))
        d->api->startBackoff(QString::fromLatin1(reply->rawHeader("Backoff").constData()).toInt());
    else if (reply->hasRawHeader("Retry-After"))
        d->api->startBackoff(QString::fromLatin1(reply->rawHeader("Retry-After").constData()).toInt());

    if (reply->error() == QNetworkReply::NoError && ok) {
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().data());
        /// Non-empty result?
        if (!bibTeXcode.isEmpty()) {
            static FileImporterBibTeX importer;
            /// Parse text into bibliography object
            File *bibtexFile = importer.fromString(bibTeXcode);

            /// Perform basic sanity checks ...
            if (bibtexFile != NULL && !bibtexFile->isEmpty()) {
                foreach(const QSharedPointer<Element> &element, *bibtexFile) {
                    emit foundElement(element); ///< ... and publish result
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
