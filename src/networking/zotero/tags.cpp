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

#include "tags.h"

#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QDebug>
#include <QUrl>
#include <QTimer>

#include "internalnetworkaccessmanager.h"
#include "api.h"

using namespace Zotero;

class Zotero::Tags::Private
{
private:
    Zotero::Tags *p;

public:
    API *api;

    Private(API *a, Zotero::Tags *parent)
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
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingTags()));
        return reply;
    }
};

Tags::Tags(API *api, QObject *parent)
        : QObject(parent), d(new Zotero::Tags::Private(api, this))
{
    QUrl url = api->baseUrl();
    url = url.adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + QLatin1String("/tags"));

    if (api->inBackoffMode())
        QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, [ = ]() {
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

    if (reply->hasRawHeader("Backoff"))
        d->api->startBackoff(QString::fromAscii(reply->rawHeader("Backoff").constData()).toInt());
    else if (reply->hasRawHeader("Retry-After"))
        d->api->startBackoff(QString::fromAscii(reply->rawHeader("Retry-After").constData()).toInt());

    if (reply->error() == QNetworkReply::NoError) {
        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("entry")) {
                QString tag;
                int count = -1;
                while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                    const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                    if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("title"))
                        tag = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("numItems")) {
                        bool ok = false;
                        count = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements).toInt(&ok);
                        if (count < 1) count = -1;
                    } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("entry"))
                        break;
                }

                if (!tag.isEmpty() && count > 0)
                    d->tags.insert(tag, count);
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("link")) {
                const QXmlStreamAttributes attrs = xmlReader.attributes();
                if (attrs.hasAttribute(QLatin1String("rel")) && attrs.hasAttribute(QLatin1String("href")) && attrs.value(QLatin1String("rel")) == QLatin1String("next"))
                    nextPage = attrs.value(QLatin1String("href")).toString();
            } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("feed"))
                break;
        }

        if (!nextPage.isEmpty())
            d->requestZoteroUrl(nextPage);
        else {
            d->busy = false;
            d->initialized = true;
            emit finishedLoading();
        }
    } else {
        qWarning() << reply->errorString(); ///< something went wrong
        d->busy = false;
        d->initialized = false;
        emit finishedLoading();
    }
}
