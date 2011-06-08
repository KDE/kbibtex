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
    if (m_name.isNull())
        m_name = label().replace(QRegExp("[^a-z0-9]", Qt::CaseInsensitive), QLatin1String(""));
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
        result << text.mid(p1, p2 - p1 + 1);
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
    static QRegExp mimeRegExp("%([0-9A-Fa-f]{2})");
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
    static const QString inputTagBegin = QLatin1String("<input");
    static const QString selectTagBegin = QLatin1String("<select ");
    static const QString selectTagEnd = QLatin1String("</select>");
    static const QString optionTagBegin = QLatin1String("<option ");
    /// regular expressions to test or retrieve attributes in HTML tags
    QRegExp inputTypeRegExp("<input[^>]+\\btype=[\"]?([^\" >\n\t]*)");
    QRegExp inputNameRegExp("<input[^>]+\\bname=[\"]?([^\" >\n\t]*)");
    QRegExp inputValueRegExp("<input[^>]+\\bvalue=[\"]?([^\" >\n\t]*)");
    QRegExp inputIsCheckedRegExp("<input[^>]* checked([> \t\n]|=[\"]?checked)");
    QRegExp selectNameRegExp("<select[^>]+\\bname=[\"]?([^\" >\n\t]*)");
    QRegExp optionValueRegExp("<option[^>]+\\bvalue=[\"]?([^\" >\n\t]*)");
    QRegExp optionSelectedRegExp("<option[^>]* selected([> \t\n]|=[\"]?selected)");

    /// initialize result map
    QMap<QString, QString> result;

    /// determined boundaries of (only) "form" tag
    int startPos = htmlText.indexOf(formTagBegin);
    int endPos = htmlText.indexOf(formTagEnd, startPos);

    /// search for "input" tags within form
    int p = htmlText.indexOf(inputTagBegin, startPos);
    while (p > startPos && p < endPos) {
        /// get "type", "name", and "value" attributes
        QString inputType = htmlText.indexOf(inputTypeRegExp, p) == p ? inputTypeRegExp.cap(1).toLower() : QString();
        QString inputName = htmlText.indexOf(inputNameRegExp, p) == p ? inputNameRegExp.cap(1) : QString();
        QString inputValue = htmlText.indexOf(inputValueRegExp, p) ? inputValueRegExp.cap(1) : QString();

        if (!inputName.isEmpty()) {
            /// get value of input types
            if (inputType == "hidden" || inputType == "text" || inputType == "submit")
                result[inputName] = inputValue;
            else if (inputType == "radio" || inputType == "checkbox") {
                /// must be checked or selected
                if (htmlText.indexOf(inputIsCheckedRegExp, p) == p) {
                    result[inputName] = inputValue;
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
        QString selectName = htmlText.indexOf(selectNameRegExp, p) == p ? selectNameRegExp.cap(1) : QString::null;

        /// "select" tag contains one or several "option" tags, search all
        int popt = htmlText.indexOf(optionTagBegin, p);
        int endSelect = htmlText.indexOf(selectTagEnd, p);
        while (popt > p && popt < endSelect) {
            /// get "value" attribute from "option" tag
            QString optionValue = htmlText.indexOf(optionValueRegExp, popt) == popt ? optionValueRegExp.cap(1) : QString::null;
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
    // request.setRawHeader(QString("User-Agent").toAscii(),QString("Opera/9.80 (X11; Linux i686; U; en) Presto/2.7.62 Version/11.01").toAscii());
    request.setRawHeader(QString("User-Agent").toAscii(), QString("Links (2.3-pre1; NetBSD 5.0 i386; 96x36)").toAscii());
    request.setRawHeader(QString("Accept").toAscii(), QString("text/*, */*;q=0.7").toAscii());
    request.setRawHeader(QString("Accept-Charset").toAscii(), QString("utf-8, us-ascii, ISO-8859-1, ISO-8859-15, windows-1252").toAscii());
    request.setRawHeader(QString("Accept-Language").toAscii(), QString("en-US, en;q=0.9").toAscii());
}

QNetworkAccessManager *WebSearchAbstract::networkAccessManager()
{
    if (m_networkAccessManager == NULL) {
        m_networkAccessManager = new QNetworkAccessManager(KApplication::instance());
        m_networkAccessManager->setCookieJar(new HTTPEquivCookieJar(m_networkAccessManager));
    }

    return m_networkAccessManager;
}

void WebSearchAbstract::setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec)
{
    QTimer *timer = new QTimer(reply);
    connect(timer, SIGNAL(timeout()), this, SLOT(networkReplyTimeout()));
    m_mapTimerToReply.insert(timer, reply);
    timer->start(timeOutSec*1000);
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
