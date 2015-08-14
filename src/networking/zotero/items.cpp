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
#include <QDebug>
#include <QUrlQuery>

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

    Private(API *a, Zotero::Items *parent)
            : p(parent), api(a) {
        /// nothing
    }

    QNetworkReply *requestZoteroUrl(const QUrl &url) {
        QUrl internalUrl = url;
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingItems()));
        return reply;
    }

    void retrieveItems(const QUrl &url, int start) {
        QUrl internalUrl = url;

        static const QString queryItemStart = QLatin1String("start");
        QUrlQuery query(internalUrl);
        query.removeQueryItem(queryItemStart);
        query.addQueryItem(queryItemStart, QString::number(start));
        internalUrl.setQuery(query);

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
    QUrl url = d->api->baseUrl().adjusted(QUrl::StripTrailingSlash);
    if (collection.isEmpty())
        url.setPath(url.path() + '/' + (QLatin1String("items")));
    else
        url.setPath(url.path() + '/' + (QString(QLatin1String("/collections/%1/items")).arg(collection)));
    QUrlQuery query(url);
    query.addQueryItem(QLatin1String("format"), QLatin1String("bibtex"));
    url.setQuery(query);

    if (d->api->inBackoffMode())
        QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, [ = ]() {
            d->retrieveItems(url, 0);
        });
    else
        d->retrieveItems(url, 0);
}

void  Items::retrieveItemsByTag(const QString &tag)
{
    QUrl url = d->api->baseUrl().adjusted(QUrl::StripTrailingSlash);
    QUrlQuery query(url);
    if (!tag.isEmpty())
        query.addQueryItem(QLatin1String("tag"), tag);
    url.setPath(url.path() + '/' + (QLatin1String("items")));
    query.addQueryItem(QLatin1String("format"), QLatin1String("bibtex"));
    url.setQuery(query);

    if (d->api->inBackoffMode())
        QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, [ = ]() {
            d->retrieveItems(url, 0);
        });
    else
        d->retrieveItems(url, 0);
}

void Items::finishedFetchingItems()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    static const QString queryItemStart = QLatin1String("start");
    bool ok = false;
    const int start = QUrlQuery(reply->url()).queryItemValue(queryItemStart).toInt(&ok);

    if (reply->hasRawHeader("Backoff"))
        d->api->startBackoff(QString::fromAscii(reply->rawHeader("Backoff").constData()).toInt());
    else if (reply->hasRawHeader("Retry-After"))
        d->api->startBackoff(QString::fromAscii(reply->rawHeader("Retry-After").constData()).toInt());

    if (reply->error() == QNetworkReply::NoError && ok) {
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().data());
        /// Non-empty result?
        if (!bibTeXcode.isEmpty()) {
            static FileImporterBibTeX importer;
            /// Parse text into bibliography object
            File *bibtexFile = importer.fromString(bibTeXcode);

            /// Perform basic sanity checks ...
            if (bibtexFile != NULL && !bibtexFile->isEmpty()) {
                foreach(const QSharedPointer<Element> element, *bibtexFile) {
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
        qWarning() << reply->errorString(); ///< something went wrong
        emit stoppedSearch(1); // TODO proper error codes
    }
}
