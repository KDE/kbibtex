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

#include "api.h"

#include <KUrl>
#include <KDebug>

using namespace Zotero;

class Zotero::API::Private
{
private:
    // UNUSED Zotero::API *p;

public:
    const KUrl apiBaseUrl;
    const QString apiKey;
    int userOrGroupPrefix;

    Private(RequestScope requestScope, int prefix, const QString &_apiKey, Zotero::API */* UNUSED parent*/)
        : // UNUSED p(parent),
        apiBaseUrl(KUrl(QString(QLatin1String("https://api.zotero.org/%1/%2%3")).arg(requestScope == GroupRequest ? QLatin1String("groups") : QLatin1String("users")).arg(prefix).arg(_apiKey.isEmpty() ? QString() : QString(QLatin1String("?key=%1")).arg(_apiKey)))),
        apiKey(_apiKey),
        userOrGroupPrefix(prefix) {
        /// nothing
    }
};

const int Zotero::API::limit = 45;

API::API(RequestScope requestScope, int userOrGroupPrefix, const QString &apiKey, QObject *parent)
        : QObject(parent), d(new API::Private(requestScope, userOrGroupPrefix, apiKey, this))
{
    /// nothing
}

API::~API()
{
    delete d;
}

void API::addLimitToUrl(KUrl &url) const
{
    static const QString limitKey = QLatin1String("limit");
    url.removeQueryItem(limitKey);
    url.addQueryItem(limitKey, QString::number(Zotero::API::limit));
}

void API::addKeyToUrl(KUrl &url) const
{
    if (!d->apiKey.isEmpty()){
    static const QString keyKey = QLatin1String("key");
    url.removeQueryItem(keyKey);
    url.addQueryItem(keyKey, d->apiKey);
    }
}

KUrl API::baseUrl() const
{
    return d->apiBaseUrl;
}

int API::userOrGroupPrefix() const
{
    return d->userOrGroupPrefix;
}

QNetworkRequest API::request(const KUrl &url) const
{
    kDebug() << "url=" << url.pathOrUrl();
    QNetworkRequest request(url);
    request.setRawHeader("Zotero-API-Version", "2");
    return request;
}
