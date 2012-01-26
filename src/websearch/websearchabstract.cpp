/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <cstdlib>
#include <ctime>

#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include <KStandardDirs>
#include <kio/netaccess.h>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KApplication>

#include <encoderlatex.h>
#include "websearchabstract.h"

const QString WebSearchAbstract::queryKeyFreeText = QLatin1String("free");
const QString WebSearchAbstract::queryKeyTitle = QLatin1String("title");
const QString WebSearchAbstract::queryKeyAuthor = QLatin1String("author");
const QString WebSearchAbstract::queryKeyYear = QLatin1String("year");

/// various browser strings to "disguise" origin
const QStringList WebSearchAbstract::m_userAgentList = QStringList()
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
        << QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/534.16 (KHTML, like Gecko) Chrome/10.0.648.133 Safari/534.16");

const int WebSearchAbstract::resultNoError = 0;
const int WebSearchAbstract::resultCancelled = 0; /// may get redefined in the future!
const int WebSearchAbstract::resultUnspecifiedError = 1;

const char* WebSearchAbstract::httpUnsafeChars = "%:/=+$?&\0";


HTTPEquivCookieJar::HTTPEquivCookieJar(QNetworkAccessManager *parent = NULL)
        : QNetworkCookieJar(parent), m_nam(parent)
{
    // nothing
}

void HTTPEquivCookieJar::checkForHttpEqiuv(const QString &htmlCode, const QUrl &url)
{
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
        //cookieUrl.setPath(path);
        QList<QNetworkCookie> cookies = cookiesForUrl(cookieUrl);
        cookies.append(QNetworkCookie(key.toAscii(), value.toAscii()));
        setCookiesFromUrl(cookies, cookieUrl);
    }
}

QStringList WebSearchQueryFormAbstract::authorLastNames(const Entry &entry)
{
    QStringList result;
    EncoderLaTeX *encoder = EncoderLaTeX::currentEncoderLaTeX();

    const Value v = entry[Entry::ftAuthor];
    Person *p = NULL;
    foreach(ValueItem *vi, v)
    if ((p = dynamic_cast<Person*>(vi)) != NULL)
        result.append(encoder->convertToPlainAscii(p->lastName()));

    return result;
}

WebSearchAbstract::WebSearchAbstract(QWidget *parent)
        : QObject(parent), m_name(QString::null)
{
    m_parent = parent;
}

KIcon WebSearchAbstract::icon() const
{
    QString fileName = favIconUrl();
    fileName = fileName.replace(QRegExp("[^-a-z0-9_]", Qt::CaseInsensitive), "");
    fileName.prepend(KStandardDirs::locateLocal("cache", "favicons/"));

    if (!QFileInfo(fileName).exists()) {
        if (!KIO::NetAccess::file_copy(KUrl(favIconUrl()), KUrl(fileName), NULL))
            return KIcon();
    }

    return KIcon(fileName);
}

QString WebSearchAbstract::name()
{
    static const QRegExp invalidChars("[^-a-z0-9]", Qt::CaseInsensitive);
    if (m_name.isNull())
        m_name = label().replace(invalidChars, QLatin1String(""));
    return m_name;
}

void WebSearchAbstract::cancel()
{
    m_hasBeenCanceled = true;
}

QStringList WebSearchAbstract::splitRespectingQuotationMarks(const QString &text)
{
    int p1 = 0, p2, max = text.length();
    QStringList result;

    while (p1 < max) {
        while (text[p1] == ' ') ++p1;
        p2 = p1;
        if (text[p2] == '"') {
            ++p2;
            while (p2 < max && text[p2] != '"')  ++p2;
        } else
            while (p2 < max && text[p2] != ' ') ++p2;
        result << text.mid(p1, p2 - p1 + 1).simplified();
        p1 = p2 + 1;
    }
    return result;
}

bool WebSearchAbstract::handleErrors(QNetworkReply *reply)
{
    if (m_hasBeenCanceled) {
        kDebug() << "Searching" << label() << "got cancelled";
        emit stoppedSearch(resultCancelled);
        return false;
    } else if (reply->error() != QNetworkReply::NoError) {
        m_hasBeenCanceled = true;
        kWarning() << "Search using" << label() << "failed (HTTP code" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray() << ")";
        KMessageBox::error(m_parent, reply->errorString().isEmpty() ? i18n("Searching \"%1\" failed for unknown reason.", label()) : i18n("Searching \"%1\" failed with error message:\n\n%2", label(), reply->errorString()));
        emit stoppedSearch(resultUnspecifiedError);
        return false;
    }
    return true;
}

QString WebSearchAbstract::encodeURL(QString rawText)
{
    const char *cur = httpUnsafeChars;
    while (*cur != '\0') {
        rawText = rawText.replace(QChar(*cur), '%' + QString::number(*cur, 16));
        ++cur;
    }
    rawText = rawText.replace(" ", "+");
    return rawText;
}

QString WebSearchAbstract::decodeURL(QString rawText)
{
    static const QRegExp mimeRegExp("%([0-9A-Fa-f]{2})");
    while (mimeRegExp.indexIn(rawText) >= 0) {
        bool ok = false;
        QChar c(mimeRegExp.cap(1).toInt(&ok, 16));
        if (ok)
            rawText = rawText.replace(mimeRegExp.cap(0), c);
    }
    rawText = rawText.replace("&amp;", "&").replace("+", " ");
    return rawText;
}

QMap<QString, QString> WebSearchAbstract::formParameters(const QString &htmlText, const QString &formTagBegin)
{
    /// how to recognize HTML tags
    static const QString formTagEnd = QLatin1String("</form>");
    static const QString inputTagBegin = QLatin1String("<input ");
    static const QString selectTagBegin = QLatin1String("<select ");
    static const QString selectTagEnd = QLatin1String("</select>");
    static const QString optionTagBegin = QLatin1String("<option ");
    /// regular expressions to test or retrieve attributes in HTML tags
    static const QRegExp inputTypeRegExp("<input[^>]+\\btype=[\"]?([^\" >\n\t]*)", Qt::CaseInsensitive);
    static const QRegExp inputNameRegExp("<input[^>]+\\bname=[\"]?([^\" >\n\t]*)", Qt::CaseInsensitive);
    static const QRegExp inputValueRegExp("<input[^>]+\\bvalue=[\"]?([^\" >\n\t]*)", Qt::CaseInsensitive);
    static const QRegExp inputIsCheckedRegExp("<input[^>]+\\bchecked([> \t\n]|=[\"]?checked)", Qt::CaseInsensitive);
    static const QRegExp selectNameRegExp("<select[^>]+\\bname=[\"]?([^\" >\n\t]*)", Qt::CaseInsensitive);
    static const QRegExp optionValueRegExp("<option[^>]+\\bvalue=[\"]?([^\" >\n\t]*)", Qt::CaseInsensitive);
    static const QRegExp optionSelectedRegExp("<option[^>]* selected([> \t\n]|=[\"]?selected)", Qt::CaseInsensitive);

    /// initialize result map
    QMap<QString, QString> result;

    /// determined boundaries of (only) "form" tag
    int startPos = htmlText.indexOf(formTagBegin);
    int endPos = htmlText.indexOf(formTagEnd, startPos);

    /// search for "input" tags within form
    int p = htmlText.indexOf(inputTagBegin, startPos);
    while (p > startPos && p < endPos) {
        /// get "type", "name", and "value" attributes
        QString inputType = inputTypeRegExp.indexIn(htmlText, p) == p ? inputTypeRegExp.cap(1).toLower() : QString();
        QString inputName = inputNameRegExp.indexIn(htmlText, p) == p ? inputNameRegExp.cap(1) : QString();
        QString inputValue = inputValueRegExp.indexIn(htmlText, p) ? inputValueRegExp.cap(1) : QString();

        if (!inputName.isEmpty()) {
            /// get value of input types
            if (inputType == "hidden" || inputType == "text" || inputType == "submit")
                result[inputName] = inputValue;
            else if (inputType == "radio") {
                /// must be selected
                if (htmlText.indexOf(inputIsCheckedRegExp, p) == p) {
                    result[inputName] = inputValue;
                }
            } else if (inputType == "checkbox") {
                /// must be checked
                if (htmlText.indexOf(inputIsCheckedRegExp, p) == p) {
                    /// multiple checkbox values with the same name are possible
                    result.insertMulti(inputName, inputValue);
                }
            }
        }
        /// ignore input type "image"

        p = htmlText.indexOf(inputTagBegin, p + 1);
    }

    /// search for "select" tags within form
    p = htmlText.indexOf(selectTagBegin, startPos);
    while (p > startPos && p < endPos) {
        /// get "name" attribute from "select" tag
        QString selectName = selectNameRegExp.indexIn(htmlText, p) == p ? selectNameRegExp.cap(1) : QString::null;

        /// "select" tag contains one or several "option" tags, search all
        int popt = htmlText.indexOf(optionTagBegin, p);
        int endSelect = htmlText.indexOf(selectTagEnd, p);
        while (popt > p && popt < endSelect) {
            /// get "value" attribute from "option" tag
            QString optionValue = optionValueRegExp.indexIn(htmlText, popt) == popt ? optionValueRegExp.cap(1) : QString::null;
            if (!selectName.isNull() && !optionValue.isNull()) {
                /// if this "option" tag is "selected", store value
                if (htmlText.indexOf(optionSelectedRegExp, popt) == popt) {
                    result[selectName] = optionValue;
                }
            }

            popt = htmlText.indexOf(optionTagBegin, popt + 1);
        }

        p = htmlText.indexOf(selectTagBegin, p + 1);
    }

    return result;
}

void WebSearchAbstract::setSuggestedHttpHeaders(QNetworkRequest &request, QNetworkReply *oldReply)
{
    if (oldReply != NULL)
        request.setRawHeader(QString("Referer").toAscii(), oldReply->url().toString().toAscii());
    request.setRawHeader(QString("User-Agent").toAscii(), m_userAgent.toAscii());
    request.setRawHeader(QString("Accept").toAscii(), QString("text/*, */*;q=0.7").toAscii());
    request.setRawHeader(QString("Accept-Charset").toAscii(), QString("utf-8, us-ascii, ISO-8859-1, ISO-8859-15, windows-1252").toAscii());
    request.setRawHeader(QString("Accept-Language").toAscii(), QString("en-US, en;q=0.9").toAscii());
}

QNetworkAccessManager *WebSearchAbstract::networkAccessManager()
{
    if (m_networkAccessManager == NULL) {
        srand(time(NULL));
        m_networkAccessManager = new QNetworkAccessManager(KApplication::instance());
        m_networkAccessManager->setCookieJar(new HTTPEquivCookieJar(m_networkAccessManager));
        m_userAgent = m_userAgentList[rand()%m_userAgentList.length()];
    }

    return m_networkAccessManager;
}

void WebSearchAbstract::setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec)
{
    QTimer *timer = new QTimer(reply);
    connect(timer, SIGNAL(timeout()), this, SLOT(networkReplyTimeout()));
    m_mapTimerToReply.insert(timer, reply);
    timer->start(timeOutSec * 1000);
    connect(reply, SIGNAL(finished()), this, SLOT(networkReplyFinished()));
}

void WebSearchAbstract::networkReplyTimeout()
{
    QTimer *timer = static_cast<QTimer*>(sender());
    QNetworkReply *reply = m_mapTimerToReply[timer];
    if (reply != NULL) {
        kDebug() << "Timout on reply to " << reply->url().toString();
        reply->close();
        m_mapTimerToReply.remove(timer);
    }
}
void WebSearchAbstract::networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    QTimer *timer = m_mapTimerToReply.key(reply, NULL);
    if (timer != NULL) {
        m_mapTimerToReply.remove(timer);
        timer->stop();
    }
}

QNetworkAccessManager *WebSearchAbstract::m_networkAccessManager = NULL;
