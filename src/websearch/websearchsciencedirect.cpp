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

#include <QNetworkReply>

#include <KDebug>
#include <KLocale>

#include <encoderlatex.h>
#include "fileimporterbibtex.h"
#include "websearchsciencedirect.h"


class WebSearchScienceDirect::WebSearchScienceDirectPrivate
{
private:
    WebSearchScienceDirect *p;

public:
    QStringList queryFreetext, queryAuthor;
    int currentSearchPosition;
    int numExpectedResults;
    const QString scienceDirectBaseUrl;
    QStringList bibTeXUrls;
    int runningJobs;

    WebSearchScienceDirectPrivate(WebSearchScienceDirect *parent)
            : p(parent), scienceDirectBaseUrl(QLatin1String("http://www.sciencedirect.com/")) {
        // nothing
    }

    void sanitizeBibTeXCode(QString &code) {
        /// find and escape unprotected quotation marks in the text (e.g. abstract)
        const QRegExp quotationMarks("([^= ]\\s*)\"(\\s*[a-z.])", Qt::CaseInsensitive);
        int p = -2;
        while ((p = quotationMarks.indexIn(code, p + 2)) >= 0) {
            code = code.left(p - 1) + quotationMarks.cap(1) + '\\' + '"' + quotationMarks.cap(2) + code.mid(p + quotationMarks.cap(0).length());
        }
    }
};

WebSearchScienceDirect::WebSearchScienceDirect(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchScienceDirectPrivate(this))
{
    // nothing
}

void WebSearchScienceDirect::startSearch()
{
    d->runningJobs = 0;
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

void WebSearchScienceDirect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->runningJobs = 0;
    m_hasBeenCanceled = false;
    d->bibTeXUrls.clear();
    d->currentSearchPosition = 0;

    d->queryFreetext.clear();
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyFreeText])) {
        d->queryFreetext.append(encodeURL(queryFragment));
    }
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyTitle])) {
        d->queryFreetext.append(encodeURL(queryFragment));
    }
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyYear])) {
        d->queryFreetext.append(encodeURL(queryFragment));
    }
    d->queryAuthor.clear();
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyAuthor])) {
        d->queryAuthor.append(encodeURL(queryFragment));
    }
    d->numExpectedResults = numResults;

    ++d->runningJobs;
    QNetworkRequest request(d->scienceDirectBaseUrl);
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
}

QString WebSearchScienceDirect::label() const
{
    return i18n("ScienceDirect");
}

QString WebSearchScienceDirect::favIconUrl() const
{
    return QLatin1String("http://www.sciencedirect.com/scidirimg/faviconSD.ico");
}

WebSearchQueryFormAbstract* WebSearchScienceDirect::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchScienceDirect::homepage() const
{
    return KUrl("http://www.sciencedirect.com/");
}

void WebSearchScienceDirect::cancel()
{
    WebSearchAbstract::cancel();
}

void WebSearchScienceDirect::doneFetchingStartPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs == 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        const QString htmlText = reply->readAll();
        static_cast<HTTPEquivCookieJar*>(networkAccessManager()->cookieJar())->checkForHttpEqiuv(htmlText, reply->url());
        KUrl url(d->scienceDirectBaseUrl + "science");
        QMap<QString, QString> inputMap = formParameters(htmlText, QLatin1String("<form name=\"qkSrch\""));
        inputMap.remove("qs_all");
        inputMap.remove("qs_author");
        //inputMap.remove("resultsPerPage");
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addEncodedQueryItem(it.key().toAscii(), QString(it.value()).replace(" ", "+").toAscii());
        url.addEncodedQueryItem(QString("qs_all").toAscii(), d->queryFreetext.join("+").toAscii());
        url.addEncodedQueryItem(QString("qs_author").toAscii(), d->queryAuthor.join("+").toAscii());
        //url.addQueryItem(QString("resultsPerPage").toAscii(), QString::number(d->numExpectedResults).toAscii());

        ++d->runningJobs;
        QNetworkRequest request(url);
        setSuggestedHttpHeaders(request, reply);
        QNetworkReply *newReply = networkAccessManager()->get(request);
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchScienceDirect::doneFetchingResultPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs == 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        const QString htmlText = reply->readAll();
        static_cast<HTTPEquivCookieJar*>(networkAccessManager()->cookieJar())->checkForHttpEqiuv(htmlText, reply->url());
        int p = -1;
        while ((p = htmlText.indexOf("http://www.sciencedirect.com/science/article/pii/", p + 1)) >= 0) {
            int p2 = htmlText.indexOf("\"", p + 1);
            ++d->runningJobs;
            QNetworkRequest request(htmlText.mid(p, p2 - p));
            setSuggestedHttpHeaders(request, reply);
            QNetworkReply *newReply = networkAccessManager()->get(request);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstractPage()));
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchScienceDirect::doneFetchingAbstractPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        const QString htmlText = reply->readAll();
        static_cast<HTTPEquivCookieJar*>(networkAccessManager()->cookieJar())->checkForHttpEqiuv(htmlText, reply->url());
        int p1, p2;
        if ((p1 = htmlText.indexOf("/science?_ob=DownloadURL&")) >= 0 && (p2 = htmlText.indexOf("\"", p1 + 1)) >= 0) {
            KUrl url("http://www.sciencedirect.com" + htmlText.mid(p1, p2 - p1));
            ++d->runningJobs;
            QNetworkRequest request(url);
            setSuggestedHttpHeaders(request, reply);
            QNetworkReply *newReply = networkAccessManager()->get(request);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingExportCitationPage()));
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchScienceDirect::doneFetchingExportCitationPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        const QString htmlText = reply->readAll();
        static_cast<HTTPEquivCookieJar*>(networkAccessManager()->cookieJar())->checkForHttpEqiuv(htmlText, reply->url());
        QMap<QString, QString> inputMap = formParameters(htmlText, QLatin1String("<form name=\"exportCite\""));
        inputMap.remove("format");
        inputMap.remove("citation-type");

        QString body = "format=cite-abs&citation-type=BIBTEX";
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it) {
            body += '&' + encodeURL(it.key()) + '=' + encodeURL(it.value());
        }

        ++d->runningJobs;
        QNetworkRequest request(KUrl("http://www.sciencedirect.com/science"));
        setSuggestedHttpHeaders(request, reply);
        QNetworkReply *newReply = networkAccessManager()->post(request, body.toUtf8());
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchScienceDirect::doneFetchingBibTeX()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        ts.setCodec("ISO 8859-1");
        QString bibTeXcode = ts.readAll();
        d->sanitizeBibTeXCode(bibTeXcode);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL)
                    emit foundEntry(entry);
            }
            delete bibtexFile;
        } else {
            emit stoppedSearch(resultUnspecifiedError);
            return;
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}
