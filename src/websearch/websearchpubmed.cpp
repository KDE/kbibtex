/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include "websearchpubmed.h"
#include "xsltransform.h"
#include "fileimporterbibtex.h"

class WebSearchPubMed::WebSearchPubMedPrivate
{
private:
    WebSearchPubMed *p;
    const QString pubMedUrlPrefix;

public:
    XSLTransform xslt;

    WebSearchPubMedPrivate(WebSearchPubMed *parent)
            : p(parent), pubMedUrlPrefix(QLatin1String("http://eutils.ncbi.nlm.nih.gov/entrez/eutils/")), xslt(KStandardDirs::locate("appdata", "pubmed2bibtex.xsl")) {
        // nothing
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QString url = pubMedUrlPrefix + QLatin1String("esearch.fcgi?db=pubmed&term=");

        /// append search terms
        QStringList queryFragments;

        /// add words from "free text" and "year" fields
        QStringList freeTextWords = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        for (QStringList::ConstIterator it = freeTextWords.constBegin(); it != freeTextWords.constEnd(); ++it)
            queryFragments.append('"' + (*it) + '"');
        QStringList yearWords = p->splitRespectingQuotationMarks(query[queryKeyYear]);
        for (QStringList::ConstIterator it = yearWords.constBegin(); it != yearWords.constEnd(); ++it)
            queryFragments.append('"' + (*it) + '"');

        /// add words from "title" field
        QStringList titleWords = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        for (QStringList::ConstIterator it = titleWords.constBegin(); it != titleWords.constEnd(); ++it)
            queryFragments.append('"' + (*it) + '"' + QLatin1String("[title]"));

        /// add words from "author" field
        QStringList authorWords = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (QStringList::ConstIterator it = authorWords.constBegin(); it != authorWords.constEnd(); ++it)
            queryFragments.append('"' + (*it) + '"' + QLatin1String("[author]"));

        url.append(queryFragments.join("+AND+"));

        /// set number of expected results
        url.append(QString("&retstart=0&retmax=%1&retmode=xml").arg(numResults));

        return KUrl(url);
    }

    KUrl buildFetchIdUrl(const QStringList &idList) {
        QString url = pubMedUrlPrefix + QLatin1String("efetch.fcgi?retmode=xml&db=pubmed&id=");

        url.append(idList.join(QLatin1String(",")));

        return KUrl(url);
    }
};

WebSearchPubMed::WebSearchPubMed(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchPubMed::WebSearchPubMedPrivate(this))
{
    // nothing
}

void WebSearchPubMed::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

void WebSearchPubMed::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(eSearchDone()));
}

QString WebSearchPubMed::label() const
{
    return i18n("PubMed");
}

QString WebSearchPubMed::favIconUrl() const
{
    return QLatin1String("http://www.ncbi.nlm.nih.gov/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchPubMed::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchPubMed::homepage() const
{
    return KUrl("http://www.pubmed.gov/");
}

void WebSearchPubMed::cancel()
{
    WebSearchAbstract::cancel();
}

void WebSearchPubMed::eSearchDone()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString result = reply->readAll();

        /// without parsing XML text correctly, just extract all PubMed ids
        QRegExp regExpId("<Id>(\\d+)</Id>", Qt::CaseInsensitive);
        int p = -1;
        QStringList idList;
        while ((p = result.indexOf(regExpId, p + 1)) >= 0)
            idList << regExpId.cap(1);

        if (idList.isEmpty())
            emit stoppedSearch(resultUnspecifiedError);
        else {
            /// fetch full bibliographic details for found PubMed ids
            QNetworkRequest request(d->buildFetchIdUrl(idList));
            setSuggestedHttpHeaders(request, reply);
            QNetworkReply *newReply = networkAccessManager()->get(request);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(eFetchDone()));
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchPubMed::eFetchDone()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString input = reply->readAll();

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = d->xslt.transform(input);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            bool hasEntry = false;
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    hasEntry = true;
                    emit foundEntry(entry);
                }
            }
            emit stoppedSearch(hasEntry ? resultNoError : resultUnspecifiedError);
            delete bibtexFile;
        } else
            emit stoppedSearch(resultUnspecifiedError);
    } else
        kDebug() << "url was" << reply->url().toString();
}
