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
#include <QNetworkReply>
#include <QTimer>

#include <KStandardDirs>
#include <kio/netaccess.h>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>

#include <encoderlatex.h>
#include <internalnetworkaccessmanager.h>
#include "onlinesearchabstract.h"

const QString OnlineSearchAbstract::queryKeyFreeText = QLatin1String("free");
const QString OnlineSearchAbstract::queryKeyTitle = QLatin1String("title");
const QString OnlineSearchAbstract::queryKeyAuthor = QLatin1String("author");
const QString OnlineSearchAbstract::queryKeyYear = QLatin1String("year");

const int OnlineSearchAbstract::resultNoError = 0;
const int OnlineSearchAbstract::resultCancelled = 0; /// may get redefined in the future!
const int OnlineSearchAbstract::resultUnspecifiedError = 1;

const char* OnlineSearchAbstract::httpUnsafeChars = "%:/=+$?&\0";


QStringList OnlineSearchQueryFormAbstract::authorLastNames(const Entry &entry)
{
    QStringList result;
    EncoderLaTeX *encoder = EncoderLaTeX::instance();

    const Value v = entry[Entry::ftAuthor];
    Person *p = NULL;
    foreach(ValueItem *vi, v)
    if ((p = dynamic_cast<Person*>(vi)) != NULL)
        result.append(encoder->convertToPlainAscii(p->lastName()));

    return result;
}

OnlineSearchAbstract::OnlineSearchAbstract(QWidget *parent)
        : QObject(parent), m_name(QString::null)
{
    m_parent = parent;
}

KIcon OnlineSearchAbstract::icon() const
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

QString OnlineSearchAbstract::name()
{
    if (m_name.isNull())
        m_name = label().replace(QRegExp("[^a-z0-9]", Qt::CaseInsensitive), QLatin1String(""));
    return m_name;
}

void OnlineSearchAbstract::cancel()
{
    m_hasBeenCanceled = true;
}

QStringList OnlineSearchAbstract::splitRespectingQuotationMarks(const QString &text)
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

bool OnlineSearchAbstract::handleErrors(QNetworkReply *reply)
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

    /**
     * Check the reply for various problems that might point to
     * more severe issues. Remember: those are only indicators
     * to problems which have to be handled elsewhere (therefore,
     * returning 'true' is totally ok here).
     */
    QStringList issues;
    if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid())
        issues << QString("Redirection to '%1'").arg(reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl()).toString());
    if (reply->size() == 0)
        issues << QLatin1String("No data returned");
    if (!issues.isEmpty())
        kWarning() << "Search using" << label() << " on url" << reply->url().toString() << "returned the following issues:" << issues.join(QLatin1String("; "));

    return true;
}

QString OnlineSearchAbstract::encodeURL(QString rawText)
{
    const char *cur = httpUnsafeChars;
    while (*cur != '\0') {
        rawText = rawText.replace(QChar(*cur), '%' + QString::number(*cur, 16));
        ++cur;
    }
    rawText = rawText.replace(" ", "+");
    return rawText;
}

QString OnlineSearchAbstract::decodeURL(QString rawText)
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

QMap<QString, QString> OnlineSearchAbstract::formParameters(const QString &htmlText, const QString &formTagBegin)
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


void OnlineSearchAbstract::setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec)
{
    QTimer *timer = new QTimer(reply);
    connect(timer, SIGNAL(timeout()), this, SLOT(networkReplyTimeout()));
    m_mapTimerToReply.insert(timer, reply);
    timer->start(timeOutSec*1000);
    connect(reply, SIGNAL(finished()), this, SLOT(networkReplyFinished()));
}

void OnlineSearchAbstract::networkReplyTimeout()
{
    QTimer *timer = static_cast<QTimer*>(sender());
    QNetworkReply *reply = m_mapTimerToReply[timer];
    if (reply != NULL) {
        kDebug() << "Timout on reply to " << reply->url().toString();
        reply->close();
        m_mapTimerToReply.remove(timer);
    }
}
void OnlineSearchAbstract::networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    QTimer *timer = m_mapTimerToReply.key(reply, NULL);
    if (timer != NULL) {
        m_mapTimerToReply.remove(timer);
        timer->stop();
    }
}

QNetworkAccessManager *OnlineSearchAbstract::m_networkAccessManager = NULL;
