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

#ifdef HAVE_KF5
#include <KLocalizedString>
#endif // HAVE_KF5

#include "fileimporterbibtex.h"
#include "encoderxml.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"
#include "kbibtex.h"
#include "logging_networking.h"

class OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate
{
public:
    static const QUrl apiUrl;
    static const QString apiKey;
    const XSLTransform xslt;

    OnlineSearchScienceDirectPrivate(OnlineSearchScienceDirect *)
            : xslt(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QCoreApplication::instance()->applicationName().remove(QStringLiteral("test")) + QStringLiteral("/sciencedirectsearchapi-to-bibtex.xsl")))
    {
        /// nothing
    }

    int normalizeNumberOfResults(int requestedNumResults) const {
        if (requestedNumResults <= 10) return 10;
        else if (requestedNumResults <= 25) return 25;
        else if (requestedNumResults <= 50) return 50;
        else return 100;
    }

    QByteArray buildJsonQuery(const QMap<QString, QString> &query, int numResults) const {
        QString jsonQueryText = QStringLiteral("{\n");

        /// Free text
        const QStringList freeTextFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyFreeText]);
        if (!freeTextFragments.isEmpty())
            jsonQueryText.append(QStringLiteral("  \"qs\": \"\\\"") + freeTextFragments.join(QStringLiteral("\\\" AND \\\"")) + QStringLiteral("\\\"\""));

        /// Title
        const QStringList title = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyTitle]);
        if (!title.isEmpty()) {
            if (jsonQueryText != QStringLiteral("{\n")) jsonQueryText.append(QStringLiteral(",\n"));
            jsonQueryText.append(QStringLiteral("  \"title\": \"\\\"") + title.join(QStringLiteral("\\\" AND \\\"")) + QStringLiteral("\\\"\""));
        }

        /// Authors
        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyAuthor]);
        if (!authors.isEmpty()) {
            if (jsonQueryText != QStringLiteral("{\n")) jsonQueryText.append(QStringLiteral(",\n"));
            jsonQueryText.append(QStringLiteral("  \"authors\": \"\\\"") + authors.join(QStringLiteral("\\\" AND \\\"")) + QStringLiteral("\\\"\""));
        }

        /// Year
        static const QRegularExpression yearRangeRegExp(QStringLiteral("(18|19|20)[0-9]{2}(-+(18|19|20)[0-9]{2})?"));
        const QRegularExpressionMatch yearRangeRegExpMatch = yearRangeRegExp.match(query[queryKeyYear]);
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
            entry->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured())));

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
};

const QUrl OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiUrl(QStringLiteral("https://api.elsevier.com/content/search/sciencedirect"));
const QString OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiKey(InternalNetworkAccessManager::reverseObfuscate("\x43\x74\x9a\xa9\x6f\x5d\xa9\x9f\xeb\xda\xb9\xd8\x1b\x2b\x80\xe1\x3f\x5e\x29\x1c\xab\xc8\x54\x63\x58\x61\x13\x71\xca\xa9\xf1\xc4\xe4\xd3\xc9\xaa\x14\x70\xef\xdc\xb\x69\xff\xc6\xd5\xb6\x4a\x7d\x10\x27\xbb\xde\x92\xaa\xb0\xd6\xb9\x80\xd\x34\x48\x7e\x9d\xff"));

OnlineSearchScienceDirect::OnlineSearchScienceDirect(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchScienceDirectPrivate(this))
{
    /// nothing
}

OnlineSearchScienceDirect::~OnlineSearchScienceDirect()
{
    delete d;
}

void OnlineSearchScienceDirect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    emit progress(curStep = 0, numSteps = 1);

    QUrl u(OnlineSearchScienceDirectPrivate::apiUrl);
    QNetworkRequest request(u);
    request.setRawHeader(QByteArray("X-ELS-APIKey"), d->apiKey.toLatin1());
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Content-Type"), QByteArray("application/json"));

    const QByteArray jsonData = d->buildJsonQuery(query, numResults);

    QNetworkReply *reply = InternalNetworkAccessManager::instance().put(request, jsonData);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchScienceDirect::doneFetchingJSON);

    refreshBusyProperty();
}

QString OnlineSearchScienceDirect::label() const
{
    return i18n("ScienceDirect");
}

QString OnlineSearchScienceDirect::favIconUrl() const
{
    return QStringLiteral("https://sdfestaticassets-us-east-1.sciencedirectassets.com/shared-assets/11/images/favSD.ico");
}

QUrl OnlineSearchScienceDirect::homepage() const
{
    return QUrl(QStringLiteral("https://www.sciencedirect.com/"));
}

void OnlineSearchScienceDirect::doneFetchingJSON()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            if (document.isObject()) {
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
