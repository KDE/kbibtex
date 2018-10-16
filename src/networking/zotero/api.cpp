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

#include "api.h"

#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QTimer>

using namespace Zotero;

class Zotero::API::Private
{
public:
    const QUrl apiBaseUrl;
    const int userOrGroupPrefix;
    const QString apiKey;

    QDateTime backoffElapseTime;

    Private(RequestScope requestScope, int prefix, const QString &_apiKey, Zotero::API *parent)
            : apiBaseUrl(QUrl(QString(QStringLiteral("https://api.zotero.org/%1/%2")).arg(requestScope == GroupRequest ? QStringLiteral("groups") : QStringLiteral("users")).arg(prefix))),
          userOrGroupPrefix(prefix),
          apiKey(_apiKey), backoffElapseTime(QDateTime::currentDateTime().addSecs(-5)) {
        Q_UNUSED(parent)
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

void API::addLimitToUrl(QUrl &url) const
{
    static const QString limitKey = QStringLiteral("limit");
    QUrlQuery query(url);
    query.removeQueryItem(limitKey);
    query.addQueryItem(limitKey, QString::number(Zotero::API::limit));
    url.setQuery(query);
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
    request.setRawHeader("Zotero-API-Version", "3");
    request.setRawHeader("Accept", "application/atom+xml");
    request.setRawHeader("Authorization", QString(QStringLiteral("Bearer ")).append(d->apiKey).toLatin1().constData());
    return request;
}

void API::startBackoff(int duration) {
    if (duration > 0 && !inBackoffMode()) {
        d->backoffElapseTime = QDateTime::currentDateTime().addSecs(duration + 1);
        emit backoffModeStart();
        /// Use single-shot timer and functor to emit signal
        /// that backoff mode has finished
        QTimer::singleShot((duration + 1) * 1000, this, [ = ]() {
            emit backoffModeEnd();
        });
    }
}

bool API::inBackoffMode() const {
    return d->backoffElapseTime >= QDateTime::currentDateTime();
}

qint64 API::backoffSecondsLeft() const {
    const qint64 diff = QDateTime::currentDateTime().secsTo(d->backoffElapseTime);
    if (diff < 0)
        return 0;
    else
        return diff;
}
