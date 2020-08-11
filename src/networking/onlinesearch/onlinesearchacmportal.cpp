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

#include "onlinesearchacmportal.h"

#include <QBuffer>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#ifdef HAVE_KF5
#include <KLocalizedString>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#include <File>
#include <Entry>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchAcmPortal::OnlineSearchAcmPortalPrivate
{
public:
    QString joinedQueryString;
    int numExpectedResults, numFoundResults;
    const QString acmPortalBaseUrl;
    int currentSearchPosition;
    static const int requestedResultsPerPage;

    OnlineSearchAcmPortalPrivate(OnlineSearchAcmPortal *)
            : numExpectedResults(0), numFoundResults(0),
          acmPortalBaseUrl(QStringLiteral("https://dl.acm.org/")), currentSearchPosition(0) {
        /// nothing
    }

    Entry *entryFromJsonObject(const QJsonObject &object) const {
        QJsonDocument doc(object);

        /// Assemble information which key-value pairs are contained in JSON object
        /// but not evalued in this code. Debug output reported here should be send
        /// to KBibTeX's authors along with information which ACM records were search
        /// for or actually found.
        static const QSet<QString> evaluatedOrIgnoredValues{QStringLiteral("id"), QStringLiteral("call-number"), QStringLiteral("DOI"), QStringLiteral("ISBN"), QStringLiteral("ISSN"), QStringLiteral("URL"), QStringLiteral("type"), QStringLiteral("author"), QStringLiteral("issued"), QStringLiteral("original-date"), QStringLiteral("number-of-pages"), QStringLiteral("page"), QStringLiteral("number"), QStringLiteral("issue"), QStringLiteral("volume"), QStringLiteral("publisher"), QStringLiteral("publisher-place"), QStringLiteral("title"), QStringLiteral("collection-title"), QStringLiteral("container-title"), QStringLiteral("event-place"), QStringLiteral("keyword"), QStringLiteral("accessed")};
        QSet<QString> objectsValues;
        for (const QString &objectValue : object.keys()) objectsValues.insert(objectValue);
        const QSet<QString> unusedValues = objectsValues - evaluatedOrIgnoredValues;
        for (const QString &unusedValue : unusedValues) {
            const QString valueString = object.value(unusedValue).toString();
            if (!valueString.isEmpty())
                qDebug(LOG_KBIBTEX_NETWORKING) << "unused value:" << unusedValue << "=" << valueString;
        }

        QString doi = object.value(QStringLiteral("DOI")).toString();
        const QString isbn = object.value(QStringLiteral("ISBN")).toString();
        const QString id = object.value(QStringLiteral("id")).toString();
        if (doi.isEmpty() && !id.isEmpty()) {
            /// Sometimes the 'DOI' field is empty, but there is a DOI in the 'id' field
            const QRegularExpressionMatch doiInIdMatch = KBibTeX::doiRegExp.match(id);
            if (doiInIdMatch.hasMatch())
                doi = doiInIdMatch.captured();
        }
        QString entryIdSerial = !doi.isEmpty() ? doi : (!isbn.isEmpty() ? isbn : (!id.isEmpty() ? id : object.value(QStringLiteral("URL")).toString().remove(QStringLiteral("http://")).remove(QStringLiteral("https://"))));
        const QString entryId = QStringLiteral("ACM-") + entryIdSerial.remove(QRegularExpression(QStringLiteral("[^0-9./-]")));

        QString entryType = Entry::etMisc;
        const QString type = object.value(QStringLiteral("type")).toString().toUpper();
        if (type == QStringLiteral("PAPER_CONFERENCE"))
            entryType = Entry::etInProceedings;
        else if (type == QStringLiteral("ARTICLE_JOURNAL"))
            entryType = Entry::etArticle;
        else if (type == QStringLiteral("CHAPTER"))
            entryType = Entry::etInBook;
        Entry *entry = new Entry(entryType, entryId);

        Value authors;
        const QJsonArray authorsArray = object.value(QStringLiteral("author")).toArray();
        for (const QJsonValue &authorValue : authorsArray) {
            const QJsonObject authorObject = authorValue.toObject();
            Person *author = new Person(authorObject.value(QStringLiteral("given")).toString(), authorObject.value(QStringLiteral("family")).toString());
            authors.append(QSharedPointer<ValueItem>(author));
        }
        entry->insert(Entry::ftAuthor, authors);

        const int year = object.value(QStringLiteral("issued")).toObject().value(QStringLiteral("date-parts")).toArray().constBegin()->toArray().constBegin()->toInt(-1);
        if (year >= 1700 && year < 2100) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(QString::number(year))));
            entry->insert(Entry::ftYear, v);

            const int month = object.value(QStringLiteral("issued")).toObject().value(QStringLiteral("date-parts")).toArray().constBegin()->toArray().at(1).toInt(-1);
            if (month >= 1 && month <= 12) {
                Value v;
                v.append(QSharedPointer<ValueItem>(new MacroKey(KBibTeX::MonthsTriple[month - 1])));
                entry->insert(Entry::ftMonth, v);
            }
        }

        const QString numpagesStr = object.value(QStringLiteral("number-of-pages")).toString();
        bool numpagesOk = false;
        const int numpages = numpagesStr.toInt(&numpagesOk);
        if (numpagesOk && numpages > 0) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(QString::number(numpages))));
            entry->insert(QStringLiteral("numpages"), v);
        }

        const QString page = object.value(QStringLiteral("page")).toString();
        if (!page.isEmpty()) {
            static const QRegularExpression fromPageRegExp(QStringLiteral("^[1-9][0-9]*")), toPageRegExp(QStringLiteral("[1-9][0-9]*$"));
            const QRegularExpressionMatch fromPageRegExpMatch = fromPageRegExp.match(page), toPageRegExpMatch = toPageRegExp.match(page);
            bool okFromPage = false, okToPage = false;
            const int fromPage = fromPageRegExpMatch.hasMatch() ? fromPageRegExpMatch.captured().toInt(&okFromPage) : -1;
            const int toPage = toPageRegExpMatch.hasMatch() ? toPageRegExpMatch.captured().toInt(&okToPage) : -1;
            if (fromPage > 0 && okFromPage && (!okToPage || toPage <= 0 || (okToPage && toPage == fromPage))) {
                /// Single page number
                Value v;
                v.append(QSharedPointer<ValueItem>(new PlainText(QString::number(fromPage))));
                entry->insert(Entry::ftPages, v);
            } else if (fromPage > 0 && okFromPage && toPage > 0 && okToPage && fromPage < toPage) {
                /// Two page numbers
                Value v;
                v.append(QSharedPointer<ValueItem>(new PlainText(QString(QStringLiteral("%1%2%3")).arg(fromPage).arg(QChar(0x2013)).arg(toPage))));
                entry->insert(Entry::ftPages, v);
            }
        }

        if (!doi.isEmpty()) {
            Value vDoi;
            vDoi.append(QSharedPointer<ValueItem>(new PlainText(doi)));
            entry->insert(Entry::ftDOI, vDoi);
            Value vUrl;
            vUrl.append(QSharedPointer<ValueItem>(new VerbatimText(QStringLiteral("https://doi.org/") + doi)));
            entry->insert(Entry::ftDOI, vDoi);
        }

        const QString issn = object.value(QStringLiteral("ISSN")).toString();
        if (!issn.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(issn)));
            entry->insert(Entry::ftISSN, v);
        }
        if (!isbn.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(isbn)));
            entry->insert(Entry::ftISBN, v);
        }
        const QString number = object.value(QStringLiteral("number")).toString();
        if (!number.isEmpty() && number.startsWith(QStringLiteral("Article "))) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(number.midRef(8).trimmed().toString())));
            entry->insert(QStringLiteral("articleno"), v);
        }
        const QString issue = object.value(QStringLiteral("issue")).toString();
        if (!issue.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(issue)));
            entry->insert(Entry::ftNumber, v);
        }
        const QString volume = object.value(QStringLiteral("volume")).toString();
        if (!volume.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(volume)));
            entry->insert(Entry::ftVolume, v);
        }
        const QString publisher = object.value(QStringLiteral("publisher")).toString();
        if (!publisher.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(publisher)));
            entry->insert(Entry::ftPublisher, v);
        }
        const QString publisherPlace = object.value(QStringLiteral("publisher-place")).toString();
        if (!publisherPlace.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(publisherPlace)));
            entry->insert(Entry::ftAddress, v);
        }

        const QString title = object.value(QStringLiteral("title")).toString();
        if (!title.isEmpty()) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(title)));
            entry->insert(Entry::ftTitle, v);
        }

        const QString collectionTitle = object.value(QStringLiteral("collection-title")).toString();
        if (!collectionTitle.isEmpty() && entryType == Entry::etInProceedings) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(collectionTitle)));
            entry->insert(Entry::ftSeries, v);
        }

        const QString containerTitle = object.value(QStringLiteral("container-title")).toString();
        if (!containerTitle.isEmpty() && entryType == Entry::etArticle) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(containerTitle)));
            entry->insert(Entry::ftJournal, v);
        } else if (!containerTitle.isEmpty() && (entryType == Entry::etInProceedings || entryType == Entry::etInBook)) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(containerTitle)));
            entry->insert(Entry::ftBookTitle, v);
        }

        const QString eventPlace = object.value(QStringLiteral("event-place")).toString();
        if (!eventPlace.isEmpty() && entryType == Entry::etInProceedings) {
            Value v;
            v.append(QSharedPointer<ValueItem>(new PlainText(eventPlace)));
            entry->insert(Entry::ftLocation, v);
        }

        const QString keywords = object.value(QStringLiteral("keyword")).toString();
        if (!keywords.isEmpty()) {
            const QStringList keywordList = keywords.split(QRegularExpression(QStringLiteral("\\s*,\\s*")));
            Value v;
            for (const QString &keyword : keywordList)
                v.append(QSharedPointer<ValueItem>(new Keyword(keyword)));
            entry->insert(Entry::ftKeywords, v);
        }

        return entry;
    }

    void sanitizeBibTeXCode(QString &code) {
        static const QRegularExpression htmlEncodedChar(QStringLiteral("&#(\\d+);"));
        QRegularExpressionMatch match;
        while ((match = htmlEncodedChar.match(code)).hasMatch()) {
            bool ok = false;
            QChar c(match.captured(1).toInt(&ok));
            if (ok)
                code = code.replace(match.captured(0), c);
        }

        /// ACM's BibTeX code does not properly use various commands.
        /// For example, ACM writes "Ke\ssler" instead of "Ke{\ss}ler"
        static const QStringList inlineCommands {QStringLiteral("\\ss")};
        for (const QString &cmd : inlineCommands) {
            const QString wrappedCmd = QString(QStringLiteral("{%1}")).arg(cmd);
            code = code.replace(cmd, wrappedCmd);
        }
    }
};

const int OnlineSearchAcmPortal::OnlineSearchAcmPortalPrivate::requestedResultsPerPage = 50; ///< either 10, 20, or 50

OnlineSearchAcmPortal::OnlineSearchAcmPortal(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchAcmPortalPrivate(this))
{
    /// nothing
}

OnlineSearchAcmPortal::~OnlineSearchAcmPortal()
{
    delete d;
}

void OnlineSearchAcmPortal::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->joinedQueryString.clear();
    d->currentSearchPosition = 1;
    d->numFoundResults = 0;
    emit progress(curStep = 0, numSteps = numResults / OnlineSearchAcmPortalPrivate::requestedResultsPerPage + 3);

    for (QMap<QueryKey, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->joinedQueryString.append(it.value() + QStringLiteral(" "));
    }
    d->numExpectedResults = numResults;

    QNetworkRequest request(d->acmPortalBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingStartPage);

    refreshBusyProperty();
}

QString OnlineSearchAcmPortal::label() const
{
#ifdef HAVE_KF5
    return i18n("ACM Digital Library");
#else // HAVE_KF5
    //= onlinesearch-acmdigitallibrary-label
    return QObject::tr("ACM Digital Library");
#endif // HAVE_KF5
}

QUrl OnlineSearchAcmPortal::homepage() const
{
    return QUrl(QStringLiteral("https://dl.acm.org/"));
}

void OnlineSearchAcmPortal::doneFetchingStartPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// Redirection to another url
            ++numSteps;
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingStartPage);
        } else {
            const QString htmlSource = QString::fromUtf8(reply->readAll().constData());
            int p1 = -1, p2 = -1, p3 = -1;
            if ((p1 = htmlSource.indexOf(QStringLiteral(" name=\"defaultQuickSearch\""))) >= 0
                    && (p2 = htmlSource.indexOf(QStringLiteral("action="), p1 - 64)) >= 0
                    && (p3 = htmlSource.indexOf(QStringLiteral("\""), p2 + 8)) >= 0) {
                const QString body = QString(QStringLiteral("AllField=%1")).arg(d->joinedQueryString).simplified();
                const QString action = decodeURL(htmlSource.mid(p2 + 8, p3 - p2 - 8));
                const QUrl url(reply->url().resolved(QUrl(QString(QStringLiteral("%1?pageSize=%2&%3")).arg(action).arg(OnlineSearchAcmPortalPrivate::requestedResultsPerPage).arg(body))));

                QNetworkRequest request(url);
                QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
                InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
                connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingSearchPage);
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "failed.";
                stopSearch(resultUnspecifiedError);
            }
        }
    }

    refreshBusyProperty();
}

void OnlineSearchAcmPortal::doneFetchingSearchPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlSource = QString::fromUtf8(reply->readAll().constData());

        QVector<QString> dois;
        static const QRegularExpression citationUrlRegExp(QStringLiteral("/doi(/abs)?/(") + KBibTeX::doiRegExp.pattern() + QStringLiteral(")"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator citationUrlRegExpMatchIt = citationUrlRegExp.globalMatch(htmlSource);
        while (citationUrlRegExpMatchIt.hasNext()) {
            const QRegularExpressionMatch citationUrlRegExpMatch = citationUrlRegExpMatchIt.next();
            dois.append(citationUrlRegExpMatch.captured(2));
            if (d->currentSearchPosition + dois.length() > d->numExpectedResults)
                break;
        }

        bool stillBusy = false;
        if (d->currentSearchPosition + OnlineSearchAcmPortalPrivate::requestedResultsPerPage < d->numExpectedResults) {
            ++numSteps;
            d->currentSearchPosition += OnlineSearchAcmPortalPrivate::requestedResultsPerPage;
            QUrl url(reply->url());
            QUrlQuery query(url);
            query.removeQueryItem(QStringLiteral("start"));
            query.addQueryItem(QStringLiteral("start"), QString::number(d->currentSearchPosition));
            url.setQuery(query);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingSearchPage);
            stillBusy = true;
        }
        if (!dois.isEmpty()) {
            QNetworkRequest request(QUrl(QStringLiteral("https://dl.acm.org/action/exportCiteProcCitation")));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            QByteArray postBody = QByteArray("dois=");
            bool first = true;
            for (const QString &doi : dois) {
                if (first)
                    first = false;
                else
                    postBody.append(',');
                postBody.append(doi.toUtf8());
            }
            postBody.append("&targetFile=custom-bibtex&format=bibTex");
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().post(request, postBody);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingJSON);
            stillBusy = true;
        }
        if (!stillBusy)
            stopSearch(resultNoError);
    }

    refreshBusyProperty();
}

void OnlineSearchAcmPortal::doneFetchingJSON()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            if (document.isObject()) {
                const QString contentType = document.object().value(QStringLiteral("contentType")).toString().toLower();
                if (contentType == QStringLiteral("application/x-bibtex")) {
                    const QJsonValue itemsArrayValue = document.object().value(QStringLiteral("items"));
                    if (itemsArrayValue.isArray()) {
                        const QJsonArray itemsArray = itemsArrayValue.toArray();
                        bool encounteredUnexpectedData = false;
                        for (const QJsonValue &itemValue : itemsArray) {
                            if (itemValue.isObject()) {
                                const QJsonObject itemObject = itemValue.toObject();
                                if (itemObject.count() == 1) {
                                    if (itemObject.constBegin().value().isObject()) {
                                        Entry *entry = d->entryFromJsonObject(itemObject.constBegin().value().toObject());
                                        if (entry != nullptr) {
                                            if (publishEntry(QSharedPointer<Entry>(entry)))
                                                ++d->numFoundResults;
                                        } else {
                                            qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: Data could not be interpreted as a bibliographic entry";
                                            encounteredUnexpectedData = true;
                                            break;
                                        }
                                    } else {
                                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: Value is not an object";
                                        encounteredUnexpectedData = true;
                                        break;
                                    }
                                } else {
                                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: Object in items does not have exactly one key-value pair";
                                    encounteredUnexpectedData = true;
                                    break;
                                }
                            } else {
                                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: No object found in 'items' array where expected";
                                encounteredUnexpectedData = true;
                                break;
                            }
                        }
                        if (encounteredUnexpectedData)
                            stopSearch(resultUnspecifiedError);
                        else
                            stopSearch(resultNoError);
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: No 'items' array found";
                        stopSearch(resultUnspecifiedError);
                    }
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: Unexpected content type:" << contentType;
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: Document is not an object";
                stopSearch(resultUnspecifiedError);
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ACM Digital Library: " << parseError.errorString();
            stopSearch(resultUnspecifiedError);
        }

        if (d->numFoundResults >= d->numExpectedResults)
            stopSearch(resultNoError);
    }

    refreshBusyProperty();
}
