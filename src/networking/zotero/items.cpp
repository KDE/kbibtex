/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#include <QQueue>
#include <QXmlStreamReader>

#include <KDebug>

#include "file.h"
#include "fileimporterbibtex.h"

#include "internalnetworkaccessmanager.h"

using namespace Zotero;

const int limit = 45;

class Zotero::Items::Private
{
private:
    Zotero::Items *p;

public:
    QQueue<QString> downloadQueue;

    Private(Zotero::Items *parent)
            : p(parent) {
        /// nothing
    }

    QNetworkReply *requestZoteroUrl(const KUrl &url, bool doLimit = true) {
        KUrl internalUrl = url;
        if (doLimit) {
            static const QString queryItemLimit = QLatin1String("limit");
            internalUrl.removeQueryItem(queryItemLimit);
            internalUrl.addQueryItem(queryItemLimit, QString::number(limit));
        }
        QNetworkRequest request(internalUrl);
        request.setRawHeader("Zotero-API-Version", "2");

        kDebug() << "Requesting from Zotero: " << internalUrl.pathOrUrl();

        return InternalNetworkAccessManager::self()->get(request);
    }

    void downloadNextQueuedBibItem() {
        if (!downloadQueue.isEmpty()) {
            KUrl url = KUrl(downloadQueue.dequeue());
            url.addQueryItem(QLatin1String("format"), QLatin1String("bibtex"));
            QNetworkReply *reply = requestZoteroUrl(url, false);
            connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingItem()));
        } else {
            kDebug() << "Queue is empty";
        }
    }
};

Items::Items(QObject *parent)
        : QObject(parent), d(new Zotero::Items::Private(this))
{
    /// nothing
}

void Items::retrieveItems(const KUrl &baseUrl, const QString &collection)
{
    KUrl url = baseUrl;
    url.addPath(QString(QLatin1String("/collections/%1/items")).arg(collection));
    url.addQueryItem(QLatin1String("format"), QLatin1String("atom"));
    url.addQueryItem(QLatin1String("content"), QLatin1String("none"));
    QNetworkReply *reply = d->requestZoteroUrl(url);
    connect(reply, SIGNAL(finished()), this, SLOT(finishedFetchingItemList()));
    d->downloadQueue.clear();
}

void Items::finishedFetchingItemList()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("entry")) {
                QString itemUrl;
                while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                    const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                    if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("id"))
                        /// Usually looks like  http://zotero.org/users/475425/items/PQKBRC33
                        itemUrl = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("entry"))
                        break;
                }

                if (!itemUrl.isEmpty()) {
                    /// Rewrite "id" URL to match requirements for API call
                    /// Usually looks like  https://api.zotero.org/users/475425/items/PQKBRC33
                    itemUrl.replace(QLatin1String("http://"), QLatin1String("https://"));
                    itemUrl.replace(QLatin1String("/zotero.org/"), QLatin1String("/api.zotero.org/"));
                    /// Queue bibliographic item's URL
                    d->downloadQueue.enqueue(itemUrl);
                }
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("link")) {
                const QXmlStreamAttributes attrs = xmlReader.attributes();
                if (attrs.hasAttribute(QLatin1String("rel")) && attrs.hasAttribute(QLatin1String("href"))) {
                    if (attrs.value(QLatin1String("rel")) == QLatin1String("next"))
                        nextPage = attrs.value(QLatin1String("href")).toString();
                }
            } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("feed"))
                break;
        }

        if (!nextPage.isEmpty()) {
            /// There is a next page for this query,
            /// so continue by fetching next page
            QNetworkReply *nextReply = d->requestZoteroUrl(nextPage, false);
            connect(nextReply, SIGNAL(finished()), this, SLOT(finishedFetchingItemList()));
        } else {
            /// No next page left, therefore start fetching
            /// individual bibliographic items
            d->downloadNextQueuedBibItem();
        }
    } else
        kWarning() << reply->errorString(); ///< something went wrong
}

void Items::finishedFetchingItem()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        /// Redirection? Continue by loading redirection target,
        /// activating exactly this slot once it is completed
        KUrl newUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        QNetworkReply *newReply = d->requestZoteroUrl(newUrl, false);
        connect(newReply, SIGNAL(finished()), this, SLOT(finishedFetchingItem()));
    } else if (reply->error() == QNetworkReply::NoError) {
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().data());
        /// Non-empty result?
        if (!bibTeXcode.isEmpty()) {
            static FileImporterBibTeX importer;
            /// Parse text into bibliography object
            File *bibtexFile = importer.fromString(bibTeXcode);

            /// Perform basic sanity checks ...
            if (bibtexFile != NULL && bibtexFile->count() == 1 && !bibtexFile->first().isNull())
                emit foundElement(bibtexFile->first()); ///< ... and publish result

            delete bibtexFile;

        }

        /// Continue fetching next individual bibliographic item
        d->downloadNextQueuedBibItem();
    } else
        kWarning() << reply->errorString(); ///< something went wrong
}
