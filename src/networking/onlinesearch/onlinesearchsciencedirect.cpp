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

#include <KDebug>
#include <KLocale>

#include <encoderlatex.h>
#include "fileimporterbibtex.h"
#include "onlinesearchsciencedirect.h"
#include <internalnetworkaccessmanager.h>

class OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate
{
private:
    OnlineSearchScienceDirect *p;

public:
    QString queryFreetext, queryAuthor;
    int currentSearchPosition;
    int numExpectedResults, numFoundResults;
    const QString scienceDirectBaseUrl;
    QStringList bibTeXUrls;
    int runningJobs;
    int numSteps, curStep;

    OnlineSearchScienceDirectPrivate(OnlineSearchScienceDirect *parent)
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

OnlineSearchScienceDirect::OnlineSearchScienceDirect(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchScienceDirectPrivate(this))
{
    // nothing
}

OnlineSearchScienceDirect::~OnlineSearchScienceDirect()
{
    delete d;
}

void OnlineSearchScienceDirect::startSearch()
{
    d->runningJobs = 0;
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

void OnlineSearchScienceDirect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->runningJobs = 0;
    d->numFoundResults = 0;
    m_hasBeenCanceled = false;
    d->bibTeXUrls.clear();
    d->currentSearchPosition = 0;
    d->curStep = 0;
    d->numSteps = 2 + 3 * numResults;

    d->queryFreetext = query[queryKeyFreeText] + ' ' + query[queryKeyTitle] + ' ' + query[queryKeyYear];
    d->queryAuthor = query[queryKeyAuthor];
    d->numExpectedResults = numResults;

    ++d->runningJobs;
    QNetworkRequest request(d->scienceDirectBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));

    emit progress(0, d->numSteps);
}

QString OnlineSearchScienceDirect::label() const
{
    return i18n("ScienceDirect");
}

QString OnlineSearchScienceDirect::favIconUrl() const
{
    return QLatin1String("http://www.sciencedirect.com/scidirimg/faviconSD.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchScienceDirect::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchScienceDirect::homepage() const
{
    return KUrl("http://www.sciencedirect.com/");
}

void OnlineSearchScienceDirect::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchScienceDirect::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    --d->runningJobs;
    Q_ASSERT(d->runningJobs == 0);

    QUrl redirUrl;
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply, redirUrl)) {
        const QString htmlText = reply->readAll();

        if (redirUrl.isValid()) {
            ++d->numSteps;
            ++d->runningJobs;

            /// redirection to another url
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply->url());
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
        } else {
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            KUrl url(d->scienceDirectBaseUrl + "science");
            QMap<QString, QString> inputMap = formParameters(htmlText, QLatin1String("<form name=\"qkSrch\""));
            inputMap["qs_all"] = d->queryFreetext.trimmed();
            inputMap["qs_author"] = d->queryAuthor.trimmed();
            inputMap["resultsPerPage"] = QString::number(d->numExpectedResults);
            inputMap["_ob"] = "QuickSearchURL";
            inputMap["_method"] = "submitForm";
            inputMap["sdSearch"] = "Search";

            static const QStringList orderOfParameters = QString("_ob|_method|_acct|_origin|_zone|md5|_eidkey|qs_issue|qs_pages|qs_title|qs_vol|sdSearch|qs_all|qs_author|resultsPerPage").split("|");
            foreach(const QString &key, orderOfParameters) {
                if (!inputMap.contains(key)) continue;
                url.addQueryItem(key, inputMap[key]);
            }

            ++d->runningJobs;
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
            setNetworkReplyTimeout(newReply);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingResultPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs == 0);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        KUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirUrl.isEmpty()) {
            ++d->runningJobs;
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
            setNetworkReplyTimeout(newReply);
        } else {
            emit progress(++d->curStep, d->numSteps);

            const QString htmlText = reply->readAll();
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            int p = -1, p2 = -1;
            while ((p = htmlText.indexOf("http://www.sciencedirect.com/science/article/pii/", p + 1)) >= 0 && (p2 = htmlText.indexOf("\"", p + 1)) >= 0)
                if (d->numFoundResults < d->numExpectedResults) {
                    ++d->numFoundResults;
                    ++d->runningJobs;
                    KUrl url(htmlText.mid(p, p2 - p));
                    QNetworkRequest request(url);
                    QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                    setNetworkReplyTimeout(newReply);
                    connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstractPage()));
                }
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingAbstractPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        KUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirUrl.isEmpty()) {
            ++d->runningJobs;
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstractPage()));
            setNetworkReplyTimeout(newReply);
        } else {
            emit progress(++d->curStep, d->numSteps);

            const QString htmlText = reply->readAll();
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            int p1 = -1, p2 = -1;
            if ((p1 = htmlText.indexOf("/science?_ob=DownloadURL&")) >= 0 && (p2 = htmlText.indexOf("\"", p1 + 1)) >= 0) {
                KUrl url("http://www.sciencedirect.com" + htmlText.mid(p1, p2 - p1));
                ++d->runningJobs;
                QNetworkRequest request(url);
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingExportCitationPage()));
                setNetworkReplyTimeout(newReply);
            }
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingExportCitationPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        KUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirUrl.isEmpty()) {
            ++d->runningJobs;
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingExportCitationPage()));
            setNetworkReplyTimeout(newReply);
        } else {
            emit progress(++d->curStep, d->numSteps);

            const QString htmlText = reply->readAll();
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            QMap<QString, QString> inputMap = formParameters(htmlText, QLatin1String("<form name=\"exportCite\""));
            inputMap["format"] = "cite";
            inputMap["citation-type"] = "BIBTEX";
            inputMap["RETURN_URL"] = d->scienceDirectBaseUrl + "/science/home";
            static const QStringList orderOfParameters = QString("_ob|_method|_acct|_userid|_docType|_eidkey|_ArticleListID|_uoikey|count|md5|JAVASCRIPT_ON|format|citation-type|Export|RETURN_URL").split("|");

            QString body;
            foreach(const QString &key, orderOfParameters) {
                if (!inputMap.contains(key)) continue;
                if (!body.isEmpty()) body += '&';
                body += encodeURL(key) + '=' + encodeURL(inputMap[key]);
            }

            ++d->runningJobs;
            QNetworkRequest request(KUrl(d->scienceDirectBaseUrl + "/science"));
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->post(request, body.toUtf8());
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        QString bibTeXcode = ts.readAll();
        d->sanitizeBibTeXCode(bibTeXcode);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntry = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (!entry.isNull()) {
                    hasEntry = true;
                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                    entry->insert("x-fetchedfrom", v);
                    emit foundEntry(entry);
                }
            }
            delete bibtexFile;
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(hasEntry ? resultNoError : resultUnspecifiedError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
