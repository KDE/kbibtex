/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchsciencedirect.h"

#include <QNetworkReply>
#include <QUrlQuery>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#ifdef HAVE_KF
#include <KLocalizedString>
#else // HAVE_KF
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF

#include <KBibTeX>
#include <FileImporterBibTeX>
#include <EncoderXML>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate
{
private:
    OnlineSearchScienceDirect *parent;

public:
    static const QUrl apiUrl;
    static const QString apiKeys[];
    static const int apiKeysCount;
    static int apiKeyIndex;

    OnlineSearchScienceDirectPrivate(OnlineSearchScienceDirect *_parent)
            : parent(_parent)
    {
        /// nothing
    }

    int normalizeNumberOfResults(int requestedNumResults) const {
        if (requestedNumResults <= 10) return 10;
        else if (requestedNumResults <= 25) return 25;
        else if (requestedNumResults <= 50) return 50;
        else return 100;
    }

    QByteArray buildJsonQuery(const QMap<QueryKey, QString> &query, int numResults) const {
        QString jsonQueryText = QStringLiteral("{\n");

        /// Free text
        const QStringList freeTextFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::FreeText]);
        if (!freeTextFragments.isEmpty())
            jsonQueryText.append(QStringLiteral("  \"qs\": \"\\\"") + freeTextFragments.join(QStringLiteral("\\\" AND \\\"")) + QStringLiteral("\\\"\""));

        /// Title
        const QStringList title = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Title]);
        if (!title.isEmpty()) {
            if (jsonQueryText != QStringLiteral("{\n")) jsonQueryText.append(QStringLiteral(",\n"));
            jsonQueryText.append(QStringLiteral("  \"title\": \"\\\"") + title.join(QStringLiteral("\\\" AND \\\"")) + QStringLiteral("\\\"\""));
        }

        /// Authors
        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Author]);
        if (!authors.isEmpty()) {
            if (jsonQueryText != QStringLiteral("{\n")) jsonQueryText.append(QStringLiteral(",\n"));
            jsonQueryText.append(QStringLiteral("  \"authors\": \"\\\"") + authors.join(QStringLiteral("\\\" AND \\\"")) + QStringLiteral("\\\"\""));
        }

        /// Year
        static const QRegularExpression yearRangeRegExp(QStringLiteral("(18|19|20)[0-9]{2}(-+(18|19|20)[0-9]{2})?"));
        const QRegularExpressionMatch yearRangeRegExpMatch = yearRangeRegExp.match(query[QueryKey::Year]);
        if (yearRangeRegExpMatch.hasMatch()) {
            if (jsonQueryText != QStringLiteral("{\n")) jsonQueryText.append(QStringLiteral(",\n"));
            jsonQueryText.append(QStringLiteral("  \"date\": \"") + yearRangeRegExpMatch.captured() + QStringLiteral("\""));
        }

        /// Request numResults many entries
        if (jsonQueryText != QStringLiteral("{\n")) jsonQueryText.append(QStringLiteral(",\n"));
        jsonQueryText.append(QStringLiteral("  \"display\": {\n    \"show\": ") + QString::number(normalizeNumberOfResults(numResults)) + QStringLiteral("\n  }"));

        jsonQueryText.append(QStringLiteral("\n}\n"));

        return jsonQueryText.toUtf8();
    }

    Entry *entryFromJsonObject(const QJsonObject &object) const {
        const QString title = object.value(QStringLiteral("title")).toString();
        const QString pii = object.value(QStringLiteral("pii")).toString();
        /// Basic sanity check
        if (title.isEmpty() || pii.isEmpty())
            return nullptr;

        Entry *entry = new Entry(Entry::etArticle, QStringLiteral("ScienceDirect:") + pii);
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(title)));
        entry->insert(QStringLiteral("pii"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(pii)));

        const QString doi = object.value(QStringLiteral("doi")).toString();
        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(doi);
        if (doiRegExpMatch.hasMatch())
            entry->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured(QStringLiteral("doi")))));

        const QString url = object.value(QStringLiteral("uri")).toString().remove(QStringLiteral("?dgcid=api_sd_search-api-endpoint"));
        if (!url.isEmpty())
            entry->insert(Entry::ftUrl, Value() << QSharedPointer<VerbatimText>(new VerbatimText(url)));

        const QJsonObject pages = object.value(QStringLiteral("pages")).toObject();
        bool firstPageOk = false, lastPageOk = false;;
        const int firstPage = pages.value(QStringLiteral("first")).toString().toInt(&firstPageOk);
        const int lastPage = firstPageOk ? pages.value(QStringLiteral("last")).toString().toInt(&lastPageOk) : -1;
        if (firstPageOk && lastPageOk && firstPage <= lastPage)
            entry->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QString(QStringLiteral("%1%2%3")).arg(firstPage).arg(QChar(0x2013)).arg(lastPage))));

        static const QRegularExpression dateRegExp(QStringLiteral("^((17|18|19|20)[0-9]{2})(-(0[1-9]|1[012]))?"));
        const QString publicationDate = object.value(QStringLiteral("publicationDate")).toString();
        const QRegularExpressionMatch dateRegExpMatch = dateRegExp.match(publicationDate);
        if (dateRegExpMatch.hasMatch()) {
            entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(dateRegExpMatch.captured(1))));
            bool monthOk = false;
            const int month = dateRegExpMatch.captured(4).toInt(&monthOk);
            if (monthOk && month >= 1 && month <= 12)
                entry->insert(Entry::ftMonth, Value() << QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[month - 1])));
        }

        const QJsonArray authorArray = object.value(QStringLiteral("authors")).toArray();
        QMap<int, QString> authorMap;
        int maxOrder = -1;
        for (const QJsonValue &author : authorArray) {
            const QString name = author.toObject().value(QStringLiteral("name")).toString();
            const int order = author.toObject().value(QStringLiteral("order")).toInt(-1);
            if (order >= 0 && !name.isEmpty()) {
                if (order > maxOrder) maxOrder = order;
                authorMap.insert(order, name);
            }
        }
        Value authors;
        for (int i = 0; i <= maxOrder; ++i) {
            QStringList components = authorMap.value(i, QString()).split(QStringLiteral(" "));
            if (components.isEmpty()) continue;
            const QString lastName = components.last();
            components.pop_back();
            const QString firstName = components.join(QStringLiteral(" "));
            authors.append(QSharedPointer<Person>(new Person(firstName, lastName)));
        }
        if (!authors.isEmpty())
            entry->insert(Entry::ftAuthor, authors);

        const QString sourceTitle = object.value(QStringLiteral("sourceTitle")).toString();
        if (!sourceTitle.isEmpty())
            entry->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(sourceTitle)));

        const QString volumeIssue = object.value(QStringLiteral("volumeIssue")).toString();
        static const QRegularExpression volumeRegExp(QStringLiteral("olume\\s+([1-9][0-9]*)"));
        const QRegularExpressionMatch volumeRegExpMatch = volumeRegExp.match(volumeIssue);
        if (volumeRegExpMatch.hasMatch())
            entry->insert(Entry::ftVolume, Value() << QSharedPointer<PlainText>(new PlainText(volumeRegExpMatch.captured(1))));
        static const QRegularExpression issueRegExp(QStringLiteral("ssue\\s+([1-9][0-9]*)"));
        const QRegularExpressionMatch issueRegExpMatch = issueRegExp.match(volumeIssue);
        if (issueRegExpMatch.hasMatch())
            entry->insert(Entry::ftNumber, Value() << QSharedPointer<PlainText>(new PlainText(issueRegExpMatch.captured(1))));

        return entry;
    }

    bool startSearch(const QMap<QueryKey, QString> &query = QMap<QueryKey, QString>(), int numResults = -1) {
        static QMap<QueryKey, QString> previousQuery;
        static int previousNumResults = -1;
        if (apiKeyIndex < 0 || apiKeyIndex >= apiKeysCount)
            return false;
        else if ((query.count() == 0 || numResults < 1) && (previousQuery.count() == 0 || previousNumResults < 1))
            return false;
        else if (query.count() > 0 && numResults > 0) {
            previousQuery = query;
            previousNumResults = numResults;
        }

        const int numSteps{apiKeysCount - apiKeyIndex};
        Q_EMIT parent->progress(numSteps - 1, numSteps);

        Q_ASSERT(apiKeys[apiKeyIndex].length() == 32);
        QUrl u(apiUrl);
        QNetworkRequest request(u);
        request.setRawHeader(QByteArray("X-ELS-APIKey"), apiKeys[apiKeyIndex].toLatin1());
        request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));

        const QByteArray jsonData = buildJsonQuery(previousQuery, previousNumResults);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().put(request, jsonData);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, parent, &OnlineSearchScienceDirect::doneFetchingJSON);

        parent->refreshBusyProperty();

        return true;
    }
};

const QUrl OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiUrl(QStringLiteral("https://api.elsevier.com/content/search/sciencedirect"));
const QString OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiKeys[] = {
    InternalNetworkAccessManager::reverseObfuscate("\xd1\xb0\x9f\xa9\xd9\xeb\xf9\xcd\x73\x4a\x91\xf7\x65\x07\xd7\xe6\x95\xf6\xb6\x86\x37\x01\x7b\x1f\x60\x06\x45\x74\x82\xe3\x44\x7c\x65\x52\x34\x52\xfe\xc8\x1c\x24\x44\x20\xd4\xe6\x7e\x1a\x0f\x3e\xc6\xf6\xdb\xe2\x8d\xeb\x50\x31\xfd\xc4\xc8\xfd\x9b\xfd\xf1\xc6" /* 7f59af... */),
    InternalNetworkAccessManager::reverseObfuscate("\x6a\x58\xac\x9e\xe6\xd3\xa4\x90\xc8\xae\x6a\x59\x95\xa0\x69\x58\x1d\x7c\xc0\xf9\x7b\x4c\xbe\xdb\xc9\xaf\xd6\xe5\xb5\x80\xf7\xc6\xf3\xc3\x23\x14\xcb\xa9\xaa\xce\x90\xa6\xba\x83\x7e\x4b\x99\xad\x49\x2c\x9f\xfc\xc3\xf2\xd0\xe7\x0e\x38\x39\x0a\xd6\xb4\x09\x38" /* 1b3671... */),
    InternalNetworkAccessManager::reverseObfuscate("\xc5\xf5\x94\xf1\x88\xba\x15\x73\x90\xa7\x9f\xfd\xc0\xa6\x92\xf1\x21\x40\x38\x5c\x5d\x6d\x41\x24\x22\x12\x72\x13\x9f\xf9\x56\x65\x3b\x5d\x8f\xeb\xde\xbb\xed\xdd\x55\x60\x21\x15\x29\x1b\x80\xb5\xd8\xbd\xe1\xd5\x57\x60\xeb\xd2\x80\xb5\xa6\xc3\xea\xdc\x16\x72" /* d6e597... */),
    InternalNetworkAccessManager::reverseObfuscate("\x58\x6f\xf2\xc1\x0f\x3d\x88\xbe\x6f\x5e\xa0\xc1\x24\x14\x42\x23\x0c\x6d\x72\x47\x78\x1b\x35\x02\x4a\x73\x7c\x1e\x8c\xef\x62\x57\xd5\xe2\x79\x1a\x6c\x08\x23\x10\xf9\x9b\x48\x71\xff\x9c\x0b\x3c\x59\x6e\xa4\xc1\x29\x11\x7f\x19\xcd\xf4\x5b\x62\x22\x14\x34\x56" /* b699f8... */)
};
const int OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiKeysCount = sizeof(OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiKeys) / sizeof(OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiKeys[0]);
int OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiKeyIndex = apiKeysCount - 1;

OnlineSearchScienceDirect::OnlineSearchScienceDirect(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchScienceDirectPrivate(this))
{
    /// nothing
}

OnlineSearchScienceDirect::~OnlineSearchScienceDirect()
{
    delete d;
}

void OnlineSearchScienceDirect::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    const bool r = d->startSearch(query, numResults);
    if (!r)
        delayedStoppedSearch(resultAuthorizationRequired);
}

QString OnlineSearchScienceDirect::label() const
{
#ifdef HAVE_KF
    return i18n("ScienceDirect");
#else // HAVE_KF
    //= onlinesearch-sciencedirect-label
    return QObject::tr("ScienceDirect");
#endif // HAVE_KF
}

QUrl OnlineSearchScienceDirect::homepage() const
{
    return QUrl(QStringLiteral("https://www.sciencedirect.com/"));
}

void OnlineSearchScienceDirect::doneFetchingJSON()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error() == 204 && reply->hasRawHeader("x-els-status") && reply->rawHeader("x-els-status").contains("AUTHORIZATION_ERROR")) {
        if (OnlineSearchScienceDirectPrivate::apiKeyIndex > 0) {
            --OnlineSearchScienceDirectPrivate::apiKeyIndex;
            const bool r = d->startSearch();
            if (!r)
                delayedStoppedSearch(resultAuthorizationRequired);
        } else {
            // No more API keys to test
            delayedStoppedSearch(resultAuthorizationRequired);
        }
        return;
    }

    Q_EMIT progress(++curStep, numSteps);


    if (handleErrors(reply)) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            if (document.isObject()) {
                const int resultsFound = document.object().value(QStringLiteral("resultsFound")).toInt(-1);
                if (resultsFound > 0) {
                    const QJsonValue resultArrayValue = document.object().value(QStringLiteral("results"));
                    if (resultArrayValue.isArray()) {
                        const QJsonArray resultArray = resultArrayValue.toArray();
                        bool encounteredUnexpectedData = false;
                        for (const QJsonValue &resultValue : resultArray) {
                            if (resultValue.isObject()) {
                                Entry *entry = d->entryFromJsonObject(resultValue.toObject());
                                if (entry != nullptr)
                                    publishEntry(QSharedPointer<Entry>(entry));
                                else {
                                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: Data could not be interpreted as a bibliographic entry";
                                    encounteredUnexpectedData = true;
                                    break;
                                }
                            } else {
                                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: No object found in 'results' array where expected";
                                encounteredUnexpectedData = true;
                                break;
                            }
                        }
                        if (encounteredUnexpectedData)
                            stopSearch(resultUnspecifiedError);
                        else
                            stopSearch(resultNoError);
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: No 'results' array found";
                        stopSearch(resultUnspecifiedError);
                    }
                } else if (resultsFound == 0) {
                    qCDebug(LOG_KBIBTEX_NETWORKING) << "No results found by ScienceDirect";
                    stopSearch(resultNoError);
                } else {
                    /// resultsFound < 0  --> no 'resultsFound' field in JSON data
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: No 'resultsFound' field found";
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: Document is not an object";
                stopSearch(resultUnspecifiedError);
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from ScienceDirect: " << parseError.errorString();
            stopSearch(resultUnspecifiedError);
        }
    }

    refreshBusyProperty();
}
