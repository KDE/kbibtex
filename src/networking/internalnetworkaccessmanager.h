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

#ifndef KBIBTEX_NETWORKING_INTERNALNAM_H
#define KBIBTEX_NETWORKING_INTERNALNAM_H

#include "kbibtexnetworking_export.h"

#include <QNetworkAccessManager>
#include <QUrl>
#include <QMap>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class HTTPEquivCookieJar;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT InternalNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    static InternalNetworkAccessManager *self();
    QNetworkReply *get(QNetworkRequest &request, const QUrl &oldUrl);
    QNetworkReply *get(QNetworkRequest &request, const QNetworkReply *oldReply = NULL);

    void mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url);

    void setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec = 30);

protected:
    InternalNetworkAccessManager(QObject *parent = NULL);
    class HTTPEquivCookieJar;
    HTTPEquivCookieJar *cookieJar;

private:
    QMap<QTimer *, QNetworkReply *> m_mapTimerToReply;

    static InternalNetworkAccessManager *instance;
    static QString userAgentString;

    static QString userAgent();

private slots:
    void networkReplyTimeout();
    void networkReplyFinished();

};

#endif // KBIBTEX_NETWORKING_INTERNALNAM_H
