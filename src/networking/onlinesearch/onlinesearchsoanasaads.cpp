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

#include "onlinesearchsoanasaads.h"

#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFile>

#ifdef HAVE_KF5
#include <KLocalizedString>
#endif // HAVE_KF5

#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchSOANASAADS::Private
{
private:
    static const QUrl apiSearchUrl;

public:
    static const QUrl apiExportUrl;
    static const QByteArray apiKey;

    QUrl buildSearchUrl(const QMap<QueryKey, QString> &query, int numResults)
    {
        QUrl url(apiSearchUrl);
        QUrlQuery urlQuery;

        QString q;

        /// Free text
        const QStringList freeTextFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::FreeText]);
        if (!freeTextFragments.isEmpty())
            q.append(QStringLiteral("\"") + freeTextFragments.join(QStringLiteral("\" \"")) + QStringLiteral("\""));

        /// Title
        const QStringList title = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Title]);
        if (!title.isEmpty()) {
            if (!q.isEmpty()) q.append(' ');
            q.append(QStringLiteral("title:\"") + title.join(QStringLiteral("\" title:\"")) + QStringLiteral("\""));
        }

        /// Authors
        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Author]);
        if (!authors.isEmpty()) {
            if (!q.isEmpty()) q.append(' ');
            q.append(QStringLiteral("author:\"") + authors.join(QStringLiteral("\" author:\"")) + QStringLiteral("\""));
        }

        QString year = query[QueryKey::Year];
        if (!year.isEmpty()) {
            static const QRegularExpression yearRegExp("\\b(18|19|20)[0-9]{2}\\b");
            const QRegularExpressionMatch yearRegExpMatch = yearRegExp.match(year);
            if (yearRegExpMatch.hasMatch()) {
                if (!q.isEmpty()) q.append(' ');
                year = yearRegExpMatch.captured(0);
                q.append(QStringLiteral(" year:") + year);
            }
        }

        urlQuery.addQueryItem(QStringLiteral("fl"), QStringLiteral("bibcode"));
        urlQuery.addQueryItem(QStringLiteral("q"), q);
        urlQuery.addQueryItem(QStringLiteral("rows"), QString::number(numResults));

        url.setQuery(urlQuery);
        return url;
    }
};

const QUrl OnlineSearchSOANASAADS::Private::apiSearchUrl(QStringLiteral("https://api.adsabs.harvard.edu/v1/search/query"));
const QUrl OnlineSearchSOANASAADS::Private::apiExportUrl(QStringLiteral("https://api.adsabs.harvard.edu/v1/export/bibtexabs"));
const QByteArray OnlineSearchSOANASAADS::Private::apiKey(InternalNetworkAccessManager::reverseObfuscate("\xa3\xe4\xf4\x9b\xe9\xd8\xdb\xb3\x74\x3a\x28\x61\x1d\x51\x35\x6d\x7b\x48\xd0\x8a\xe4\xd7\x82\xfa\x1d\x4e\x33\x76\xca\x90\x65\x52\xd\x5e\x73\x1b\x94\xf7\x79\x49\xdf\xa6\xb0\xe4\x9c\xc6\x8a\xbc\x6e\x24\x56\x5\xa2\xd2\xaf\xf7\xc4\xf4\x8\x5a\x62\x54\x60\x21\x92\xf7\xc6\xa7\xe6\xaf\x68\x5b\x8c\xcb\xdd\xb1\x5c\x2a\xd\x5c").toLatin1());

OnlineSearchSOANASAADS::OnlineSearchSOANASAADS(QObject *parent)
        : OnlineSearchAbstract(parent), d(new Private)
{
    /// nothing
}

OnlineSearchSOANASAADS::~OnlineSearchSOANASAADS()
{
    delete d;
}

void OnlineSearchSOANASAADS::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    emit progress(curStep = 0, numSteps = 2);

    QUrl url = d->buildSearchUrl(query, numResults);
    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + Private::apiKey);

    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchSOANASAADS::doneFetchingSearchJSON);

    refreshBusyProperty();
}

QString OnlineSearchSOANASAADS::label() const
{
    return i18n("NASA Astrophysics Data System (ADS)");
}

QUrl OnlineSearchSOANASAADS::homepage() const
{
    return QUrl(QStringLiteral("https://ui.adsabs.harvard.edu/"));
}

void OnlineSearchSOANASAADS::doneFetchingSearchJSON()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            if (document.isObject()) {
                const QJsonValue responseValue = document.object().value(QStringLiteral("response"));
                if (responseValue.isObject()) {
                    const QJsonValue docsValue = responseValue.toObject().value(QStringLiteral("docs"));
                    if (docsValue.isArray()) {
                        const QJsonArray docsArray = docsValue.toArray();
                        QStringList bibcodeList;
                        for (const QJsonValue &itemValue : docsArray) {
                            if (itemValue.isObject()) {
                                const QJsonValue bibcodeValue = itemValue.toObject().value(QString("bibcode"));
                                if (bibcodeValue.isString()) {
                                    bibcodeList.append(bibcodeValue.toString());
                                }
                            } else {
                                qCDebug(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: No 'bibcode' found";
                            }
                        }

                        if (bibcodeList.isEmpty()) {
                            qCInfo(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: No bibcode identifiers for query found";
                            stopSearch(resultNoError);
                        } else {
                            const QString exportJson = QStringLiteral("{\"bibcode\":[\"") + bibcodeList.join(QStringLiteral("\",\"")) + QStringLiteral("\"]}");
                            QNetworkRequest request(Private::apiExportUrl);
                            request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + Private::apiKey);
                            request.setRawHeader(QByteArray("Content-Type"), QByteArray("application/json"));
                            QNetworkReply *newReply = InternalNetworkAccessManager::instance().post(request, exportJson.toUtf8());
                            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
                            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchSOANASAADS::doneFetchingExportBibTeX);
                        }
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: No 'docs' found";
                        stopSearch(resultNoError);
                    }
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: No 'response' found";
                    stopSearch(resultNoError);
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: Document is not an object";
                stopSearch(resultUnspecifiedError);
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: " << parseError.errorString();
            stopSearch(resultUnspecifiedError);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchSOANASAADS::doneFetchingExportBibTeX()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            if (document.isObject()) {
                const QJsonValue exportValue = document.object().value(QStringLiteral("export"));
                if (exportValue.isString()) {
                    FileImporterBibTeX importer(this);
                    File *bibtexFile = importer.fromString(exportValue.toString());

                    if (bibtexFile != nullptr) {
                        for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                            QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                            if (!publishEntry(entry))
                                qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to publish entry";
                        }
                        delete bibtexFile;
                    }
                    stopSearch(resultNoError);
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: No 'export' found";
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: Document is not an object";
                stopSearch(resultUnspecifiedError);
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Astrophysics Data System: " << parseError.errorString();
            stopSearch(resultUnspecifiedError);
        }
    }

    refreshBusyProperty();
}
