/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "findpdf.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegExp>
#include <QApplication>
#include <QTemporaryFile>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>

#include <poppler/qt5/poppler-qt5.h>

#include "kbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "value.h"
#include "fileinfo.h"
#include "logging_networking.h"

int maxDepth = 5;
static const char *depthProperty = "depth";
static const char *termProperty = "term";
static const char *originProperty = "origin";


class FindPDF::Private
{
private:
    FindPDF *p;

public:
    int aliveCounter;
    QList<ResultItem> result;
    Entry currentEntry;
    QSet<QUrl> knownUrls;
    QSet<QNetworkReply *> runningDownloads;

    Private(FindPDF *parent)
            : p(parent), aliveCounter(0)
    {
        /// nothing
    }

    bool queueUrl(const QUrl &url, const QString &term, const QString &origin, int depth)
    {
        if (!knownUrls.contains(url) && depth > 0) {
            knownUrls.insert(url);
            QNetworkRequest request = QNetworkRequest(url);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply, 15); ///< set a timeout on network connections
            reply->setProperty(depthProperty, QVariant::fromValue<int>(depth));
            reply->setProperty(termProperty, term);
            reply->setProperty(originProperty, origin);
            runningDownloads.insert(reply);
            connect(reply, &QNetworkReply::finished, p, &FindPDF::downloadFinished);
            ++aliveCounter;
            return true;
        } else
            return false;
    }

    void processGeneralHTML(QNetworkReply *reply, const QString &text)
    {
        /// fetch some properties from Reply object
        const QString term = reply->property(termProperty).toString();
        const QString origin = reply->property(originProperty).toString();
        bool ok = false;
        int depth = reply->property(depthProperty).toInt(&ok);
        if (!ok) depth = 0;

        /// regular expressions to guess links to follow
        static const QRegExp anchorRegExp[5] = {
            QRegExp(QString(QStringLiteral("<a[^>]*href=\"([^\"]*%1[^\"]*[.]pdf)\"")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
            QRegExp(QString(QStringLiteral("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*%1[^<]*[.]pdf")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
            QRegExp(QString(QStringLiteral("<a[^>]*href=\"([^\"]*%1[^\"]*)\"")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
            QRegExp(QString(QStringLiteral("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*%1[^<]*\\b")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
            QRegExp(QStringLiteral("<a[^>]*href=\"([^\"]+)\""), Qt::CaseInsensitive) /// any link
        };

        bool gotLink = false;
        for (int i = 0; !gotLink && i < 4; ++i) {
            if (anchorRegExp[i].indexIn(text) >= 0) {
                const QUrl url = QUrl::fromEncoded(anchorRegExp[i].cap(1).toLatin1());
                queueUrl(reply->url().resolved(url), term, origin, depth - 1);
                gotLink = true;
            }
        }

        if (!gotLink && text.count(anchorRegExp[4]) == 1) {
            /// this is only the last resort:
            /// to follow the first link found in the HTML document
            if (anchorRegExp[4].indexIn(text) >= 0) {
                const QUrl url = QUrl::fromEncoded(anchorRegExp[4].cap(1).toLatin1());
                queueUrl(reply->url().resolved(url), term, origin, depth - 1);
            }
        }
    }

    void processGoogleResult(QNetworkReply *reply, const QString &text)
    {
        static const QString h3Tag(QStringLiteral("<h3"));
        static const QString aTag(QStringLiteral("<a"));
        static const QString hrefAttrib(QStringLiteral("href=\""));

        const QString term = reply->property(termProperty).toString();
        bool ok = false;
        int depth = reply->property(depthProperty).toInt(&ok);
        if (!ok) depth = 0;

        /// extract the first numHitsToFollow-many hits found by Google Scholar
        const int numHitsToFollow = 10;
        int p = -1;
        for (int i = 0; i < numHitsToFollow; ++i) {
            if ((p = text.indexOf(h3Tag, p + 1)) >= 0 && (p = text.indexOf(aTag, p + 1)) >= 0 && (p = text.indexOf(hrefAttrib, p + 1)) >= 0) {
                int p1 = p + 6;
                int p2 = text.indexOf(QLatin1Char('"'), p1 + 1);
                QUrl url(text.mid(p1, p2 - p1));
                const QString googleService = reply->url().host().contains(QStringLiteral("scholar.google")) ? QStringLiteral("scholar.google") : QStringLiteral("www.google");
                queueUrl(reply->url().resolved(url), term, googleService, depth - 1);
            }
        }
    }

    void processSpringerLink(QNetworkReply *reply, const QString &text)
    {
        static const QRegExp fulltextPDFlink(QStringLiteral("href=\"([^\"]+/fulltext.pdf)\""));

        if (fulltextPDFlink.indexIn(text) > 0) {
            bool ok = false;
            int depth = reply->property(depthProperty).toInt(&ok);
            if (!ok) depth = 0;

            QUrl url(fulltextPDFlink.cap(1));
            queueUrl(reply->url().resolved(url), QString(), QStringLiteral("springerlink"), depth - 1);
        }
    }

    void processCiteSeerX(QNetworkReply *reply, const QString &text)
    {
        static const QRegExp downloadPDFlink(QStringLiteral("href=\"(/viewdoc/download[^\"]+type=pdf)\""));

        if (downloadPDFlink.indexIn(text) > 0) {
            bool ok = false;
            int depth = reply->property(depthProperty).toInt(&ok);
            if (!ok) depth = 0;

            QUrl url(QUrl::fromEncoded(downloadPDFlink.cap(1).toLatin1()));
            queueUrl(reply->url().resolved(url), QString(), QStringLiteral("citeseerx"), depth - 1);
        }
    }

    bool processPDF(QNetworkReply *reply, const QByteArray &data)
    {
        bool progress = false;
        const QString origin = reply->property(originProperty).toString();
        const QUrl url = reply->url();

        /// Search for duplicate URLs
        bool containsUrl = false;
        foreach (const ResultItem &ri, result) {
            containsUrl |= ri.url == url;
            /// Skip already visited URLs
            if (containsUrl) break;
        }

        if (!containsUrl) {
            Poppler::Document *doc = Poppler::Document::loadFromData(data);

            ResultItem resultItem;
            resultItem.tempFilename = new QTemporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QStringLiteral("kbibtex_findpdf_XXXXXX.pdf"));
            resultItem.tempFilename->setAutoRemove(true);
            if (resultItem.tempFilename->open()) {
                const int lenDataWritten = resultItem.tempFilename->write(data);
                resultItem.tempFilename->close();
                if (lenDataWritten != data.length()) {
                    /// Failed to write to temporary file
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to write to temporary file for filename" << resultItem.tempFilename->fileName();
                    delete resultItem.tempFilename;
                    resultItem.tempFilename = NULL;
                }
            } else {
                /// Failed to create temporary file
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to create temporary file for templaet" << resultItem.tempFilename->fileTemplate();
                delete resultItem.tempFilename;
                resultItem.tempFilename = NULL;
            }
            resultItem.url = url;
            resultItem.textPreview = doc->info(QStringLiteral("Title")).simplified();
            static const int maxTextLen = 1024;
            for (int i = 0; i < doc->numPages() && resultItem.textPreview.length() < maxTextLen; ++i) {
                Poppler::Page *page = doc->page(i);
                if (!resultItem.textPreview.isEmpty()) resultItem.textPreview += QLatin1Char(' ');
                resultItem.textPreview += page->text(QRect()).simplified().leftRef(maxTextLen);
                delete page;
            }
            resultItem.textPreview.remove(QStringLiteral("Microsoft Word - ")); ///< Some word processors need to put their name everywhere ...
            resultItem.downloadMode = NoDownload;
            resultItem.relevance = origin == Entry::ftDOI ? 1.0 : (origin == QStringLiteral("eprint") ? 0.75 : 0.5);
            result << resultItem;
            progress = true;

            delete doc;
        }

        return progress;
    }
};

FindPDF::FindPDF(QObject *parent)
        : QObject(parent), d(new Private(this))
{
    /// nothing
}

FindPDF::~FindPDF()
{
    abort();
    delete d;
}

bool FindPDF::search(const Entry &entry)
{
    if (d->aliveCounter > 0) return false;

    d->knownUrls.clear();
    d->result.clear();
    d->currentEntry = entry;

    emit progress(0, d->aliveCounter, 0);

    /// Generate a string which contains the title's beginning
    QString searchWords;
    if (entry.contains(Entry::ftTitle)) {
        const QStringList titleChunks = PlainTextValue::text(entry.value(Entry::ftTitle)).split(QStringLiteral(" "), QString::SkipEmptyParts);
        if (!titleChunks.isEmpty()) {
            searchWords = titleChunks[0];
            for (int i = 1; i < titleChunks.count() && searchWords.length() < 64; ++i)
                searchWords += QLatin1Char(' ') + titleChunks[i];
        }
    }
    const QStringList authors = entry.authorsLastName();
    for (int i = 0; i < authors.count() && searchWords.length() < 96; ++i)
        searchWords += QLatin1Char(' ') + authors[i];

    QStringList urlFields = QStringList() << Entry::ftDOI << Entry::ftUrl << QStringLiteral("ee");
    for (int i = 2; i < 256; ++i)
        urlFields << QString(QStringLiteral("%1%2")).arg(Entry::ftDOI).arg(i) << QString(QStringLiteral("%1%2")).arg(Entry::ftUrl).arg(i);
    foreach (const QString &field, urlFields) {
        if (entry.contains(field)) {
            const QString fieldText = PlainTextValue::text(entry.value(field));
            int p = -1;
            while ((p = KBibTeX::doiRegExp.indexIn(fieldText, p + 1)) >= 0)
                d->queueUrl(QUrl(FileInfo::doiUrlPrefix() + KBibTeX::doiRegExp.cap(0)), fieldText, Entry::ftDOI, maxDepth);

            p = -1;
            while ((p = KBibTeX::urlRegExp.indexIn(fieldText, p + 1)) >= 0)
                d->queueUrl(QUrl(KBibTeX::urlRegExp.cap(0)), searchWords, Entry::ftUrl, maxDepth);
        }
    }

    if (entry.contains(QStringLiteral("eprint"))) {
        /// check eprint fields as used for arXiv
        const QString fieldText = PlainTextValue::text(entry.value(QStringLiteral("eprint")));
        if (!fieldText.isEmpty())
            d->queueUrl(QUrl(QString(QStringLiteral("http://arxiv.org/find/all/1/all:+%1/0/1/0/all/0/1")).arg(fieldText)), fieldText, QStringLiteral("eprint"), maxDepth);
    }

    if (!searchWords.isEmpty()) {
        /// Search in Google Scholar
        QUrl googleUrl(QStringLiteral("https://www.google.com/search?hl=en&sa=G"));
        QUrlQuery query(googleUrl);
        query.addQueryItem(QStringLiteral("q"), searchWords + QStringLiteral(" filetype:pdf"));
        googleUrl.setQuery(query);
        d->queueUrl(googleUrl, searchWords, QStringLiteral("www.google"), maxDepth);

        /// Search in Google Scholar
        QUrl googleScholarUrl(QStringLiteral("https://scholar.google.com/scholar?hl=en&btnG=Search&as_sdt=1"));
        query = QUrlQuery(googleScholarUrl);
        query.addQueryItem(QStringLiteral("q"), searchWords + QStringLiteral(" filetype:pdf"));
        googleScholarUrl.setQuery(query);
        d->queueUrl(googleScholarUrl, searchWords, QStringLiteral("scholar.google"), maxDepth);

        /// Search in Bing
        QUrl bingUrl(QStringLiteral("https://www.bing.com/search?setlang=en-US"));
        query = QUrlQuery(bingUrl);
        query.addQueryItem(QStringLiteral("q"), searchWords + QStringLiteral(" filetype:pdf"));
        bingUrl.setQuery(query);
        d->queueUrl(bingUrl, searchWords, QStringLiteral("bing"), maxDepth);

        /// Search in Microsoft Academic Search
        QUrl masUrl(QStringLiteral("http://academic.research.microsoft.com/Search"));
        query = QUrlQuery(masUrl);
        query.addQueryItem(QStringLiteral("query"), searchWords);
        masUrl.setQuery(query);
        d->queueUrl(masUrl, searchWords, QStringLiteral("academicsearch"), maxDepth);

        /// Search in CiteSeerX
        QUrl citeseerXurl(QStringLiteral("http://citeseerx.ist.psu.edu/search?submit=Search&sort=rlv&t=doc"));
        query = QUrlQuery(citeseerXurl);
        query.addQueryItem(QStringLiteral("q"), searchWords);
        citeseerXurl.setQuery(query);
        d->queueUrl(citeseerXurl, searchWords, QStringLiteral("citeseerx"), maxDepth);

        /// Search in StartPage
        QUrl startPageUrl(QStringLiteral("https://www.startpage.com/do/asearch?cat=web&cmd=process_search&language=english&engine0=v1all&abp=-1&t=white&nj=1&prf=23ad6aab054a88d3da5c443280cee596&suggestOn=0"));
        query = QUrlQuery(startPageUrl);
        query.addQueryItem(QStringLiteral("query"), searchWords + QStringLiteral(" filetype:pdf"));
        d->queueUrl(startPageUrl, searchWords, QStringLiteral("startpage"), maxDepth);

        /// Search in arXiv
        QUrl arXivUrl = QUrl::fromUserInput(QString(QStringLiteral("http://arxiv.org/find/all/1/all:+%1/0/1/0/all/0/1")).arg(searchWords));
        d->queueUrl(arXivUrl, searchWords, QStringLiteral("arxiv"), maxDepth);
    }

    if (d->aliveCounter == 0) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Directly at start, no URLs are queue for a search -> this should never happen";
        emit finished();
    }

    return true;
}

QList<FindPDF::ResultItem> FindPDF::results()
{
    if (d->aliveCounter == 0)
        return d->result;
    else {
        /// Return empty list while search is running
        return QList<FindPDF::ResultItem>();
    }
}

void FindPDF::abort() {
    QSet<QNetworkReply *>::Iterator it = d->runningDownloads.begin();
    while (it != d->runningDownloads.end()) {
        QNetworkReply *reply = *it;
        it = d->runningDownloads.erase(it);
        reply->abort();
    }
}

void FindPDF::downloadFinished()
{
    static const char *htmlHead1 = "<html", *htmlHead2 = "<HTML";
    static const char *pdfHead = "%PDF-";

    --d->aliveCounter;
    emit progress(d->knownUrls.count(), d->aliveCounter, d->result.count());

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    d->runningDownloads.remove(reply);
    const QString term = reply->property(termProperty).toString();
    const QString origin = reply->property(originProperty).toString();
    bool depthOk = false;
    int depth = reply->property(depthProperty).toInt(&depthOk);
    if (!depthOk) depth = 0;

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();

        QUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        redirUrl = redirUrl.isEmpty() ? QUrl() : reply->url().resolved(redirUrl);
        qCDebug(LOG_KBIBTEX_NETWORKING) << "finished Downloading " << reply->url().toString() << "   depth=" << depth  << "  d->aliveCounter=" << d->aliveCounter << "  data.size=" << data.size() << "  redirUrl=" << redirUrl.toString() << "   origin=" << origin;

        if (!redirUrl.isEmpty())
            d->queueUrl(redirUrl, term, origin, depth - 1);
        else if (data.contains(htmlHead1) || data.contains(htmlHead2)) {
            /// returned data is a HTML file, i.e. contains "<html"

            /// check for limited depth before continuing
            if (depthOk && depth > 0) {
                /// Get webpage as plain text
                /// Assume UTF-8 data
                const QString text = QString::fromUtf8(data.constData());

                /// regular expression to check if this is a Google Scholar result page
                static const QRegExp googleScholarTitleRegExp(QStringLiteral("<title>[^>]* - Google Scholar</title>"));
                /// regular expression to check if this is a SpringerLink page
                static const QRegExp springerLinkTitleRegExp(QStringLiteral("<title>[^>]* - Springer - [^>]*</title>"));
                /// regular expression to check if this is a CiteSeerX page
                static const QRegExp citeseerxTitleRegExp(QStringLiteral("<title>CiteSeerX &mdash; [^>]*</title>"));

                if (text.indexOf(googleScholarTitleRegExp) > 0)
                    d->processGoogleResult(reply, text);
                else if (text.indexOf(springerLinkTitleRegExp) > 0)
                    d->processSpringerLink(reply, text);
                else if (text.indexOf(citeseerxTitleRegExp) > 0)
                    d->processCiteSeerX(reply, text);
                else {
                    /// regular expression to extract title
                    static QRegExp titleRegExp(QStringLiteral("<title>(.*)</title>"));
                    titleRegExp.setMinimal(true);
                    if (titleRegExp.indexIn(text) >= 0)
                        qCDebug(LOG_KBIBTEX_NETWORKING) << "Using general HTML processor for page" << titleRegExp.cap(1) << " URL=" << reply->url().toDisplayString();
                    else
                        qCDebug(LOG_KBIBTEX_NETWORKING) << "Using general HTML processor for URL=" << reply->url().toDisplayString();
                    d->processGeneralHTML(reply, text);
                }
            }
        } else if (data.contains(pdfHead)) {
            /// looks like a PDF file -> grab it
            const bool gotPDFfile = d->processPDF(reply, data);
            if (gotPDFfile)
                emit progress(d->knownUrls.count(), d->aliveCounter, d->result.count());
        } else {
            /// Assume UTF-8 data
            const QString text = QString::fromUtf8(data.constData());
            qCWarning(LOG_KBIBTEX_NETWORKING) << "don't know how to handle " << text.left(256);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "error from reply: " << reply->errorString() << "(" << reply->url().toString() << ")" << "  term=" << term << "  origin=" << origin << "  depth=" << depth;

    if (d->aliveCounter == 0) {
        /// no more running downloads left
        emit finished();
    }
}
