/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <ctime>

#include <QStringList>
#include <QRegExp>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtGlobal>
#include <QNetworkProxy>

#include <KUrl>
#include <KApplication>
#include <KProtocolManager>

#include "internalnetworkaccessmanager.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class InternalNetworkAccessManager::HTTPEquivCookieJar: public QNetworkCookieJar
{
public:
    void mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url) {
        static QRegExp cookieContent("^([^\"=; ]+)=([^\"=; ]+).*\\bpath=([^\"=; ]+)", Qt::CaseInsensitive);
        int p1 = -1;
        if ((p1 = htmlCode.toLower().indexOf("http-equiv=\"set-cookie\"", 0, Qt::CaseInsensitive)) >= 5
                && (p1 = htmlCode.lastIndexOf("<meta", p1, Qt::CaseInsensitive)) >= 0
                && (p1 = htmlCode.indexOf("content=\"", p1, Qt::CaseInsensitive)) >= 0
                && cookieContent.indexIn(htmlCode.mid(p1 + 9, 256)) >= 0) {
            const QString key = cookieContent.cap(1);
            const QString value = cookieContent.cap(2);
            const QString path = cookieContent.cap(3);
            QUrl cookieUrl = url;
            QList<QNetworkCookie> cookies = cookiesForUrl(cookieUrl);
            cookies.append(QNetworkCookie(key.toAscii(), value.toAscii()));
            setCookiesFromUrl(cookies, cookieUrl);
        }
    }

    HTTPEquivCookieJar(QObject *parent = NULL)
            : QNetworkCookieJar(parent) {
        // nothing
    }
};


/// various browser strings to "disguise" origin
const QStringList userAgentList = QStringList()
                                  << QLatin1String("Mozilla/5.0 (Linux; U; Android 2.3.3; en-us; HTC_DesireS_S510e Build/GRI40) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1")
                                  << QLatin1String("Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.2.3) Gecko/20100402 Prism/1.0b4")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; Win 9x 4.90; SG; rv:1.9.2.4) Gecko/20101104 Netscape/9.1.0285")
                                  << QLatin1String("Mozilla/5.0 (compatible; Konqueror/4.5; FreeBSD) KHTML/4.5.4 (like Gecko)")
                                  << QLatin1String("Mozilla/5.0 (compatible; Yahoo! Slurp China; http://misc.yahoo.com.cn/help.html)")
                                  << QLatin1String("yacybot (x86 Windows XP 5.1; java 1.6.0_12; Europe/de) http://yacy.net/bot.html")
                                  << QLatin1String("Nokia6230i/2.0 (03.25) Profile/MIDP-2.0 Configuration/CLDC-1.1")
                                  << QLatin1String("Links (2.3-pre1; NetBSD 5.0 i386; 96x36)")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/523.15 (KHTML, like Gecko, Safari/419.3) Arora/0.3 (Change: 287 c9dfb30)")
                                  << QLatin1String("Mozilla/4.0 (compatible; Dillo 2.2)")
                                  << QLatin1String("Emacs-W3/4.0pre.46 URL/p4.0pre.46 (i686-pc-linux; X11)")
                                  << QLatin1String("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.13) Gecko/20080208 Galeon/2.0.4 (2008.1) Firefox/2.0.0.13")
                                  << QLatin1String("Lynx/2.8 (compatible; iCab 2.9.8; Macintosh; U; 68K)")
                                  << QLatin1String("Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en; rv:1.8.1.14) Gecko/20080409 Camino/1.6 (like Firefox/2.0.0.14)")
                                  << QLatin1String("msnbot/2.1")
                                  << QLatin1String("Mozilla/5.0 (iPad; U; CPU OS 3_2 like Mac OS X; en-us) AppleWebKit/531.21.10 (KHTML, like Gecko) Version/4.0.4 Mobile/7B334b Safari/531.21.10")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; ; en-NZ) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.8.0")
                                  << QLatin1String("NCSA Mosaic/3.0 (Windows 95)")
                                  << QLatin1String("Mozilla/5.0 (SymbianOS/9.1; U; [en]; Series60/3.0 NokiaE60/4.06.0) AppleWebKit/413 (KHTML, like Gecko) Safari/413")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/534.16 (KHTML, like Gecko) Chrome/10.0.648.133 Safari/534.16");

QString InternalNetworkAccessManager::userAgentString = QString::null;
InternalNetworkAccessManager *InternalNetworkAccessManager::instance = NULL;

InternalNetworkAccessManager::InternalNetworkAccessManager(QObject *parent)
        : QNetworkAccessManager(parent)
{
    cookieJar = new HTTPEquivCookieJar(this);
}


void InternalNetworkAccessManager::mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url)
{
    Q_ASSERT(cookieJar != NULL);
    cookieJar->mergeHtmlHeadCookies(htmlCode, url);
    setCookieJar(cookieJar);
}

InternalNetworkAccessManager *InternalNetworkAccessManager::self()
{
    if (instance == NULL) {
        instance = new InternalNetworkAccessManager(KApplication::instance());
    }

    return instance;
}

QNetworkReply *InternalNetworkAccessManager::get(QNetworkRequest &request, const QUrl &oldUrl)
{
    /// Query the KDE subsystem if a proxy has to be used
    /// for the host of a given URL
    QString proxyHostName = KProtocolManager::proxyForUrl(request.url());
    if (!proxyHostName.isEmpty() && proxyHostName != QLatin1String("DIRECT")) {
        /// Extract both hostname and port number for proxy
        proxyHostName = proxyHostName.mid(proxyHostName.indexOf(QLatin1String("://")) + 3);
        const QStringList proxyComponents = proxyHostName.split(QChar(':'));
        /// Set proxy to Qt's NetworkAccessManager
        setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxyComponents[0], proxyComponents[1].toInt()));
    } else {
        /// No proxy to be used, clear previous settings
        setProxy(QNetworkProxy());
    }

    request.setRawHeader(QString("Accept").toAscii(), QString("text/*, */*;q=0.7").toAscii());
    request.setRawHeader(QString("Accept-Charset").toAscii(), QString("utf-8, us-ascii, ISO-8859-1, ISO-8859-15, windows-1252").toAscii());
    request.setRawHeader(QString("Accept-Language").toAscii(), QString("en-US, en;q=0.9").toAscii());
    request.setRawHeader("User-Agent", userAgent().toAscii());
    if (oldUrl.isValid())
        request.setRawHeader("Referer", oldUrl.toString().toAscii());
    QNetworkReply *reply = QNetworkAccessManager::get(request);
    return reply;
}

QNetworkReply *InternalNetworkAccessManager::get(QNetworkRequest &request, const QNetworkReply *oldReply)
{
    return get(request, oldReply == NULL ? QUrl() : oldReply->url());
}

QString InternalNetworkAccessManager::userAgent()
{
    if (userAgentString.isEmpty()) {
        qsrand(time(NULL));
        userAgentString = userAgentList[qrand()%userAgentList.length()];
    }
    return userAgentString;
}
