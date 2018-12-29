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

#include "onlinesearchabstract.h"

#include <QFileInfo>
#include <QNetworkReply>
#include <QDir>
#include <QTimer>
#include <QStandardPaths>
#include <QRegularExpression>
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

#ifdef HAVE_QTWIDGETS
QIcon OnlineSearchAbstract::icon(QListWidgetItem *listWidgetItem)
{
    static const QRegularExpression invalidChars(QStringLiteral("[^-a-z0-9_]"), QRegularExpression::CaseInsensitiveOption);
    const QString cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/favicons/");
    QDir().mkpath(cacheDirectory);
    const QString fileNameStem = cacheDirectory + QString(favIconUrl()).remove(invalidChars);
    const QStringList fileNameExtensions {QStringLiteral(".ico"), QStringLiteral(".png"), QString()};

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
    static const QRegularExpression invalidChars(QStringLiteral("[^-a-z0-9]"), QRegularExpression::CaseInsensitiveOption);
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
    /// The URL to be shown or logged shall not contain any API key
    const QUrl urlToShow = InternalNetworkAccessManager::removeApiKey(reply->url());

    newUrl = QUrl();
    if (m_hasBeenCanceled) {
        stopSearch(resultCancelled);
        return false;
    } else if (reply->error() != QNetworkReply::NoError) {
        m_hasBeenCanceled = true;
        const QString errorString = reply->errorString();
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "failed (error code" << reply->error() << "," << errorString << "), HTTP code" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << ":" << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray() << ") for URL" << urlToShow.toDisplayString();
        const QNetworkRequest &request = reply->request();
        /// Dump all HTTP headers that were sent with the original request (except for API keys)
        const QList<QByteArray> rawHeadersSent = request.rawHeaderList();
        for (const QByteArray &rawHeaderName : rawHeadersSent) {
            if (rawHeaderName.toLower().contains("apikey") || rawHeaderName.toLower().contains("api-key")) continue; ///< skip dumping header values containing an API key
            qCDebug(LOG_KBIBTEX_NETWORKING) << "SENT " << rawHeaderName << ":" << request.rawHeader(rawHeaderName);
        }
        /// Dump all HTTP headers that were received
        const QList<QByteArray> rawHeadersReceived = reply->rawHeaderList();
        for (const QByteArray &rawHeaderName : rawHeadersReceived) {
            if (rawHeaderName.toLower().contains("apikey") || rawHeaderName.toLower().contains("api-key")) continue; ///< skip dumping header values containing an API key
            qCDebug(LOG_KBIBTEX_NETWORKING) << "RECVD " << rawHeaderName << ":" << reply->rawHeader(rawHeaderName);
        }
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
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "on url" << urlToShow.toDisplayString() << "returned no data";

    return true;
}

QString OnlineSearchAbstract::htmlAttribute(const QString &htmlCode, const int startPos, const QString &attribute) const
{
    const int endPos = htmlCode.indexOf(QLatin1Char('>'), startPos);
    if (endPos < 0) return QString(); ///< no closing angle bracket found

    const QString attributePattern = QString(QStringLiteral(" %1=")).arg(attribute);
    const int attributePatternPos = htmlCode.indexOf(attributePattern, startPos, Qt::CaseInsensitive);
    if (attributePatternPos < 0 || attributePatternPos > endPos) return QString(); ///< attribute not found within limits

    const int attributePatternLen = attributePattern.length();
    const int openingQuotationMarkPos = attributePatternPos + attributePatternLen;
    const QChar quotationMark = htmlCode[openingQuotationMarkPos];
    if (quotationMark != QLatin1Char('"') && quotationMark != QLatin1Char('\'')) {
        /// No valid opening quotation mark found
        int spacePos = openingQuotationMarkPos;
        while (spacePos < endPos && !htmlCode[spacePos].isSpace()) ++spacePos;
        if (spacePos > endPos) return QString(); ///< no closing space found
        return htmlCode.mid(openingQuotationMarkPos, spacePos - openingQuotationMarkPos);
    } else {
        /// Attribute has either single or double quotation marks
        const int closingQuotationMarkPos = htmlCode.indexOf(quotationMark, openingQuotationMarkPos + 1);
        if (closingQuotationMarkPos < 0 || closingQuotationMarkPos > endPos) return QString(); ///< closing quotation mark not found within limits
        return htmlCode.mid(openingQuotationMarkPos + 1, closingQuotationMarkPos - openingQuotationMarkPos - 1);
    }
}

bool OnlineSearchAbstract::htmlAttributeIsSelected(const QString &htmlCode, const int startPos, const QString &attribute) const
{
    const int endPos = htmlCode.indexOf(QLatin1Char('>'), startPos);
    if (endPos < 0) return false; ///< no closing angle bracket found

    const QString attributePattern = QStringLiteral(" ") + attribute;
    const int attributePatternPos = htmlCode.indexOf(attributePattern, startPos, Qt::CaseInsensitive);
    if (attributePatternPos < 0 || attributePatternPos > endPos) return false; ///< attribute not found within limits

    const int attributePatternLen = attributePattern.length();
    const QChar nextAfterAttributePattern = htmlCode[attributePatternPos + attributePatternLen];
    if (nextAfterAttributePattern.isSpace() || nextAfterAttributePattern == QLatin1Char('>') || nextAfterAttributePattern == QLatin1Char('/'))
        /// No value given for attribute (old-style HTML), so assuming it means checked/selected
        return true;
    else if (nextAfterAttributePattern == QLatin1Char('=')) {
        /// Expecting value to attribute, so retrieve it and check for 'selected' or 'checked'
        const QString attributeValue = htmlAttribute(htmlCode, attributePatternPos, attribute).toLower();
        return attributeValue == QStringLiteral("selected") || attributeValue == QStringLiteral("checked");
    }

    /// Reaching this point only if HTML code is invalid
    return false;
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
    const QList<QVariant> args {QStringLiteral("kdialog"), 0U, icon, title, text, QStringList(), QVariantMap(), timeout};
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
    static const QRegularExpression mimeRegExp(QStringLiteral("%([0-9A-Fa-f]{2})"));
    QRegularExpressionMatch mimeRegExpMatch;
    while ((mimeRegExpMatch = mimeRegExp.match(rawText)).hasMatch()) {
        bool ok = false;
        QChar c(mimeRegExpMatch.captured(1).toInt(&ok, 16));
        if (ok)
            rawText = rawText.replace(mimeRegExpMatch.captured(0), c);
    }
    rawText = rawText.replace(QStringLiteral("&amp;"), QStringLiteral("&")).replace(QLatin1Char('+'), QStringLiteral(" "));
    return rawText;
}

QMap<QString, QString> OnlineSearchAbstract::formParameters(const QString &htmlText, int startPos) const
{
    /// how to recognize HTML tags
    static const QString formTagEnd = QStringLiteral("</form>");
    static const QString inputTagBegin = QStringLiteral("<input ");
    static const QString selectTagBegin = QStringLiteral("<select ");
    static const QString selectTagEnd = QStringLiteral("</select>");
    static const QString optionTagBegin = QStringLiteral("<option ");

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
        const QString inputType = htmlAttribute(htmlText, p, QStringLiteral("type")).toLower();
        const QString inputName = htmlAttribute(htmlText, p, QStringLiteral("name"));
        const QString inputValue = htmlAttribute(htmlText, p, QStringLiteral("value"));

        if (!inputName.isEmpty()) {
            /// get value of input types
            if (inputType == QStringLiteral("hidden") || inputType == QStringLiteral("text") || inputType == QStringLiteral("submit"))
                result[inputName] = inputValue;
            else if (inputType == QStringLiteral("radio")) {
                /// must be selected
                if (htmlAttributeIsSelected(htmlText, p, QStringLiteral("checked"))) {
                    result[inputName] = inputValue;
                }
            } else if (inputType == QStringLiteral("checkbox")) {
                /// must be checked
                if (htmlAttributeIsSelected(htmlText, p, QStringLiteral("checked"))) {
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
        const QString selectName = htmlAttribute(htmlText, p, QStringLiteral("name"));

        /// "select" tag contains one or several "option" tags, search all
        int popt = htmlText.indexOf(optionTagBegin, p, Qt::CaseInsensitive);
        int endSelect = htmlText.indexOf(selectTagEnd, p, Qt::CaseInsensitive);
        while (popt > p && popt < endSelect) {
            /// get "value" attribute from "option" tag
            const QString optionValue =  htmlAttribute(htmlText, popt, QStringLiteral("value"));
            if (!selectName.isEmpty() && !optionValue.isEmpty()) {
                /// if this "option" tag is "selected", store value
                if (htmlAttributeIsSelected(htmlText, popt, QStringLiteral("selected"))) {
                    result[selectName] = optionValue;
                }
            }

            popt = htmlText.indexOf(optionTagBegin, popt + 1, Qt::CaseInsensitive);
        }

        p = htmlText.indexOf(selectTagBegin, p + 1, Qt::CaseInsensitive);
    }

    return result;
}

#ifdef HAVE_QTWIDGETS
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
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Received invalid icon data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
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
            qCDebug(LOG_KBIBTEX_NETWORKING) << "Received XML or HTML data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString() << ": " << htmlCode.left(128);
            return;
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Favicon is of unknown format: " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
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
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not save icon data from URL" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString() << "to file" << filename;
            return;
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not download icon from URL " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString() << ": " << reply->errorString();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchAbstract::dumpToFile(const QString &filename, const QString &text)
{
    const QString usedFilename = QDir::tempPath() + QLatin1Char('/') + filename;

    QFile f(usedFilename);
    if (f.open(QFile::WriteOnly)) {
        qCDebug(LOG_KBIBTEX_NETWORKING) << "Dumping text" << KBibTeX::squeezeText(text, 96) << "to" << usedFilename;
        f.write(text.toUtf8());
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
    stopSearch(m_delayedStoppedSearchReturnCode);
}

void OnlineSearchAbstract::sanitizeEntry(QSharedPointer<Entry> entry)
{
    if (entry.isNull()) return;

    /// Sometimes, there is no identifier, so set a random one
    if (entry->id().isEmpty())
        entry->setId(QString(QStringLiteral("entry-%1")).arg(QString::number(qrand(), 36)));
    /// Missing entry type? Set it to 'misc'
    if (entry->type().isEmpty())
        entry->setType(Entry::etMisc);

    static const QString ftIssue = QStringLiteral("issue");
    if (entry->contains(ftIssue)) {
        /// ACM's Digital Library uses "issue" instead of "number" -> fix that
        Value v = entry->value(ftIssue);
        entry->remove(ftIssue);
        entry->insert(Entry::ftNumber, v);
    }

    /// If entry contains a description field but no abstract,
    /// rename description field to abstract
    static const QString ftDescription = QStringLiteral("description");
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
        /// Fix strings for months: "September" -> "sep"
        Value monthValue = entry->value(Entry::ftMonth);
        bool updated = false;
        for (Value::Iterator it = monthValue.begin(); it != monthValue.end(); ++it) {
            const QString valueItem = PlainTextValue::text(*it);

            static const QRegularExpression longMonth = QRegularExpression(QStringLiteral("(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[a-z]*"), QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch longMonthMatch = longMonth.match(valueItem);
            if (longMonthMatch.hasMatch()) {
                it = monthValue.erase(it);
                it = monthValue.insert(it, QSharedPointer<MacroKey>(new MacroKey(longMonthMatch.captured(1).toLower())));
                updated = true;
            }
        }
        if (updated)
            entry->insert(Entry::ftMonth, monthValue);
    }

    if (entry->contains(Entry::ftDOI) && entry->contains(Entry::ftUrl)) {
        /// Remove URL from entry if contains a DOI and the DOI field matches the DOI in the URL
        const Value &doiValue = entry->value(Entry::ftDOI);
        for (const auto &doiValueItem : doiValue) {
            const QString doi = PlainTextValue::text(doiValueItem);
            Value v = entry->value(Entry::ftUrl);
            bool gotChanged = false;
            for (Value::Iterator it = v.begin(); it != v.end();) {
                const QSharedPointer<ValueItem> &vi = (*it);
                if (vi->containsPattern(QStringLiteral("/") + doi)) {
                    it = v.erase(it);
                    gotChanged = true;
                } else
                    ++it;
            }
            if (v.isEmpty())
                entry->remove(Entry::ftUrl);
            else if (gotChanged)
                entry->insert(Entry::ftUrl, v);
        }
    } else if (!entry->contains(Entry::ftDOI) && entry->contains(Entry::ftUrl)) {
        /// If URL looks like a DOI, remove URL and add a DOI field
        QSet<QString> doiSet;
        Value v = entry->value(Entry::ftUrl);
        bool gotChanged = false;
        for (Value::Iterator it = v.begin(); it != v.end();) {
            const QString viText = PlainTextValue::text(*it);
            const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(viText);
            if (doiRegExpMatch.hasMatch()) {
                doiSet.insert(doiRegExpMatch.captured());
                it = v.erase(it);
                gotChanged = true;
            } else
                ++it;
        }
        if (v.isEmpty())
            entry->remove(Entry::ftUrl);
        else if (gotChanged)
            entry->insert(Entry::ftUrl, v);
        if (!doiSet.isEmpty()) {
            Value doiValue;
            for (const QString &doi : doiSet)
                doiValue.append(QSharedPointer<PlainText>(new PlainText(doi)));
            entry->insert(Entry::ftDOI, doiValue);
        }
    } else if (!entry->contains(Entry::ftDOI)) {
        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(entry->id());
        if (doiRegExpMatch.hasMatch()) {
            /// If entry id looks like a DOI, add a DOI field
            Value doiValue;
            doiValue.append(QSharedPointer<PlainText>(new PlainText(doiRegExpMatch.captured())));
            entry->insert(Entry::ftDOI, doiValue);
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
    v.append(QSharedPointer<PlainText>(new PlainText(label())));
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
    emit progress(curStep, numSteps);
    emit stoppedSearch(errorCode);
}

void OnlineSearchAbstract::refreshBusyProperty() {
    const bool newBusyState = busy();
    if (newBusyState != m_previousBusyState) {
        m_previousBusyState = newBusyState;
        emit busyChanged();
    }
}
