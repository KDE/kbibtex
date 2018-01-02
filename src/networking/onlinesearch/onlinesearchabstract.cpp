/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchabstract.h"

#include <QFileInfo>
#include <QNetworkReply>
#include <QDir>
#include <QTimer>
#include <QStandardPaths>
#ifdef HAVE_QTWIDGETS
#include <QListWidgetItem>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#endif // HAVE_QTWIDGETS

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KMessageBox>
#endif // HAVE_KF5

#include "encoderlatex.h"
#include "internalnetworkaccessmanager.h"
#include "kbibtex.h"
#include "logging_networking.h"

const QString OnlineSearchAbstract::queryKeyFreeText = QStringLiteral("free");
const QString OnlineSearchAbstract::queryKeyTitle = QStringLiteral("title");
const QString OnlineSearchAbstract::queryKeyAuthor = QStringLiteral("author");
const QString OnlineSearchAbstract::queryKeyYear = QStringLiteral("year");

const int OnlineSearchAbstract::resultNoError = 0;
const int OnlineSearchAbstract::resultCancelled = 0; /// may get redefined in the future!
const int OnlineSearchAbstract::resultUnspecifiedError = 1;
const int OnlineSearchAbstract::resultAuthorizationRequired = 2;
const int OnlineSearchAbstract::resultNetworkError = 3;
const int OnlineSearchAbstract::resultInvalidArguments = 4;

const char *OnlineSearchAbstract::httpUnsafeChars = "%:/=+$?&\0";


#ifdef HAVE_QTWIDGETS
QStringList OnlineSearchQueryFormAbstract::authorLastNames(const Entry &entry)
{
    const EncoderLaTeX &encoder = EncoderLaTeX::instance();
    const Value v = entry[Entry::ftAuthor];
    QStringList result;
    result.reserve(v.size());
    for (const QSharedPointer<ValueItem> &vi : v) {
        QSharedPointer<const Person> p = vi.dynamicCast<const Person>();
        if (!p.isNull())
            result.append(encoder.convertToPlainAscii(p->lastName()));
    }

    return result;
}
#endif // HAVE_QTWIDGETS

OnlineSearchAbstract::OnlineSearchAbstract(QObject *parent)
        : QObject(parent), m_hasBeenCanceled(false), numSteps(0), curStep(0), m_previousBusyState(false), m_delayedStoppedSearchReturnCode(0)
{
    m_parent = parent;
}

QIcon OnlineSearchAbstract::icon(QListWidgetItem *listWidgetItem)
{
    static const QRegExp invalidChars(QStringLiteral("[^-a-z0-9_]"), Qt::CaseInsensitive);
    const QString cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/favicons/");
    QDir().mkpath(cacheDirectory);
    const QString fileNameStem = cacheDirectory + QString(favIconUrl()).remove(invalidChars);
    const QStringList fileNameExtensions = QStringList() << QStringLiteral(".ico") << QStringLiteral(".png") << QString();

    for (const QString &extension : fileNameExtensions) {
        const QString fileName = fileNameStem + extension;
        if (QFileInfo::exists(fileName))
            return QIcon(fileName);
    }

    QNetworkRequest request(favIconUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    reply->setObjectName(fileNameStem);
    if (listWidgetItem != nullptr)
        m_iconReplyToListWidgetItem.insert(reply, listWidgetItem);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchAbstract::iconDownloadFinished);
    return QIcon::fromTheme(QStringLiteral("applications-internet"));
}

#ifdef HAVE_QTWIDGETS
OnlineSearchQueryFormAbstract *OnlineSearchAbstract::customWidget(QWidget *) {
    return nullptr;
}

void OnlineSearchAbstract::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    curStep = numSteps = 0;
    delayedStoppedSearch(resultNoError);
}
#endif // HAVE_QTWIDGETS

QString OnlineSearchAbstract::name()
{
    static const QRegExp invalidChars("[^-a-z0-9]", Qt::CaseInsensitive);
    if (m_name.isEmpty())
        m_name = label().remove(invalidChars);
    return m_name;
}

bool OnlineSearchAbstract::busy() const
{
    return numSteps > 0 && curStep < numSteps;
}

void OnlineSearchAbstract::cancel()
{
    m_hasBeenCanceled = true;

    curStep = numSteps = 0;
    refreshBusyProperty();
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
        stopSearch(resultCancelled);
        return false;
    } else if (reply->error() != QNetworkReply::NoError) {
        m_hasBeenCanceled = true;
        const QString errorString = reply->errorString();
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "failed (error code" << reply->error() << "(" << errorString << "), HTTP code" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << ":" << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray() << ")";
#ifdef HAVE_KF5
        sendVisualNotification(errorString.isEmpty() ? i18n("Searching '%1' failed for unknown reason.", label()) : i18n("Searching '%1' failed with error message:\n\n%2", label(), errorString), label(), QStringLiteral("kbibtex"), 7 * 1000);
#endif // HAVE_KF5

        int resultCode = resultUnspecifiedError;
        if (reply->error() == QNetworkReply::AuthenticationRequiredError || reply->error() == QNetworkReply::ProxyAuthenticationRequiredError)
            resultCode = resultAuthorizationRequired;
        else if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::TimeoutError)
            resultCode = resultNetworkError;
        stopSearch(resultCode);

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
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "on url" << reply->url().toDisplayString() << "returned no data";

    return true;
}

#ifdef HAVE_KF5
/**
 * Display a passive notification popup using the D-Bus interface.
 * Copied from KDialog with modifications.
 */
void OnlineSearchAbstract::sendVisualNotification(const QString &text, const QString &title, const QString &icon, int timeout)
{
    static const QString dbusServiceName = QStringLiteral("org.freedesktop.Notifications");
    static const QString dbusInterfaceName = QStringLiteral("org.freedesktop.Notifications");
    static const QString dbusPath = QStringLiteral("/org/freedesktop/Notifications");

    // check if service already exists on plugin instantiation
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    if (interface == nullptr || !interface->isServiceRegistered(dbusServiceName)) {
        return;
    }

    if (timeout <= 0)
        timeout = 10 * 1000;

    QDBusMessage m = QDBusMessage::createMethodCall(dbusServiceName, dbusPath, dbusInterfaceName, QStringLiteral("Notify"));
    QList<QVariant> args = QList<QVariant>() << QStringLiteral("kdialog") << 0U << icon << title << text << QStringList() << QVariantMap() << timeout;
    m.setArguments(args);

    QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
    if (replyMsg.type() == QDBusMessage::ReplyMessage) {
        if (!replyMsg.arguments().isEmpty()) {
            return;
        }
        // Not displaying any error messages as this is optional for kdialog
        // and KPassivePopup is a perfectly valid fallback.
        //else {
        //  qCDebug(LOG_KBIBTEX_NETWORKING) << "Error: received reply with no arguments.";
        //}
    } else if (replyMsg.type() == QDBusMessage::ErrorMessage) {
        //qCDebug(LOG_KBIBTEX_NETWORKING) << "Error: failed to send D-Bus message";
        //qCDebug(LOG_KBIBTEX_NETWORKING) << replyMsg;
    } else {
        //qCDebug(LOG_KBIBTEX_NETWORKING) << "Unexpected reply type";
    }
}
#endif // HAVE_KF5

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
    rawText = rawText.replace(QStringLiteral("&amp;"), QStringLiteral("&")).replace(QLatin1Char('+'), QStringLiteral(" "));
    return rawText;
}

QMap<QString, QString> OnlineSearchAbstract::formParameters(const QString &htmlText, int startPos)
{
    /// how to recognize HTML tags
    static const QString formTagEnd = QStringLiteral("</form>");
    static const QString inputTagBegin = QStringLiteral("<input ");
    static const QString selectTagBegin = QStringLiteral("<select ");
    static const QString selectTagEnd = QStringLiteral("</select>");
    static const QString optionTagBegin = QStringLiteral("<option ");
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
    int endPos = htmlText.indexOf(formTagEnd, startPos, Qt::CaseInsensitive);
    if (startPos < 0 || endPos < 0) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not locate form in text";
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
            if (inputType == QStringLiteral("hidden") || inputType == QStringLiteral("text") || inputType == QStringLiteral("submit"))
                result[inputName] = inputValue;
            else if (inputType == QStringLiteral("radio")) {
                /// must be selected
                if (htmlText.indexOf(inputIsCheckedRegExp, p) == p) {
                    result[inputName] = inputValue;
                }
            } else if (inputType == QStringLiteral("checkbox")) {
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
        const QUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirUrl.isValid()) {
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
            newReply->setObjectName(reply->objectName());
            QListWidgetItem *listWidgetItem = m_iconReplyToListWidgetItem.value(reply, nullptr);
            m_iconReplyToListWidgetItem.remove(reply);
            if (listWidgetItem != nullptr)
                m_iconReplyToListWidgetItem.insert(newReply, listWidgetItem);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAbstract::iconDownloadFinished);
            return;
        }

        const QByteArray iconData = reply->readAll();
        if (iconData.size() < 10) {
            /// Unlikely that an icon's data is less than 10 bytes,
            /// must be an error.
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Received invalid icon data from " << reply->url().toDisplayString();
            return;
        }

        QString extension;
        if (iconData[1] == 'P' && iconData[2] == 'N' && iconData[3] == 'G') {
            /// PNG files have string "PNG" at second to fourth byte
            extension = QStringLiteral(".png");
        } else if (iconData[0] == (char)0x00 && iconData[1] == (char)0x00 && iconData[2] == (char)0x01 && iconData[3] == (char)0x00) {
            /// Microsoft Icon have first two bytes always 0x0000,
            /// third and fourth byte is 0x0001 (for .ico)
            extension = QStringLiteral(".ico");
        } else if (iconData[0] == '<') {
            /// HTML or XML code
            const QString htmlCode = QString::fromUtf8(iconData);
            qCDebug(LOG_KBIBTEX_NETWORKING) << "Received XML or HTML data from " << reply->url().toDisplayString() << ": " << htmlCode.left(128);
            return;
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Favicon is of unknown format: " << reply->url().toDisplayString();
            return;
        }
        const QString filename = reply->objectName() + extension;

        QFile iconFile(filename);
        if (iconFile.open(QFile::WriteOnly)) {
            iconFile.write(iconData);
            iconFile.close();

            QListWidgetItem *listWidgetItem = m_iconReplyToListWidgetItem.value(reply, nullptr);
            if (listWidgetItem != nullptr)
                listWidgetItem->setIcon(QIcon(filename));
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not save icon data from URL" << reply->url().toDisplayString() << "to file" << filename;
            return;
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not download icon from URL " << reply->url().toDisplayString() << ": " << reply->errorString();
}

void OnlineSearchAbstract::dumpToFile(const QString &filename, const QString &text)
{
    const QString usedFilename = QDir::tempPath() + QLatin1Char('/') + filename;

    QFile f(usedFilename);
    if (f.open(QFile::WriteOnly)) {
        qCDebug(LOG_KBIBTEX_NETWORKING) << "Dumping text" << KBibTeX::squeezeText(text, 96) << "to" << usedFilename;
        QTextStream ts(&f);
        ts.setCodec("UTF-8");
        ts << text;
        f.close();
    }
}

void OnlineSearchAbstract::delayedStoppedSearch(int returnCode)
{
    m_delayedStoppedSearchReturnCode = returnCode;
    QTimer::singleShot(500, this, &OnlineSearchAbstract::delayedStoppedSearchTimer);
}

void OnlineSearchAbstract::delayedStoppedSearchTimer()
{
    emit progress(1, 1);
    stopSearch(m_delayedStoppedSearchReturnCode);
}

void OnlineSearchAbstract::sanitizeEntry(QSharedPointer<Entry> entry)
{
    if (entry.isNull()) return;

    /// Sometimes, there is no identifier, so set a random one
    if (entry->id().isEmpty())
        entry->setId(QString(QStringLiteral("entry-%1")).arg(QString::number(qrand(), 36)));

    const QString ftIssue = QStringLiteral("issue");
    if (entry->contains(ftIssue)) {
        /// ACM's Digital Library uses "issue" instead of "number" -> fix that
        Value v = entry->value(ftIssue);
        entry->remove(ftIssue);
        entry->insert(Entry::ftNumber, v);
    }

    /// If entry contains a description field but no abstract,
    /// rename description field to abstract
    const QString ftDescription = QStringLiteral("description");
    if (!entry->contains(Entry::ftAbstract) && entry->contains(ftDescription)) {
        Value v = entry->value(ftDescription);
        entry->remove(ftDescription);
        entry->insert(Entry::ftAbstract, v);
    }

    /// Remove "dblp" artifacts in abstracts and keywords
    if (entry->contains(Entry::ftAbstract)) {
        const QString abstract = PlainTextValue::text(entry->value(Entry::ftAbstract));
        if (abstract == QStringLiteral("dblp"))
            entry->remove(Entry::ftAbstract);
    }
    if (entry->contains(Entry::ftKeywords)) {
        const QString keywords = PlainTextValue::text(entry->value(Entry::ftKeywords));
        if (keywords == QStringLiteral("dblp"))
            entry->remove(Entry::ftKeywords);
    }

    if (entry->contains(Entry::ftMonth)) {
        /// Fix strigns for months: "September" -> "sep"
        const QString monthStr = PlainTextValue::text(entry->value(Entry::ftMonth));

        static const QRegExp longMonth = QRegExp(QStringLiteral("(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[a-z]*"), Qt::CaseInsensitive);
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
    entry->insert(QStringLiteral("x-fetchedfrom"), v);

    sanitizeEntry(entry);

    emit foundEntry(entry);

    return true;
}

void OnlineSearchAbstract::stopSearch(int errorCode) {
    if (errorCode == resultNoError)
        curStep = numSteps;
    else
        curStep = numSteps = 0;
    emit stoppedSearch(errorCode);
}

void OnlineSearchAbstract::refreshBusyProperty() {
    const bool newBusyState = busy();
    if (newBusyState != m_previousBusyState) {
        m_previousBusyState = newBusyState;
        emit busyChanged();
    }
}
