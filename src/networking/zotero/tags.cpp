/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "tags.h"

#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QUrl>
#include <QTimer>

#include "internalnetworkaccessmanager.h"
#include "api.h"
#include "logging_networking.h"

using namespace Zotero;

class Zotero::Tags::Private
{
private:
    Zotero::Tags *p;

public:
    QSharedPointer<Zotero::API> api;

    Private(QSharedPointer<Zotero::API> a, Zotero::Tags *parent)
            : p(parent), api(a) {
        initialized = false;
        busy = false;
    }

    bool initialized, busy;

    QMap<QString, int> tags;

    QNetworkReply *requestZoteroUrl(const QUrl &url) {
        busy = true;
        QUrl internalUrl = url;
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        connect(reply, &QNetworkReply::finished, p, &Zotero::Tags::finishedFetchingTags);
        return reply;
    }
};

Tags::Tags(QSharedPointer<Zotero::API> api, QObject *parent)
        : QObject(parent), d(new Zotero::Tags::Private(api, this))
{
    QUrl url = api->baseUrl();
    url = url.adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + QStringLiteral("/tags"));

    if (api->inBackoffMode())
        /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
        QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, this, [ = ]() {
            d->requestZoteroUrl(url);
        });
    else
        d->requestZoteroUrl(url);
}

Tags::~Tags()
{
    delete d;
}

bool Tags::initialized() const
{
    return d->initialized;
}

bool Tags::busy() const
{
    return d->busy;
}

QMap<QString, int> Tags::tags() const
{
    return d->tags;
}

void Tags::finishedFetchingTags()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

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

    if (reply->error() == QNetworkReply::NoError) {
        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("entry")) {
                QString tag;
                int count = -1;
                while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                    const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                    if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("title"))
                        tag = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("numItems")) {
                        bool ok = false;
                        count = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements).toInt(&ok);
                        if (count < 1) count = -1;
                    } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QStringLiteral("entry"))
                        break;
                }

                if (!tag.isEmpty() && count > 0)
                    d->tags.insert(tag, count);
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("link")) {
                const QXmlStreamAttributes attrs = xmlReader.attributes();
                if (attrs.hasAttribute(QStringLiteral("rel")) && attrs.hasAttribute(QStringLiteral("href")) && attrs.value(QStringLiteral("rel")) == QStringLiteral("next"))
                    nextPage = attrs.value(QStringLiteral("href")).toString();
            } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QStringLiteral("feed"))
                break;
        }

        if (!nextPage.isEmpty()) {
            if (d->api->inBackoffMode())
                /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
                QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, this, [ = ]() {
                     d->requestZoteroUrl(nextPage);
                });
            else
                d->requestZoteroUrl(nextPage);
        } else {
            d->busy = false;
            d->initialized = true;
            emit finishedLoading();
        }
    } else {
        qCWarning(LOG_KBIBTEX_NETWORKING) << reply->errorString(); ///< something went wrong
        d->busy = false;
        d->initialized = false;
        emit finishedLoading();
    }
}
