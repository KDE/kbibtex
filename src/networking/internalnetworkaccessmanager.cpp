/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QCoreApplication>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#if QT_VERSION >= 0x050a00
#include <QRandomGenerator>
#endif // QT_VERSION

#ifdef HAVE_KF5
#include <KProtocolManager>
#endif // HAVE_KF5

#include "logging_networking.h"

#if QT_VERSION >= 0x050a00
#define randomGeneratorGlobalBounded(min,max)  QRandomGenerator::global()->bounded((min),(max))
#else // QT_VERSION
#define randomGeneratorGlobalBounded(min,max)  ((min)+(qrand()%((max)-(min)+1)))
#endif // QT_VERSION

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
#if QT_VERSION < 0x050a00
    qsrand(static_cast<int>(QDateTime::currentDateTime().toMSecsSinceEpoch() % 0x7fffffffl));
#endif // QT_VERSION
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
#if QT_VERSION >= 0x050e00
        QStringList proxyComponents = proxyHostName.split(QStringLiteral(":"), Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
        QStringList proxyComponents = proxyHostName.split(QStringLiteral(":"), QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
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
    /// Set 'Referer' and 'Origin' to match the request URL's domain, i.e. URL with empty path
    QUrl domainUrl = request.url();
    domainUrl.setPath(QString());
    const QByteArray domain = removeApiKey(domainUrl).toDisplayString().toLatin1();
    request.setRawHeader(QByteArray("Referer"), domain);
    request.setRawHeader(QByteArray("Origin"), domain);
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
    if (userAgentString.isEmpty()) {
        if (randomGeneratorGlobalBounded(0, 1) == 0) {
            /// Fake Chrome user agent string
            static const QString chromeTemplate{QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/%1.%2 (KHTML, like Gecko) Chrome/%3.%4.%5.%6 Safari/%1.%2")};
            const auto appleWebKitVersionMajor = randomGeneratorGlobalBounded(537, 856);
            const auto appleWebKitVersionMinor = randomGeneratorGlobalBounded(3, 53);
            const auto chromeVersionMajor = randomGeneratorGlobalBounded(77, 85);
            const auto chromeVersionMinor = randomGeneratorGlobalBounded(0, 4);
            const auto chromeVersionBuild = randomGeneratorGlobalBounded(3793, 8973);
            const auto chromeVersionPatch = randomGeneratorGlobalBounded(53, 673);
            userAgentString = chromeTemplate.arg(appleWebKitVersionMajor).arg(appleWebKitVersionMinor).arg(chromeVersionMajor).arg(chromeVersionMinor).arg(chromeVersionBuild).arg(chromeVersionPatch);
        } else {
            /// Fake Firefox user agent string
            static const QString mozillaTemplate{QStringLiteral("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:%1.%2) Gecko/20100101 Firefox/%1.%2")};
            const auto firefoxVersionMajor = randomGeneratorGlobalBounded(69, 74);
            const auto firefoxVersionMinor = randomGeneratorGlobalBounded(0, 2);
            userAgentString = mozillaTemplate.arg(firefoxVersionMajor).arg(firefoxVersionMinor);
        }
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

QString InternalNetworkAccessManager::removeApiKey(const QString &text)
{
    static const QRegularExpression apiKeyRegExp(QStringLiteral("\\bapi_?key=[^\"&? ]"));
    return QString(text).remove(apiKeyRegExp);
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
    qCWarning(LOG_KBIBTEX_NETWORKING) << QStringLiteral("Got the following SSL errors when querying the following URL: ") << removeApiKey(reply->url()).toDisplayString();
    for (const QSslError &error : errors)
        qCWarning(LOG_KBIBTEX_NETWORKING) << QStringLiteral(" * ") + error.errorString() << "; Code: " << static_cast<int>(error.error());
}

#include "internalnetworkaccessmanager.moc"
