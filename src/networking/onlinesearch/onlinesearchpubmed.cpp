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

#include "onlinesearchpubmed.h"

#include <QNetworkReply>
#include <QDateTime>
#include <QTimer>
#include <QRegularExpression>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KMessageBox>
#endif // HAVE_KF5

#include "xsltransform.h"
#include "encoderxml.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

const int OnlineSearchPubMed::maxNumResults = 25;
const uint OnlineSearchPubMed::queryChokeTimeout = 10; /// 10 seconds
uint OnlineSearchPubMed::lastQueryEpoch = 0;

class OnlineSearchPubMed::OnlineSearchPubMedPrivate
{
private:
    const QString pubMedUrlPrefix;
    static const QString xsltFilenameBase;

public:
    const XSLTransform xslt;

    OnlineSearchPubMedPrivate(OnlineSearchPubMed *)
            : pubMedUrlPrefix(QStringLiteral("https://eutils.ncbi.nlm.nih.gov/entrez/eutils/")),
          xslt(XSLTransform::locateXSLTfile(xsltFilenameBase))
    {
        if (!xslt.isValid())
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to initialize XSL transformation based on file '" << xsltFilenameBase << "'";
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        /// used to auto-detect PMIDs (unique identifiers for documents) in free text search
        static const QRegularExpression pmidRegExp(QStringLiteral("^[0-9]{6,}$"));

        QString url = pubMedUrlPrefix + QStringLiteral("esearch.fcgi?db=pubmed&tool=kbibtex&term=");

        const QStringList freeTextWords = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyFreeText]);
        const QStringList yearWords = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyYear]);
        const QStringList titleWords = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyTitle]);
        const QStringList authorWords = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyAuthor]);

        /// append search terms
        QStringList queryFragments;
        queryFragments.reserve(freeTextWords.size() + yearWords.size() + titleWords.size() + authorWords.size());

        /// add words from "free text" field, but auto-detect PMIDs
        for (const QString &text : freeTextWords)
            queryFragments.append(text + (pmidRegExp.match(text).hasMatch() ? QString() : QStringLiteral("[All Fields]")));

        /// add words from "year" field
        for (const QString &text : yearWords)
            queryFragments.append(text);

        /// add words from "title" field
        for (const QString &text : titleWords)
            queryFragments.append(text + QStringLiteral("[Title]"));

        /// add words from "author" field
        for (const QString &text : authorWords)
            queryFragments.append(text + QStringLiteral("[Author]"));

        /// Join all search terms with an AND operation
        url.append(queryFragments.join(QStringLiteral("+AND+")));
        url = url.replace(QLatin1Char('"'), QStringLiteral("%22"));

        /// set number of expected results
        url.append(QString(QStringLiteral("&retstart=0&retmax=%1&retmode=xml")).arg(numResults));

        return QUrl::fromUserInput(url);
    }

    QUrl buildFetchIdUrl(const QStringList &idList) {
        const QString urlText = pubMedUrlPrefix + QStringLiteral("efetch.fcgi?retmode=xml&db=pubmed&id=") + idList.join(QStringLiteral(","));
        return QUrl::fromUserInput(urlText);
    }
};

const QString OnlineSearchPubMed::OnlineSearchPubMedPrivate::xsltFilenameBase = QStringLiteral("pubmed2bibtex.xsl");


OnlineSearchPubMed::OnlineSearchPubMed(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchPubMed::OnlineSearchPubMedPrivate(this))
{
    /// nothing
}

OnlineSearchPubMed::~OnlineSearchPubMed()
{
    delete d;
}

void OnlineSearchPubMed::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 2);

    /// enforcing limit on number of results
    numResults = qMin(maxNumResults, numResults);
    /// enforcing choke on number of searchs per time
    if (QDateTime::currentDateTimeUtc().toTime_t() - lastQueryEpoch < queryChokeTimeout) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Too many search queries per time; choke enforces pause of" << queryChokeTimeout << "seconds between queries";
        delayedStoppedSearch(resultNoError);
        return;
    }

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchPubMed::eSearchDone);

    refreshBusyProperty();
}


QString OnlineSearchPubMed::label() const
{
    return i18n("PubMed");
}

QString OnlineSearchPubMed::favIconUrl() const
{
    return QStringLiteral("https://www.ncbi.nlm.nih.gov/favicon.ico");
}

QUrl OnlineSearchPubMed::homepage() const
{
    return QUrl(QStringLiteral("https://www.ncbi.nlm.nih.gov/pubmed/"));
}

void OnlineSearchPubMed::eSearchDone()
{
    emit progress(++curStep, numSteps);
    lastQueryEpoch = QDateTime::currentDateTimeUtc().toTime_t();

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString result = QString::fromUtf8(reply->readAll().constData());

        if (!result.contains(QStringLiteral("<Count>0</Count>"))) {
            /// without parsing XML text correctly, just extract all PubMed ids
            QStringList idList;
            int p1, p2 = 0;
            /// All IDs are within <IdList>...</IdList>
            if ((p1 = result.indexOf(QStringLiteral("<IdList>"))) > 0 && (p2 = result.indexOf(QStringLiteral("</IdList>"), p1)) > 0) {
                int p3, p4 = p1;
                /// Search for each <Id>...</Id>
                while ((p3 = result.indexOf(QStringLiteral("<Id>"), p4)) > 0 && (p4 = result.indexOf(QStringLiteral("</Id>"), p3)) > 0 && p4 < p2) {
                    /// Extract ID and add it to list
                    const QString id = result.mid(p3 + 4, p4 - p3 - 4);
                    idList << id;
                }
            }

            if (idList.isEmpty()) {
                stopSearch(resultUnspecifiedError);
            } else {
                /// fetch full bibliographic details for found PubMed ids
                QNetworkRequest request(d->buildFetchIdUrl(idList));
                QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
                InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
                connect(newReply, &QNetworkReply::finished, this, &OnlineSearchPubMed::eFetchDone);
            }
        } else {
            /// search resulted in no hits (and PubMed told so)
            stopSearch(resultNoError);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchPubMed::eFetchDone()
{
    emit progress(++curStep, numSteps);
    lastQueryEpoch = QDateTime::currentDateTimeUtc().toTime_t();

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString input = QString::fromUtf8(reply->readAll().constData());

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = EncoderXML::instance().decode(d->xslt.transform(input));
        if (bibTeXcode.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultInvalidArguments);
        } else {  /// remove XML header
            if (bibTeXcode[0] == '<')
                bibTeXcode = bibTeXcode.mid(bibTeXcode.indexOf(QStringLiteral(">")) + 1);

            FileImporterBibTeX importer(this);
            File *bibtexFile = importer.fromString(bibTeXcode);

            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                    publishEntry(entry);
                }

                stopSearch(resultNoError);

                delete bibtexFile;
            } else {
                stopSearch(resultUnspecifiedError);
            }
        }
    }

    refreshBusyProperty();
}
