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

#include "groups.h"

#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QUrl>

#include "internalnetworkaccessmanager.h"
#include "api.h"
#include "logging_networking.h"

using namespace Zotero;

class Zotero::Groups::Private
{
private:
    Zotero::Groups *p;

public:
    API *api;

    Private(API *a, Zotero::Groups *parent)
            : p(parent), api(a) {
        initialized = false;
        busy = false;
    }

    bool initialized, busy;

    QMap<int, QString> groups;

    QNetworkReply *requestZoteroUrl(const QUrl &url) {
        busy = true;
        QUrl internalUrl = url;
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingGroups()));
        return reply;
    }
};

Groups::Groups(API *api, QObject *parent)
        : QObject(parent), d(new Zotero::Groups::Private(api, this))
{
    QUrl url = api->baseUrl();
    Q_ASSERT_X(url.path().contains(QLatin1String("users/")), "Groups::Groups(API *api, QObject *parent)", "Provided base URL does not contain 'users/' as expected");
    url = url.adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + '/' + (QLatin1String("/groups")));
    d->requestZoteroUrl(url);
}

Groups::~Groups()
{
    delete d;
}

bool Groups::initialized() const
{
    return d->initialized;
}

bool Groups::busy() const
{
    return d->busy;
}

QMap<int, QString> Groups::groups() const
{
    return d->groups;
}

void Groups::finishedFetchingGroups()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("entry")) {
                QString label;
                int groupId = -1;
                while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                    const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                    if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("title"))
                        label = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("groupID")) {
                        bool ok = false;
                        groupId = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements).toInt(&ok);
                        if (groupId < 1) groupId = -1;
                    } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("entry"))
                        break;
                }

                if (!label.isEmpty() && groupId > 0)
                    d->groups.insert(groupId, label);
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
        qCWarning(LOG_KBIBTEX_NETWORKING) << reply->errorString(); ///< something went wrong
        d->busy = false;
        d->initialized = false;
        emit finishedLoading();
    }
}
