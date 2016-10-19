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

#include "onlinesearchpubmed.h"

#include <QNetworkReply>
#include <QDateTime>
#include <QTimer>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KMessageBox>

#include "xsltransform.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

const int OnlineSearchPubMed::maxNumResults = 25;
const uint OnlineSearchPubMed::queryChokeTimeout = 10; /// 10 seconds
uint OnlineSearchPubMed::lastQueryEpoch = 0;

class OnlineSearchPubMed::OnlineSearchPubMedPrivate
{
private:
    OnlineSearchPubMed *p;
    const QString pubMedUrlPrefix;

public:
    XSLTransform xslt;
    int numSteps, curStep;

    OnlineSearchPubMedPrivate(OnlineSearchPubMed *parent)
            : p(parent), pubMedUrlPrefix(QStringLiteral("http://eutils.ncbi.nlm.nih.gov/entrez/eutils/")),
          xslt(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kbibtex/pubmed2bibtex.xsl"))),
          numSteps(0), curStep(0)
    {
        /// nothing
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        /// used to auto-detect PMIDs (unique identifiers for documents) in free text search
        static const QRegExp pmidRegExp(QStringLiteral("^[0-9]{6,}$"));

        QString url = pubMedUrlPrefix + QStringLiteral("esearch.fcgi?db=pubmed&tool=kbibtex&term=");

        /// append search terms
        QStringList queryFragments;

        /// add words from "free text" field, but auto-detect PMIDs
        QStringList freeTextWords = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        for (QStringList::ConstIterator it = freeTextWords.constBegin(); it != freeTextWords.constEnd(); ++it) {
            QString text = *it;
            queryFragments.append(text + (pmidRegExp.indexIn(text) >= 0 ? QStringLiteral("") : QStringLiteral("[All Fields]")));
        }

        /// add words from "year" field
        QStringList yearWords = p->splitRespectingQuotationMarks(query[queryKeyYear]);
        for (QStringList::ConstIterator it = yearWords.constBegin(); it != yearWords.constEnd(); ++it) {
            QString text = *it;
            queryFragments.append(text);
        }

        /// add words from "title" field
        QStringList titleWords = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        for (QStringList::ConstIterator it = titleWords.constBegin(); it != titleWords.constEnd(); ++it) {
            QString text = *it;
            queryFragments.append(text + QStringLiteral("[Title]"));
        }

        /// add words from "author" field
        QStringList authorWords = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (QStringList::ConstIterator it = authorWords.constBegin(); it != authorWords.constEnd(); ++it) {
            QString text = *it;
            queryFragments.append(text + QStringLiteral("[Author]"));
        }

        /// Join all search terms with an AND operation
        url.append(queryFragments.join(QStringLiteral("+AND+")));
        url = url.replace(QLatin1Char('"'), QStringLiteral("%22"));

        /// set number of expected results
        url.append(QString(QStringLiteral("&retstart=0&retmax=%1&retmode=xml")).arg(numResults));

        return QUrl::fromUserInput(url);
    }

    QUrl buildFetchIdUrl(const QStringList &idList) {
        QString url = pubMedUrlPrefix + QStringLiteral("efetch.fcgi?retmode=xml&db=pubmed&id=");

        url.append(idList.join(QStringLiteral(",")));

        return QUrl(url);
    }
};

OnlineSearchPubMed::OnlineSearchPubMed(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchPubMed::OnlineSearchPubMedPrivate(this))
{
    // nothing
}

OnlineSearchPubMed::~OnlineSearchPubMed()
{
    delete d;
}

void OnlineSearchPubMed::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchPubMed::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->curStep = 0;
    d->numSteps = 2;
    m_hasBeenCanceled = false;

    /// enforcing limit on number of results
    numResults = qMin(maxNumResults, numResults);
    /// enforcing choke on number of searchs per time
    if (QDateTime::currentDateTimeUtc().toTime_t() - lastQueryEpoch < queryChokeTimeout) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Too many search queries per time; choke enforces pause of" << queryChokeTimeout << "seconds between queries";
        delayedStoppedSearch(resultNoError);
        return;
    }

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchPubMed::eSearchDone);

    emit progress(0, d->numSteps);
}


QString OnlineSearchPubMed::label() const
{
    return i18n("PubMed");
}

QString OnlineSearchPubMed::favIconUrl() const
{
    return QStringLiteral("http://www.ncbi.nlm.nih.gov/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchPubMed::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchPubMed::homepage() const
{
    return QUrl(QStringLiteral("http://www.ncbi.nlm.nih.gov/pubmed/"));
}

void OnlineSearchPubMed::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchPubMed::eSearchDone()
{
    emit progress(++d->curStep, d->numSteps);
    lastQueryEpoch = QDateTime::currentDateTimeUtc().toTime_t();

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString result = QString::fromUtf8(reply->readAll().constData());

        if (!result.contains(QStringLiteral("<Count>0</Count>"))) {
            /// without parsing XML text correctly, just extract all PubMed ids
            QStringList idList;
            int p1, p2;
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
                emit stoppedSearch(resultUnspecifiedError);
            } else {
                /// fetch full bibliographic details for found PubMed ids
                QNetworkRequest request(d->buildFetchIdUrl(idList));
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
                connect(newReply, &QNetworkReply::finished, this, &OnlineSearchPubMed::eFetchDone);
            }
        } else {
            /// search resulted in no hits (and PubMed told so)
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

void OnlineSearchPubMed::eFetchDone()
{
    emit progress(++d->curStep, d->numSteps);
    lastQueryEpoch = QDateTime::currentDateTimeUtc().toTime_t();

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString input = QString::fromUtf8(reply->readAll().constData());

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = d->xslt.transform(input);
        if (bibTeXcode.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << reply->url().toDisplayString();
            emit stoppedSearch(resultInvalidArguments);
        } else {  /// remove XML header
            if (bibTeXcode[0] == '<')
                bibTeXcode = bibTeXcode.mid(bibTeXcode.indexOf(QStringLiteral(">")) + 1);

            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            if (bibtexFile != NULL) {
                bool hasEntry = false;
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    hasEntry |= publishEntry(entry);
                }

                emit stoppedSearch(resultNoError);
                emit progress(d->numSteps, d->numSteps);
                delete bibtexFile;
            } else {
                emit stoppedSearch(resultUnspecifiedError);
            }
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}
