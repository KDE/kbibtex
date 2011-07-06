/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <KLocale>
#include <KDebug>

#include "iocommon.h"
#include "fileimporterbibtex.h"
#include "websearchjstor.h"

class WebSearchJStor::WebSearchJStorPrivate
{
private:
    WebSearchJStor *p;

public:
    int numFoundResults, curStep, numSteps;
    static const QString jstorBaseUrl;
    KUrl queryUrl;

    WebSearchJStorPrivate(WebSearchJStor *parent)
            : p(parent) {
        // nothing
    }

    void sanitizeEntry(Entry *entry) {
        QString url = PlainTextValue::text(entry->value(Entry::ftUrl));
        if (url.startsWith("http://www.jstor.org/stable/")) {
            entry->setId("jstor" + url.mid(28));
        }

        const QString formattedDateKey = "jstor_formatteddate";
        const QString formattedDate = PlainTextValue::text(entry->value(formattedDateKey));

        /// first, try to guess month by inspecting the beginning
        /// the jstor_formatteddate field
        const QString formattedDateLower = formattedDate.toLower();
        int i;
        for (i = 0; i < 12; ++i)
            if (formattedDateLower.startsWith(MonthsTriple[i])) break;
        entry->remove(formattedDateKey);
        if (i < 12) {
            Value v;
            v.append(new MacroKey(MonthsTriple[i]));
            entry->insert(Entry::ftMonth, v);
        }
        /// guessing failed, therefore extract first part if it exists
        else if ((i = formattedDate.indexOf(",")) >= 0) {
            /// text may be a season ("Winter")
            Value v;
            v.append(new PlainText(formattedDate.left(i)));
            entry->insert(Entry::ftMonth, v);
        } else {
            /// this case happens if the field only contains a year
            kDebug() << "Cannot extract month/season from date" << formattedDate;
        }

        /// page field may start with "pp. ", remove that
        QString pages = PlainTextValue::text(entry->value(Entry::ftPages)).toLower();
        if (pages.startsWith("pp. ")) {
            pages = pages.mid(4);
            entry->remove(Entry::ftPages);
            Value v;
            v.append(new PlainText(pages));
            entry->insert(Entry::ftPages, v);
        }
    }
};

const QString WebSearchJStor::WebSearchJStorPrivate::jstorBaseUrl = QLatin1String("http://www.jstor.org/");

WebSearchJStor::WebSearchJStor(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchJStorPrivate(this))
{
    // nothing
}

void WebSearchJStor::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 3;
    d->numFoundResults = 0;

    d->queryUrl = KUrl(d->jstorBaseUrl);
    d->queryUrl.setPath("/action/doAdvancedSearch");
    d->queryUrl.addQueryItem("Search", "Search");
    d->queryUrl.addQueryItem("wc", "on"); /// include external references, too
    d->queryUrl.addQueryItem("la", ""); /// no specific language
    d->queryUrl.addQueryItem("jo", ""); /// no specific journal
    d->queryUrl.addQueryItem("hp", QString::number(numResults)); /// hits per page
    int queryNumber = 0;
    QStringList elements = splitRespectingQuotationMarks(query[queryKeyTitle]);
    foreach(const QString& element, elements) {
        if (queryNumber > 0) d->queryUrl.addQueryItem(QString("c%1").arg(queryNumber), "AND");
        d->queryUrl.addQueryItem(QString("f%1").arg(queryNumber), "ti");
        d->queryUrl.addQueryItem(QString("q%1").arg(queryNumber), element);
        ++queryNumber;
    }
    elements = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    foreach(const QString& element, elements) {
        if (queryNumber > 0) d->queryUrl.addQueryItem(QString("c%1").arg(queryNumber), "AND");
        d->queryUrl.addQueryItem(QString("f%1").arg(queryNumber), "au");
        d->queryUrl.addQueryItem(QString("q%1").arg(queryNumber), element);
        ++queryNumber;
    }
    elements = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    foreach(const QString& element, elements) {
        if (queryNumber > 0) d->queryUrl.addQueryItem(QString("c%1").arg(queryNumber), "AND");
        d->queryUrl.addQueryItem(QString("f%1").arg(queryNumber), "all");
        d->queryUrl.addQueryItem(QString("q%1").arg(queryNumber), element);
        ++queryNumber;
    }
    if (!query[queryKeyYear].isEmpty()) {
        d->queryUrl.addQueryItem("sd", query[queryKeyYear]);
        d->queryUrl.addQueryItem("ed", query[queryKeyYear]);
    }
    kDebug() << "queryUrl=" << d->queryUrl.pathOrUrl();

    QNetworkRequest request(d->jstorBaseUrl);
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
    emit progress(d->curStep, d->numSteps);
}

void WebSearchJStor::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

QString WebSearchJStor::label() const
{
    return i18n("JSTOR");
}

QString WebSearchJStor::favIconUrl() const
{
    return QLatin1String("http://www.jstor.org/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchJStor::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchJStor::homepage() const
{
    return KUrl("http://www.jstor.org/");
}

void WebSearchJStor::cancel()
{
    WebSearchAbstract::cancel();
}

void WebSearchJStor::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QNetworkRequest request(d->queryUrl);
        setSuggestedHttpHeaders(request);
        QNetworkReply *reply = networkAccessManager()->get(request);
        setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchJStor::doneFetchingResultPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString htmlText = reply->readAll();
        QMap<QString, QString> formData = formParameters(htmlText, "<form action=\"/action/doAdvancedResults\"");
        formData.remove("doi");

        QStringList body;
        foreach(const QString& key, formData.keys()) {
            foreach(const QString& value, formData.values(key)) {
                body.append(encodeURL(key) + '=' + encodeURL(value));
            }
        }

        int p1 = htmlText.indexOf("<form action=\"/action/doAdvancedResults\""), p2 = -1;
        while ((p1 = htmlText.indexOf("<input type=\"checkbox\" name=\"doi\" value=\"", p1 + 1)) >= 0 && (p2 = htmlText.indexOf("\"", p1 + 42)) >= 0) {
            body.append("doi=" + encodeURL(htmlText.mid(p1 + 41, p2 - p1 - 41)));
        }
        body.append("selectUnselect=");

        QNetworkRequest request(d->jstorBaseUrl + "action/downloadCitation?format=bibtex&include=abs");
        setSuggestedHttpHeaders(request, reply);
        QNetworkReply *newReply = networkAccessManager()->post(request, body.join("&").toUtf8());
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSummaryPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchJStor::doneFetchingSummaryPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        ts.setCodec("utf-8");
        QString bibTeXcode = ts.readAll();

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    Value v;
                    v.append(new VerbatimText(label()));
                    entry->insert("x-fetchedfrom", v);

                    d->sanitizeEntry(entry);
                    emit foundEntry(entry);

                    ++d->numFoundResults;
                }
            }
            delete bibtexFile;
        }

        emit progress(d->numSteps, d->numSteps);
        emit stoppedSearch(d->numFoundResults > 0 ? resultNoError : resultUnspecifiedError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

