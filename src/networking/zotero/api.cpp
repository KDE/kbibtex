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

#include "api.h"

#include <KUrl>

using namespace Zotero;

class Zotero::API::Private
{
private:
    // UNUSED Zotero::API *p;

public:
    const QUrl apiBaseUrl;
    int userOrGroupPrefix;

    Private(RequestScope requestScope, int prefix, const QString &apiKey, Zotero::API */* UNUSED parent*/)
        : // UNUSED p(parent),
        apiBaseUrl(QUrl(QString(QLatin1String("https://api.zotero.org/%1/%2%3")).arg(requestScope == GroupRequest ? QLatin1String("groups") : QLatin1String("users")).arg(prefix).arg(apiKey.isEmpty() ? QString() : QString(QLatin1String("?key=%1")).arg(apiKey)))),
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

void API::addLimitToUrl(QUrl &url) const
{
    static const QString limitKey = QLatin1String("limit");
    url.removeQueryItem(limitKey);
    url.addQueryItem(limitKey, QString::number(Zotero::API::limit));
}

QUrl API::baseUrl() const
{
    return d->apiBaseUrl;
}

int API::userOrGroupPrefix() const
{
    return d->userOrGroupPrefix;
}

QNetworkRequest API::request(const QUrl &url) const
{
    QNetworkRequest request(url);
    request.setRawHeader("Zotero-API-Version", "2");
    return request;
}
