/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QTextStream>
#include <QNetworkReply>

#include <KDebug>
#include <KLocale>
#include <KStandardDirs>
#include <KMessageBox>

#include <internalnetworkaccessmanager.h>
#include "xsltransform.h"
#include "fileimporterbibtex.h"
#include "onlinesearchpubmed.h"

class OnlineSearchPubMed::OnlineSearchPubMedPrivate
{
private:
    OnlineSearchPubMed *p;
    const QString pubMedUrlPrefix;

public:
    XSLTransform xslt;
    int numSteps, curStep;

    OnlineSearchPubMedPrivate(OnlineSearchPubMed *parent)
            : p(parent), pubMedUrlPrefix(QLatin1String("http://eutils.ncbi.nlm.nih.gov/entrez/eutils/")), xslt(KStandardDirs::locate("data", "kbibtex/pubmed2bibtex.xsl")) {
        // nothing
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
        url.append(queryFragments.join("+AND+"));
        url = url.replace("\"", "%22");

        /// set number of expected results
        url.append(QString("&retstart=0&retmax=%1&retmode=xml").arg(numResults));

        kDebug() << "pubmed url =" << url;
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
    emit stoppedSearch(resultNoError);
}

void OnlineSearchPubMed::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->curStep = 0;
    d->numSteps = 2;
    m_hasBeenCanceled = false;
    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
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

OnlineSearchQueryFormAbstract* OnlineSearchPubMed::customWidget(QWidget *)
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

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString result = reply->readAll();

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
                setNetworkReplyTimeout(newReply);
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

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString input = QString::fromUtf8(reply->readAll().data());

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = d->xslt.transform(input);
        /// remove XML header
        if (bibTeXcode[0] == '<')
            bibTeXcode = bibTeXcode.mid(bibTeXcode.indexOf(">") + 1);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            bool hasEntry = false;
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (!entry.isNull()) {
                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                    entry->insert("x-fetchedfrom", v);

                    hasEntry = true;
                    emit foundEntry(entry);
                }
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
