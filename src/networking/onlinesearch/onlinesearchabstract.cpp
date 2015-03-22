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

#include "onlinesearchabstract.h"

#include <QFileInfo>
#include <QNetworkReply>
#include <QTimer>
#include <QListWidgetItem>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>


#include <kio/netaccess.h>
#include <QDebug>
#include <KLocale>
#include <KMessageBox>
#include <KPassivePopup>
#include <QStandardPaths>

#include "encoderlatex.h"
#include "internalnetworkaccessmanager.h"
#include "kbibtexnamespace.h"

const QString OnlineSearchAbstract::queryKeyFreeText = QLatin1String("free");
const QString OnlineSearchAbstract::queryKeyTitle = QLatin1String("title");
const QString OnlineSearchAbstract::queryKeyAuthor = QLatin1String("author");
const QString OnlineSearchAbstract::queryKeyYear = QLatin1String("year");

const int OnlineSearchAbstract::resultNoError = 0;
const int OnlineSearchAbstract::resultCancelled = 0; /// may get redefined in the future!
const int OnlineSearchAbstract::resultUnspecifiedError = 1;
const int OnlineSearchAbstract::resultAuthorizationRequired = 2;
const int OnlineSearchAbstract::resultNetworkError = 3;
const int OnlineSearchAbstract::resultInvalidArguments = 4;

const char *OnlineSearchAbstract::httpUnsafeChars = "%:/=+$?&\0";


QStringList OnlineSearchQueryFormAbstract::authorLastNames(const Entry &entry)
{
    QStringList result;
    EncoderLaTeX *encoder = EncoderLaTeX::instance();

    const Value v = entry[Entry::ftAuthor];
    QSharedPointer<Person> p;
    foreach(QSharedPointer<ValueItem> vi, v)
    if (!(p = vi.dynamicCast<Person>()).isNull())
        result.append(encoder->convertToPlainAscii(p->lastName()));

    return result;
}

OnlineSearchAbstract::OnlineSearchAbstract(QWidget *parent)
        : QObject(parent), m_name(QString())
{
    m_parent = parent;
}

QIcon OnlineSearchAbstract::icon(QListWidgetItem *listWidgetItem)
{
    static const QRegExp invalidChars(QLatin1String("[^-a-z0-9_]"), Qt::CaseInsensitive);
    const QString fileNameStem = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/favicons/")) + QString(favIconUrl()).remove(invalidChars;
    const QStringList fileNameExtensions = QStringList() << QLatin1String(".ico") << QLatin1String(".png") << QString();

    foreach(const QString &extension, fileNameExtensions) {
        const QString fileName = fileNameStem + extension;
        if (QFileInfo(fileName).exists())
            return QIcon::fromTheme(fileName);
    }

    QNetworkRequest request(favIconUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    reply->setObjectName(fileNameStem);
    if (listWidgetItem != NULL)
        m_iconReplyToListWidgetItem.insert(reply, listWidgetItem);
    connect(reply, SIGNAL(finished()), this, SLOT(iconDownloadFinished()));
    return QIcon::fromTheme(QLatin1String("applications-internet"));
}

QString OnlineSearchAbstract::name()
{
    static const QRegExp invalidChars("[^-a-z0-9]", Qt::CaseInsensitive);
    if (m_name.isEmpty())
        m_name = label().remove(invalidChars);
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
    QUrl url;
    return handleErrors(reply, url);
}

bool OnlineSearchAbstract::handleErrors(QNetworkReply *reply, QUrl &newUrl)
{
    newUrl = QUrl();
    if (m_hasBeenCanceled) {
        emit stoppedSearch(resultCancelled);
        return false;
    } else if (reply->error() != QNetworkReply::NoError) {
        m_hasBeenCanceled = true;
        const QString errorString = reply->errorString();
        qWarning() << "Search using" << label() << "failed (error code" << reply->error() << "(" << errorString << "), HTTP code" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << ":" << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray() << ")";
        sendVisualNotification(errorString.isEmpty() ? i18n("Searching '%1' failed for unknown reason.", label()) : i18n("Searching '%1' failed with error message:\n\n%2", label(), errorString), label(), "kbibtex", 7 * 1000);

        int resultCode = resultUnspecifiedError;
        if (reply->error() == QNetworkReply::AuthenticationRequiredError || reply->error() == QNetworkReply::ProxyAuthenticationRequiredError)
            resultCode = resultAuthorizationRequired;
        else if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::TimeoutError)
            resultCode = resultNetworkError;
        emit stoppedSearch(resultCode);

        return false;
    }

    /**
     * Check the reply for various problems that might point to
     * more severe issues. Remember: those are only indicators
     * to problems which have to be handled elsewhere (therefore,
     * returning 'true' is totally ok here).
     */
    if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        newUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
    } else if (reply->size() == 0)
        qWarning() << "Search using" << label() << "on url" << reply->url().toString() << "returned no data";

    return true;
}

/**
 * Display a passive notification popup using the D-Bus interface.
 * Copied from KDialog with modifications.
 */
void OnlineSearchAbstract::sendVisualNotification(const QString &text, const QString &title, const QString &icon, int timeout)
{
    static const QString dbusServiceName = QLatin1String("org.freedesktop.Notifications");
    static const QString dbusInterfaceName = QLatin1String("org.freedesktop.Notifications");
    static const QString dbusPath = QLatin1String("/org/freedesktop/Notifications");

    // check if service already exists on plugin instantiation
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    if (interface == NULL || !interface->isServiceRegistered(dbusServiceName)) {
        return;
    }

    if (timeout <= 0)
        timeout = 10 * 1000;

    QDBusMessage m = QDBusMessage::createMethodCall(dbusServiceName, dbusPath, dbusInterfaceName, "Notify");
    QList<QVariant> args = QList<QVariant>() << QLatin1String("kdialog") << 0U << icon << title << text << QStringList() << QVariantMap() << timeout;
    m.setArguments(args);

    QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
    if (replyMsg.type() == QDBusMessage::ReplyMessage) {
        if (!replyMsg.arguments().isEmpty()) {
            return;
        }
        // Not displaying any error messages as this is optional for kdialog
        // and KPassivePopup is a perfectly valid fallback.
        //else {
        //  qDebug() << "Error: received reply with no arguments.";
        //}
    } else if (replyMsg.type() == QDBusMessage::ErrorMessage) {
        //qDebug() << "Error: failed to send D-Bus message";
        //qDebug() << replyMsg;
    } else {
        //qDebug() << "Unexpected reply type";
    }
}

QString OnlineSearchAbstract::encodeURL(QString rawText)
{
    const char *cur = httpUnsafeChars;
    while (*cur != '\0') {
        rawText = rawText.replace(QChar(*cur), '%' + QString::number(*cur, 16));
        ++cur;
    }
    rawText = rawText.replace(QLatin1Char(' '), QLatin1Char('+'));
    return rawText;
}

QString OnlineSearchAbstract::decodeURL(QString rawText)
{
    static const QRegExp mimeRegExp("%([0-9A-Fa-f]{2})");
    while (mimeRegExp.indexIn(rawText) >= 0) {
        bool ok = false;
        QChar c(mimeRegExp.cap(1).toInt(&ok, 16));
        if (ok)
            rawText = rawText.replace(mimeRegExp.cap(0), c);
    }
    rawText = rawText.replace(QLatin1String("&amp;"), QLatin1String("&")).replace(QLatin1Char('+'), QLatin1String(" "));
    return rawText;
}

QMap<QString, QString> OnlineSearchAbstract::formParameters(const QString &htmlText, const QString &formTagBegin)
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
    int startPos = htmlText.indexOf(formTagBegin, Qt::CaseInsensitive);
    int endPos = htmlText.indexOf(formTagEnd, startPos, Qt::CaseInsensitive);
    if (startPos < 0 || endPos < 0) {
        qWarning() << "Could not locate form" << formTagBegin << "in text";
        return result;
    }

    /// search for "input" tags within form
    int p = htmlText.indexOf(inputTagBegin, startPos, Qt::CaseInsensitive);
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

        p = htmlText.indexOf(inputTagBegin, p + 1, Qt::CaseInsensitive);
    }

    /// search for "select" tags within form
    p = htmlText.indexOf(selectTagBegin, startPos, Qt::CaseInsensitive);
    while (p > startPos && p < endPos) {
        /// get "name" attribute from "select" tag
        const QString selectName = selectNameRegExp.indexIn(htmlText, p) == p ? selectNameRegExp.cap(1) : QString();

        /// "select" tag contains one or several "option" tags, search all
        int popt = htmlText.indexOf(optionTagBegin, p, Qt::CaseInsensitive);
        int endSelect = htmlText.indexOf(selectTagEnd, p, Qt::CaseInsensitive);
        while (popt > p && popt < endSelect) {
            /// get "value" attribute from "option" tag
            const QString optionValue = optionValueRegExp.indexIn(htmlText, popt) == popt ? optionValueRegExp.cap(1) : QString();
            if (!selectName.isEmpty() && !optionValue.isEmpty()) {
                /// if this "option" tag is "selected", store value
                if (htmlText.indexOf(optionSelectedRegExp, popt) == popt) {
                    result[selectName] = optionValue;
                }
            }

            popt = htmlText.indexOf(optionTagBegin, popt + 1, Qt::CaseInsensitive);
        }

        p = htmlText.indexOf(selectTagBegin, p + 1, Qt::CaseInsensitive);
    }

    return result;
}

void OnlineSearchAbstract::iconDownloadFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray iconData = reply->readAll();

        QString extension;
        if (iconData[1] == 'P' && iconData[2] == 'N' && iconData[3] == 'G') {
            /// PNG files have string "PNG" at second to fourth byte
            extension = QLatin1String(".png");
        } else if (iconData[0] == (char)0x00 && iconData[1] == (char)0x00 && iconData[2] == (char)0x01 && iconData[3] == (char)0x00) {
            /// Microsoft Icon have first two bytes always 0x0000,
            /// third and fourth byte is 0x0001 (for .ico)
            extension = QLatin1String(".ico");
        }
        const QString filename = reply->objectName() + extension;

        QFile iconFile(filename);
        if (iconFile.open(QFile::WriteOnly)) {
            iconFile.write(iconData);
            iconFile.close();

            QListWidgetItem *listWidgetItem = m_iconReplyToListWidgetItem.value(reply, NULL);
            if (listWidgetItem != NULL)
                listWidgetItem->setIcon(QIcon::fromTheme(filename));
        }
    } else
        qWarning() << "Could not download icon " << reply->url().toString();
}

void OnlineSearchAbstract::dumpToFile(const QString &filename, const QString &text)
{
    const QString usedFilename = QDir::tempPath() + QLatin1Char('/') +  filename, true);

    QFile f(usedFilename);
    if (f.open(QFile::WriteOnly)) {
        qDebug() << "Dumping text" << squeezeText(text, 96) << "to" << usedFilename;
        QTextStream ts(&f);
        ts << text;
        f.close();
    }
}

void OnlineSearchAbstract::delayedStoppedSearch(int returnCode)
{
    m_delayedStoppedSearchReturnCode = returnCode;
    QTimer::singleShot(500, this, SLOT(delayedStoppedSearchTimer()));
}

void OnlineSearchAbstract::delayedStoppedSearchTimer()
{
    emit progress(1, 1);
    emit stoppedSearch(m_delayedStoppedSearchReturnCode);
}

void OnlineSearchAbstract::sanitizeEntry(QSharedPointer<Entry> entry)
{
    if (entry.isNull()) return;

    /// Sometimes, there is no identifier, so set a random one
    if (entry->id().isEmpty())
        entry->setId(QString(QLatin1String("entry-%1")).arg(QString::number(qrand(), 36)));

    const QString ftIssue = QLatin1String("issue");
    if (entry->contains(ftIssue)) {
        /// ACM's Digital Library uses "issue" instead of "number" -> fix that
        Value v = entry->value(ftIssue);
        entry->remove(ftIssue);
        entry->insert(Entry::ftNumber, v);
    }

    /// If entry contains a description field but no abstract,
    /// rename description field to abstract
    const QString ftDescription = QLatin1String("description");
    if (!entry->contains(Entry::ftAbstract) && entry->contains(ftDescription)) {
        Value v = entry->value(ftDescription);
        entry->remove(ftDescription);
        entry->insert(Entry::ftAbstract, v);
    }

    /// Remove "dblp" artifacts in abstracts and keywords
    if (entry->contains(Entry::ftAbstract)) {
        const QString abstract = PlainTextValue::text(entry->value(Entry::ftAbstract));
        if (abstract == QLatin1String("dblp"))
            entry->remove(Entry::ftAbstract);
    }
    if (entry->contains(Entry::ftKeywords)) {
        const QString keywords = PlainTextValue::text(entry->value(Entry::ftKeywords));
        if (keywords == QLatin1String("dblp"))
            entry->remove(Entry::ftKeywords);
    }

    if (entry->contains(Entry::ftMonth)) {
        /// Fix strigns for months: "September" -> "sep"
        const QString monthStr = PlainTextValue::text(entry->value(Entry::ftMonth));

        static const QRegExp longMonth = QRegExp(QLatin1String("(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[a-z]+"), Qt::CaseInsensitive);
        if (monthStr.indexOf(longMonth) == 0 && monthStr == longMonth.cap(0)) {
            /// String used for month is actually a full name, therefore replace it
            entry->remove(Entry::ftMonth);

            /// Regular expression checked for valid three-letter abbreviations
            /// that month names start with. Use those three letters for a macro key
            /// Example: "September" -> sep
            Value v;
            v.append(QSharedPointer<MacroKey>(new MacroKey(longMonth.cap(1).toLower())));
            entry->insert(Entry::ftMonth, v);
        }
    }

    /// Referenced strings or entries do not exist in the search result
    /// and BibTeX breaks if it finds a reference to a non-existing string or entry
    entry->remove(Entry::ftCrossRef);
}

bool OnlineSearchAbstract::publishEntry(QSharedPointer<Entry> entry)
{
    if (entry.isNull()) return false;

    Value v;
    v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
    entry->insert("x-fetchedfrom", v);

    sanitizeEntry(entry);

    emit foundEntry(entry);

    return true;
}
