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

#include "internalnetworkaccessmanager.h"

#include <ctime>

#include <QStringList>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtGlobal>
#include <QApplication>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <KProtocolManager>

#include "logging_networking.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class InternalNetworkAccessManager::HTTPEquivCookieJar: public QNetworkCookieJar
{
    Q_OBJECT

public:
    void mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url) {
        static const QRegularExpression cookieContent("^([^\"=; ]+)=([^\"=; ]+).*\\bpath=([^\"=; ]+)", QRegularExpression::CaseInsensitiveOption);
        int p1 = -1;
        QRegularExpressionMatch cookieContentRegExpMatch;
        if ((p1 = htmlCode.toLower().indexOf(QStringLiteral("http-equiv=\"set-cookie\""), 0, Qt::CaseInsensitive)) >= 5
                && (p1 = htmlCode.lastIndexOf(QStringLiteral("<meta"), p1, Qt::CaseInsensitive)) >= 0
                && (p1 = htmlCode.indexOf(QStringLiteral("content=\""), p1, Qt::CaseInsensitive)) >= 0
                && (cookieContentRegExpMatch = cookieContent.match(htmlCode.mid(p1 + 9, 512))).hasMatch()) {
            const QString key = cookieContentRegExpMatch.captured(1);
            const QString value = cookieContentRegExpMatch.captured(2);
            QList<QNetworkCookie> cookies = cookiesForUrl(url);
            cookies.append(QNetworkCookie(key.toLatin1(), value.toLatin1()));
            setCookiesFromUrl(cookies, url);
        }
    }

    HTTPEquivCookieJar(QObject *parent = nullptr)
            : QNetworkCookieJar(parent) {
        /// nothing
    }
};


QString InternalNetworkAccessManager::userAgentString;

InternalNetworkAccessManager::InternalNetworkAccessManager(QObject *parent)
        : QNetworkAccessManager(parent)
{
    cookieJar = new HTTPEquivCookieJar(this);
}


void InternalNetworkAccessManager::mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url)
{
    Q_ASSERT_X(cookieJar != nullptr, "void InternalNetworkAccessManager::mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url)", "cookieJar is invalid");
    cookieJar->mergeHtmlHeadCookies(htmlCode, url);
    setCookieJar(cookieJar);
}

InternalNetworkAccessManager &InternalNetworkAccessManager::instance()
{
    static InternalNetworkAccessManager self;
    return self;
}

QNetworkReply *InternalNetworkAccessManager::get(QNetworkRequest &request, const QUrl &oldUrl)
{
#ifdef HAVE_KF5
    /// Query the KDE subsystem if a proxy has to be used
    /// for the host of a given URL
    QString proxyHostName = KProtocolManager::proxyForUrl(request.url());
    if (!proxyHostName.isEmpty() && proxyHostName != QStringLiteral("DIRECT")) {
        /// Extract both hostname and port number for proxy
        proxyHostName = proxyHostName.mid(proxyHostName.indexOf(QStringLiteral("://")) + 3);
        QStringList proxyComponents = proxyHostName.split(QStringLiteral(":"), QString::SkipEmptyParts);
        if (proxyComponents.length() == 1) {
            /// Proxy configuration is missing a port number,
            /// using 8080 as default
            proxyComponents << QStringLiteral("8080");
        }
        if (proxyComponents.length() == 2) {
            /// Set proxy to Qt's NetworkAccessManager
            setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxyComponents[0], proxyComponents[1].toInt()));
        }
    } else {
        /// No proxy to be used, clear previous settings
        setProxy(QNetworkProxy());
    }
#else // HAVE_KF5
    setProxy(QNetworkProxy());
#endif // HAVE_KF5

    if (!request.hasRawHeader(QByteArray("Accept")))
        request.setRawHeader(QByteArray("Accept"), QByteArray("text/*, */*;q=0.7"));
    request.setRawHeader(QByteArray("Accept-Charset"), QByteArray("utf-8, us-ascii, ISO-8859-1;q=0.7, ISO-8859-15;q=0.7, windows-1252;q=0.3"));
    request.setRawHeader(QByteArray("Accept-Language"), QByteArray("en-US, en;q=0.9"));
    request.setRawHeader(QByteArray("User-Agent"), userAgent().toLatin1());
    if (oldUrl.isValid())
        request.setRawHeader(QByteArray("Referer"), removeApiKey(oldUrl).toDisplayString().toLatin1());
    QNetworkReply *reply = QNetworkAccessManager::get(request);

    /// Log SSL errors
    connect(reply, &QNetworkReply::sslErrors, this, &InternalNetworkAccessManager::logSslErrors);

    return reply;
}

QNetworkReply *InternalNetworkAccessManager::get(QNetworkRequest &request, const QNetworkReply *oldReply)
{
    return get(request, oldReply == nullptr ? QUrl() : oldReply->url());
}

QString InternalNetworkAccessManager::userAgent()
{
    /// Various browser strings to "disguise" origin
    static const QStringList userAgentList {
            QStringLiteral("Mozilla/5.0 (Linux; U; Android 2.3.3; en-us; HTC_DesireS_S510e Build/GRI40) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1"),
            QStringLiteral("Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) QupZilla/2.0.2 Chrome/45.0.2454.101 Safari/537.36"),
            QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36 OPR/38.0.2220.41"),
            QStringLiteral("Mozilla/5.0 (compatible; Yahoo! Slurp China; http://misc.yahoo.com.cn/help.html)"),
            QStringLiteral("Mozilla/5.0 (compatible; MSIE 9.0; AOL 9.7; AOLBuild 4343.19; Windows NT 6.1; WOW64; Trident/5.0; FunWebProducts)"),
            QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.89 Vivaldi/1.0.83.38 Safari/537.36"),
            QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.5 (KHTML, like Gecko) YaBrowser/1.0.1084.5402 Chrome/19.0.1084.5402 Safari/536.5"),
            QStringLiteral("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/523.15 (KHTML, like Gecko, Safari/419.3) Arora/0.3 (Change: 287 c9dfb30)"),
            QStringLiteral("Mozilla/4.0 (compatible; Dillo 2.2)"),
            QStringLiteral("Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/532.0 (KHTML, like Gecko) Chrome/4.0.201.1 Safari/532.0"),
            QStringLiteral("Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.1 (KHTML, like Gecko) Ubuntu/10.04 Chromium/14.0.813.0 Chrome/14.0.813.0 Safari/535.1"),
            QStringLiteral("Mozilla/5.0 (X11; U; Linux; de-DE) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.8.0"),
            QStringLiteral("Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.21 Safari/537.36 MMS/1.0.2459.0"),
            QStringLiteral("Mozilla/5.0 (X11; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) QtCarBrowser Safari/533.3"),
            QStringLiteral("Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.89 Vivaldi/1.0.83.38 Safari/537.36"),
            QStringLiteral("Opera/9.80 (X11; Linux i686; U; ru) Presto/2.8.131 Version/11.11"),
            QStringLiteral("Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; .NET CLR 1.1.4322; InfoPath.1; .NET CLR 2.0.50727) Sleipnir/2.8.4"),
            QStringLiteral("Mozilla/5.0 (X11; Linux i686; rv:2.2a1pre) Gecko/20110327 SeaMonkey/2.2a1pre"),
            QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/534.55.3 (KHTML, like Gecko) Version/5.1.3 Safari/534.53.10"),
            QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13) AppleWebKit/603.1.13 (KHTML, like Gecko) Version/10.1 Safari/603.1.13"),
            QStringLiteral("Mozilla/5.0 (Linux; U; Tizen/1.0 like Android; en-us; AppleWebKit/534.46 (KHTML, like Gecko) Tizen Browser/1.0 Mobile"),
            QStringLiteral("Emacs-W3/4.0pre.46 URL/p4.0pre.46 (i686-pc-linux; X11)"),
            QStringLiteral("Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0"),
            QStringLiteral("Lynx/2.8 (compatible; iCab 2.9.8; Macintosh; U; 68K)"),
            QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X x.y; rv:42.0) Gecko/20100101 Firefox/42.0"),
            QStringLiteral("msnbot/2.1"),
            QStringLiteral("Mozilla/5.0 (iPad; U; CPU OS 3_2 like Mac OS X; en-us) AppleWebKit/531.21.10 (KHTML, like Gecko) Version/4.0.4 Mobile/7B334b Safari/531.21.10"),
            QStringLiteral("Mozilla/5.0 (Windows; U; ; en-NZ) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.8.0"),
            QStringLiteral("NCSA Mosaic/3.0 (Windows 95)"),
            QStringLiteral("Mozilla/5.0 (SymbianOS/9.1; U; [en]; Series60/3.0 NokiaE60/4.06.0) AppleWebKit/413 (KHTML, like Gecko) Safari/413"),
            QStringLiteral("Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/534.16 (KHTML, like Gecko) Chrome/10.0.648.133 Safari/534.16")
    };

    if (userAgentString.isEmpty()) {
        qsrand(time(nullptr));
        userAgentString = userAgentList[qrand() % userAgentList.length()];
    }
    return userAgentString;
}

void InternalNetworkAccessManager::setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec)
{
    QTimer *timer = new QTimer(reply);
    connect(timer, &QTimer::timeout, this, &InternalNetworkAccessManager::networkReplyTimeout);
    m_mapTimerToReply.insert(timer, reply);
    timer->start(timeOutSec * 1000);
    connect(reply, &QNetworkReply::finished, this, &InternalNetworkAccessManager::networkReplyFinished);
}

QString InternalNetworkAccessManager::reverseObfuscate(const QByteArray &a) {
    if (a.length() % 2 != 0 || a.length() == 0) return QString();
    QString result;
    result.reserve(a.length() / 2);
    for (int p = a.length() - 1; p >= 0; p -= 2) {
        const QChar c = QLatin1Char(a.at(p) ^ a.at(p - 1));
        result.append(c);
    }
    return result;
}

QUrl InternalNetworkAccessManager::removeApiKey(QUrl url)
{
    QUrlQuery urlQuery(url);
    urlQuery.removeQueryItem(QStringLiteral("apikey"));
    urlQuery.removeQueryItem(QStringLiteral("api_key"));
    url.setQuery(urlQuery);
    return url;
}

void InternalNetworkAccessManager::networkReplyTimeout()
{
    QTimer *timer = static_cast<QTimer *>(sender());
    timer->stop();
    QNetworkReply *reply = m_mapTimerToReply[timer];
    if (reply != nullptr) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Timeout on reply to " << removeApiKey(reply->url()).toDisplayString();
        reply->close();
        m_mapTimerToReply.remove(timer);
    }
}
void InternalNetworkAccessManager::networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QTimer *timer = m_mapTimerToReply.key(reply, nullptr);
    if (timer != nullptr) {
        disconnect(timer, &QTimer::timeout, this, &InternalNetworkAccessManager::networkReplyTimeout);
        timer->stop();
        m_mapTimerToReply.remove(timer);
    }
}

void InternalNetworkAccessManager::logSslErrors(const QList<QSslError> &errors)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    qWarning() << QStringLiteral("Got the following SSL errors when querying the following URL: ") << removeApiKey(reply->url()).toDisplayString();
    for (const QSslError &error : errors)
        qWarning() << QStringLiteral(" * ") + error.errorString() << "; Code: " << static_cast<int>(error.error());
}

#include "internalnetworkaccessmanager.moc"
