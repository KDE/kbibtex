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

#ifndef KBIBTEX_NETWORKING_INTERNALNETWORKACCESSMANAGER_H
#define KBIBTEX_NETWORKING_INTERNALNETWORKACCESSMANAGER_H

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

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
    static InternalNetworkAccessManager &instance();

    QNetworkReply *get(QNetworkRequest &request, const QUrl &oldUrl);
    QNetworkReply *get(QNetworkRequest &request, const QNetworkReply *oldReply = nullptr);

    void mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url);

    void setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec = 30);

    /**
     * Reverse the obfuscation of an API key. Given a byte
     * array holding the obfuscated API key, restore and
     * return the original API key.
     *
     * @param obfuscated obfuscated API key
     * @return restored original API key if succeeded, empty on error
     */
    static QString reverseObfuscate(const QByteArray &obfuscated);

    /**
     * Remove API keys from an URL.
     *
     * @param url URL where API keys have to be removed
     * @return a reference to the URL passed to this function
     */
    static QUrl removeApiKey(QUrl url);

protected:
    InternalNetworkAccessManager(QObject *parent = nullptr);
    class HTTPEquivCookieJar;
    HTTPEquivCookieJar *cookieJar;

private:
    QMap<QTimer *, QNetworkReply *> m_mapTimerToReply;

    static QString userAgentString;

    static QString userAgent();

private slots:
    void networkReplyTimeout();
    void networkReplyFinished();
    void logSslErrors(const QList<QSslError> &errors);
};

#endif // KBIBTEX_NETWORKING_INTERNALNETWORKACCESSMANAGER_H
