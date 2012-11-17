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

#include <QNetworkReply>
#include <QTextStream>

#include <KMessageBox>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KStandardDirs>

#include "websearchieeexplore.h"
#include "xsltransform.h"
#include "fileimporterbibtex.h"

class WebSearchIEEEXplore::WebSearchIEEEXplorePrivate
{
private:
    WebSearchIEEEXplore *p;

public:
    const QString gatewayUrl;
    XSLTransform xslt;
    int numSteps, curStep;

    WebSearchIEEEXplorePrivate(WebSearchIEEEXplore *parent)
            : p(parent), gatewayUrl(QLatin1String("http://ieeexplore.ieee.org/gateway/ipsSearch.jsp")), xslt(KStandardDirs::locate("data", "kbibtex/ieeexplore2bibtex.xsl")) {
        // nothing
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        KUrl queryUrl = KUrl(gatewayUrl);

        QStringList queryText;

        /// Free text
        QStringList freeTextFragments = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(const QString &freeTextFragment, freeTextFragments) {
            queryText << QString(QLatin1String("\"%1\"")).arg(freeTextFragment);
        }

        /// Title
        if (!query[queryKeyTitle].isEmpty())
            queryText << QString(QLatin1String("\"Document Title\":\"%1\"")).arg(query[queryKeyTitle]);

        /// Author
        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(const QString &author, authors) {
            queryText << QString(QLatin1String("Author:\"%1\"")).arg(author);
        }

        /// Year
        if (!query[queryKeyYear].isEmpty())
            queryText << QString(QLatin1String("\"Publication Year\":\"%1\"")).arg(query[queryKeyYear]);

        queryUrl.addQueryItem(QLatin1String("queryText"), queryText.join(QLatin1String(" AND ")));
        queryUrl.addQueryItem(QLatin1String("sortfield"), QLatin1String("py"));
        queryUrl.addQueryItem(QLatin1String("sortorder"), QLatin1String("desc"));
        queryUrl.addQueryItem(QLatin1String("hc"), QString::number(numResults));
        queryUrl.addQueryItem(QLatin1String("rs"), QLatin1String("1"));

        kDebug() << "buildQueryUrl=" << queryUrl.pathOrUrl();

        return queryUrl;
    }

    void sanitize(Entry *entry) {
        /// XSL file cannot yet replace semicolon-separate author list
        /// by "and"-separated author list, so do it manually
        const QString ftXAuthor = QLatin1String("x-author");
        if (!entry->contains(Entry::ftAuthor) && entry->contains(ftXAuthor)) {
            const Value xAuthorValue = entry->value(ftXAuthor);
            Value authorValue;
            for (Value::ConstIterator it = xAuthorValue.constBegin(); it != xAuthorValue.constEnd(); ++it) {
                PlainText *pt = dynamic_cast<PlainText *>(*it);
                if (pt != NULL) {
                    QStringList names;
                    FileImporterBibTeX::splitPersonList(pt->text(), names);
                    for (QStringList::ConstIterator nit = names.constBegin(); nit != names.constEnd(); ++nit) {
                        Person *person = FileImporterBibTeX::splitName(*nit);
                        authorValue << person;
                    }
                }
            }

            entry->insert(Entry::ftAuthor, authorValue);
            entry->remove(ftXAuthor);
        }
    }
};

WebSearchIEEEXplore::WebSearchIEEEXplore(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchIEEEXplore::WebSearchIEEEXplorePrivate(this))
{
    // nothing
}

WebSearchIEEEXplore::~WebSearchIEEEXplore()
{
    delete d;
}

void WebSearchIEEEXplore::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void WebSearchIEEEXplore::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 2;

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingXML()));

    emit progress(d->curStep, d->numSteps);
}

void WebSearchIEEEXplore::doneFetchingXML()
{
    ++d->curStep;
    emit progress(d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
            /// redirection to another url
            QUrl redirUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            setSuggestedHttpHeaders(request, reply);
            QNetworkReply *newReply = networkAccessManager()->get(request);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingXML()));
        } else {
            QTextStream ts(reply->readAll());
            ts.setCodec("utf-8");
            const QString xmlCode = ts.readAll();

            /// use XSL transformation to get BibTeX document from XML result
            const QString bibTeXcode = d->xslt.transform(xmlCode);

            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    Entry *entry = dynamic_cast<Entry *>(*it);
                    if (entry != NULL) {
                        Value v;
                        v.append(new VerbatimText(label()));
                        entry->insert("x-fetchedfrom", v);
                        d->sanitize(entry);
                        emit foundEntry(entry);
                        hasEntries = true;
                    }

                }

                if (!hasEntries)
                    kDebug() << "No hits found in" << reply->url().toString();
                emit stoppedSearch(resultNoError);
                emit progress(d->numSteps, d->numSteps);

                delete bibtexFile;
            } else {
                kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                emit stoppedSearch(resultUnspecifiedError);
            }
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

QString WebSearchIEEEXplore::label() const
{
    return i18n("IEEEXplore");
}

QString WebSearchIEEEXplore::favIconUrl() const
{
    return QLatin1String("http://ieeexplore.ieee.org/favicon.ico");
}

WebSearchQueryFormAbstract *WebSearchIEEEXplore::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchIEEEXplore::homepage() const
{
    return KUrl("http://ieeexplore.ieee.org/");
}

void WebSearchIEEEXplore::cancel()
{
    WebSearchAbstract::cancel();
}
