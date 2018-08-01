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

#ifdef HAVE_KF5
#include <KLocalizedString>
#endif // HAVE_KF5

#include "fileimporterbibtex.h"
#include "encoderxml.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"
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

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QUrl queryUrl = apiUrl;
        QUrlQuery q(queryUrl.query());

        QString queryText;

        /// Free text
        const QStringList freeTextFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyFreeText]);
        if (!freeTextFragments.isEmpty()) {
            if (!queryText.isEmpty()) queryText.append(QStringLiteral(" AND "));
            queryText.append(QStringLiteral("\"") + freeTextFragments.join(QStringLiteral("\" AND \"")) + QStringLiteral("\""));
        }

        /// Title
        const QStringList title = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyTitle]);
        if (!title.isEmpty()) {
            if (!queryText.isEmpty()) queryText.append(QStringLiteral(" AND "));
            queryText.append(QStringLiteral("title(\"") + title.join(QStringLiteral("\" AND \"")) + QStringLiteral("\")"));
        }

        /// Authors
        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyAuthor]);
        if (!authors.isEmpty()) {
            if (!queryText.isEmpty()) queryText.append(QStringLiteral(" AND "));
            queryText.append(QStringLiteral("aut(\"") + authors.join(QStringLiteral("\" AND \"")) + QStringLiteral("\")"));
        }

        q.addQueryItem(QStringLiteral("query"), queryText);

        /// Year
        if (!query[queryKeyYear].isEmpty())
            q.addQueryItem(QStringLiteral("date"), query[queryKeyYear]);

        /// Request numResults many entries
        q.addQueryItem(QStringLiteral("count"), QString::number(numResults));

        queryUrl.setQuery(q);

        return queryUrl;
    }
};

const QUrl OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate::apiUrl(QStringLiteral("https://api.elsevier.com/content/search/scidir"));
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

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    request.setRawHeader(QByteArray("X-ELS-APIKey"), d->apiKey.toLatin1());
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/xml"));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchScienceDirect::doneFetchingXML);

    refreshBusyProperty();
}

QString OnlineSearchScienceDirect::label() const
{
    return i18n("ScienceDirect");
}

QString OnlineSearchScienceDirect::favIconUrl() const
{
    return QStringLiteral("https://cdn.els-cdn.com/sd/favSD.ico");
}

QUrl OnlineSearchScienceDirect::homepage() const
{
    return QUrl(QStringLiteral("https://www.sciencedirect.com/"));
}

void OnlineSearchScienceDirect::doneFetchingXML()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            request.setRawHeader(QByteArray("X-ELS-APIKey"), d->apiKey.toLatin1());
            request.setRawHeader(QByteArray("Accept"), QByteArray("application/xml"));
            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchScienceDirect::doneFetchingXML);
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString xmlCode = QString::fromUtf8(reply->readAll().constData()).remove(QStringLiteral("xmlns=\"http://www.w3.org/2005/Atom\""));

            /// use XSL transformation to get BibTeX document from XML result
            const QString bibTeXcode = EncoderXML::instance().decode(d->xslt.transform(xmlCode));
            if (bibTeXcode.isEmpty()) {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                stopSearch(resultInvalidArguments);
            } else {
                FileImporterBibTeX importer(this);
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != nullptr) {
                    for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    stopSearch(resultNoError);

                    delete bibtexFile;
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                    stopSearch(resultUnspecifiedError);
                }
            }
        }
    }

    refreshBusyProperty();
}
