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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "items.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QUrlQuery>
#include <QTimer>

#include "file.h"
#include "fileimporterbibtex.h"
#include "api.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

using namespace Zotero;

class Zotero::Items::Private
{
private:
    Zotero::Items *p;

public:
    QSharedPointer<Zotero::API> api;

    Private(QSharedPointer<Zotero::API> a, Zotero::Items *parent)
            : p(parent), api(a) {
        /// nothing
    }

    QNetworkReply *requestZoteroUrl(const QUrl &url) {
        QUrl internalUrl = url;
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, &QNetworkReply::finished, p, &Zotero::Items::finishedFetchingItems);
        return reply;
    }

    void retrieveItems(const QUrl &url, int start) {
        QUrl internalUrl = url;

        static const QString queryItemStart = QStringLiteral("start");
        QUrlQuery query(internalUrl);
        query.removeQueryItem(queryItemStart);
        query.addQueryItem(queryItemStart, QString::number(start));
        internalUrl.setQuery(query);

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
    QUrl url = d->api->baseUrl().adjusted(QUrl::StripTrailingSlash);
    if (collection.isEmpty())
        url.setPath(url.path() + QStringLiteral("/items"));
    else
        url.setPath(url.path() + QString(QStringLiteral("/collections/%1/items")).arg(collection));
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("bibtex"));
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
        query.addQueryItem(QStringLiteral("tag"), tag);
    url.setPath(url.path() + QStringLiteral("/items"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("bibtex"));
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
    static const QString queryItemStart = QStringLiteral("start");
    bool ok = false;
    const int start = QUrlQuery(reply->url()).queryItemValue(queryItemStart).toInt(&ok);

    if (reply->hasRawHeader("Backoff"))
        d->api->startBackoff(QString::fromLatin1(reply->rawHeader("Backoff").constData()).toInt());
    else if (reply->hasRawHeader("Retry-After"))
        d->api->startBackoff(QString::fromLatin1(reply->rawHeader("Retry-After").constData()).toInt());

    if (reply->error() == QNetworkReply::NoError && ok) {
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());
        /// Non-empty result?
        if (!bibTeXcode.isEmpty()) {
            static FileImporterBibTeX importer(this);
            /// Parse text into bibliography object
            File *bibtexFile = importer.fromString(bibTeXcode);

            /// Perform basic sanity checks ...
            if (bibtexFile != nullptr && !bibtexFile->isEmpty()) {
                for (const QSharedPointer<Element> &element : const_cast<const File &>(*bibtexFile)) {
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
        qCWarning(LOG_KBIBTEX_NETWORKING) << reply->errorString(); ///< something went wrong
        emit stoppedSearch(1); // TODO proper error codes
    }
}
