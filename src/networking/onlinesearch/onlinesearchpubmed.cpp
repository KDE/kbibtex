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

#include <KDebug>
#include <KLocale>
#include <KStandardDirs>
#include <KMessageBox>

#include "xsltransform.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"

const int OnlineSearchPubMed::maxNumResults = 25;
const qint64 OnlineSearchPubMed::queryChokeTimeout = 10 * 1000; /// 10 seconds
qint64 OnlineSearchPubMed::lastQueryEpoch = 0;

class OnlineSearchPubMed::OnlineSearchPubMedPrivate
{
private:
    OnlineSearchPubMed *p;
    const QString pubMedUrlPrefix;

public:
    XSLTransform *xslt;
    int numSteps, curStep;

    OnlineSearchPubMedPrivate(OnlineSearchPubMed *parent)
            : p(parent), pubMedUrlPrefix(QLatin1String("http://eutils.ncbi.nlm.nih.gov/entrez/eutils/")),
          numSteps(0), curStep(0) {
        const QString xsltFilename = QLatin1String("kbibtex/pubmed2bibtex.xsl");
        xslt = XSLTransform::createXSLTransform(KStandardDirs::locate("data", xsltFilename));
        if (xslt == NULL)
            kWarning() << "Could not create XSLT transformation for" << xsltFilename;
    }

    ~OnlineSearchPubMedPrivate() {
        delete xslt;
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        /// used to auto-detect PMIDs (unique identifiers for documents) in free text search
        static const QRegExp pmidRegExp(QLatin1String("^[0-9]{6,}$"));

        QString url = pubMedUrlPrefix + QLatin1String("esearch.fcgi?db=pubmed&tool=kbibtex&term=");

        /// append search terms
        QStringList queryFragments;

        /// add words from "free text" field, but auto-detect PMIDs
        QStringList freeTextWords = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        for (QStringList::ConstIterator it = freeTextWords.constBegin(); it != freeTextWords.constEnd(); ++it) {
            QString text = *it;
            queryFragments.append(text + (pmidRegExp.indexIn(text) >= 0 ? QLatin1String("") : QLatin1String("[All Fields]")));
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
            queryFragments.append(text + QLatin1String("[Title]"));
        }

        /// add words from "author" field
        QStringList authorWords = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (QStringList::ConstIterator it = authorWords.constBegin(); it != authorWords.constEnd(); ++it) {
            QString text = *it;
            queryFragments.append(text + QLatin1String("[Author]"));
        }

        /// Join all search terms with an AND operation
        url.append(queryFragments.join(QLatin1String("+AND+")));
        url = url.replace(QLatin1Char('"'), QLatin1String("%22"));

        /// set number of expected results
        url.append(QString(QLatin1String("&retstart=0&retmax=%1&retmode=xml")).arg(numResults));

        return KUrl(url);
    }

    KUrl buildFetchIdUrl(const QStringList &idList) {
        QString url = pubMedUrlPrefix + QLatin1String("efetch.fcgi?retmode=xml&db=pubmed&id=");

        url.append(idList.join(QLatin1String(",")));

        return KUrl(url);
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
    if (d->xslt == NULL) {
        /// Don't allow searches if xslt is not defined
        kWarning() << "Cannot allow searching" << label() << "if XSL Transformation not available";
        delayedStoppedSearch(resultUnspecifiedError);
        return;
    }

    d->curStep = 0;
    d->numSteps = 2;
    m_hasBeenCanceled = false;

    /// enforcing limit on number of results
    numResults = qMin(maxNumResults, numResults);
    /// enforcing choke on number of searchs per time
    if (QDateTime::currentDateTime().toTime_t() - lastQueryEpoch < queryChokeTimeout) {
        kDebug() << "Too many search queries per time; choke enforces pause of" << (queryChokeTimeout / 1000) << "seconds between queries";
        delayedStoppedSearch(resultNoError);
        return;
    }

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(eSearchDone()));

    emit progress(0, d->numSteps);
}


QString OnlineSearchPubMed::label() const
{
    return i18n("PubMed");
}

QString OnlineSearchPubMed::favIconUrl() const
{
    return QLatin1String("http://www.ncbi.nlm.nih.gov/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchPubMed::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchPubMed::homepage() const
{
    return KUrl("http://www.pubmed.gov/");
}

void OnlineSearchPubMed::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchPubMed::eSearchDone()
{
    emit progress(++d->curStep, d->numSteps);
    lastQueryEpoch = QDateTime::currentDateTime().toTime_t();

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString result = QString::fromUtf8(reply->readAll().data());

        if (!result.contains(QLatin1String("<Count>0</Count>"))) {
            /// without parsing XML text correctly, just extract all PubMed ids
            QStringList idList;
            int p1, p2;
            /// All IDs are within <IdList>...</IdList>
            if ((p1 = result.indexOf(QLatin1String("<IdList>"))) > 0 && (p2 = result.indexOf(QLatin1String("</IdList>"), p1)) > 0) {
                int p3, p4 = p1;
                /// Search for each <Id>...</Id>
                while ((p3 = result.indexOf(QLatin1String("<Id>"), p4)) > 0 && (p4 = result.indexOf(QLatin1String("</Id>"), p3)) > 0 && p4 < p2) {
                    /// Extract ID and add it to list
                    const QString id = result.mid(p3 + 4, p4 - p3 - 4);
                    idList << id;
                }
            }

            if (idList.isEmpty()) {
                kDebug() << "No ids here:" << squeeze_text(result.simplified(), 100);
                emit stoppedSearch(resultUnspecifiedError);
            } else {
                /// fetch full bibliographic details for found PubMed ids
                QNetworkRequest request(d->buildFetchIdUrl(idList));
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
                connect(newReply, SIGNAL(finished()), this, SLOT(eFetchDone()));
            }
        } else {
            /// search resulted in no hits (and PubMed told so)
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchPubMed::eFetchDone()
{
    emit progress(++d->curStep, d->numSteps);
    lastQueryEpoch = QDateTime::currentDateTime().toTime_t();

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString input = QString::fromUtf8(reply->readAll().data());

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = d->xslt->transform(input);
        /// remove XML header
        if (bibTeXcode[0] == '<')
            bibTeXcode = bibTeXcode.mid(bibTeXcode.indexOf(">") + 1);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            bool hasEntry = false;
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntry |= publishEntry(entry);
            }

            if (!hasEntry)
                kDebug() << "No BibTeX entry found here:" << squeeze_text(bibTeXcode, 100);
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
            delete bibtexFile;
        } else {
            kDebug() << "Doesn't look like BibTeX file:" << squeeze_text(bibTeXcode, 100);
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
