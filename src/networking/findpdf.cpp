/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "findpdf.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QApplication>
#include <QTemporaryFile>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#ifdef HAVE_SCIHUB
#include <QRandomGenerator>
#endif // HAVE_SCIHUB

#ifdef HAVE_POPPLERQT5
#include <poppler-qt5.h>
#else // not HAVE_POPPLERQT5
#ifdef HAVE_POPPLERQT6
#include <poppler-qt6.h>
#endif // HAVE_POPPLERQT6
#endif // HAVE_POPPLERQT5

#include <KBibTeX>
#include <Value>
#include <FileInfo>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

static const int maxDepth = 5;
static const char *depthProperty = "depth";
static const char *termProperty = "term";
static const char *originProperty = "origin";


class FindPDF::Private
{
private:
    FindPDF *p;

    /**
     * @brief Remove the fragment part of an URL (i.e. everything starting from '#')
     * @param u QUrl which may contain a fragment part or not
     * @return QUrl with the fragment part removed if there was any
     */
    inline QUrl removeFragment(const QUrl &u) {
        if (u.hasFragment()) {
            QUrl _u{u};
            _u.setFragment(QString());
            return _u;
        } else
            return u;
    }

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
        const QUrl sanitizedUrl{removeFragment(url)};

        if (!knownUrls.contains(sanitizedUrl) && depth > 0) {
            knownUrls.insert(sanitizedUrl);
            QNetworkRequest request = QNetworkRequest(sanitizedUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply, 15); ///< set a timeout on network connections
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
        const QUrl baseUrl{reply->url()};
        bool ok = false;
        int depth = reply->property(depthProperty).toInt(&ok);
        if (!ok) depth = 0;

        /// regular expressions to guess links to follow
        const QVector<QRegularExpression> specificAnchorRegExp = {
            QRegularExpression(QString(QStringLiteral("<a[^>]*href=\"([^\"]*%1[^\"]*[.]pdf)\"")).arg(QRegularExpression::escape(term)), QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(QString(QStringLiteral("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*%1[^<]*[.]pdf")).arg(QRegularExpression::escape(term)), QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(QString(QStringLiteral("<a[^>]*href=\"([^\"]*%1[^\"]*)\"")).arg(QRegularExpression::escape(term)), QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(QString(QStringLiteral("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*%1[^<]*\\b")).arg(QRegularExpression::escape(term)), QRegularExpression::CaseInsensitiveOption)
        };
        static const QRegularExpression genericAnchorRegExp = QRegularExpression(QStringLiteral("<a[^>]*href=\"([^\"]+)\""), QRegularExpression::CaseInsensitiveOption);

        bool gotLink = false;
        for (const QRegularExpression &anchorRegExp : specificAnchorRegExp) {
            const QRegularExpressionMatch match = anchorRegExp.match(text);
            if (match.hasMatch()) {
                const QUrl url = QUrl::fromEncoded(match.captured(1).toLatin1());
                queueUrl(baseUrl.resolved(url), term, origin, depth - 1);
                gotLink = true;
                break;
            }
        }

        if (!gotLink) {
            /// this is only the last resort:
            /// to follow the first link found in the HTML document
            const QRegularExpressionMatch match = genericAnchorRegExp.match(text);
            if (match.hasMatch()) {
                const QUrl url = QUrl::fromEncoded(match.captured(1).toLatin1());
                queueUrl(baseUrl.resolved(url), term, origin, depth - 1);
            }
        }

        if (baseUrl.isValid()) {
            int p1 = text.indexOf(QStringLiteral("<embed"));
            while (p1 > 0) {
                const int p2 = text.indexOf(QStringLiteral(">"), p1 + 5);
                if (p2 > p1) {
                    const int p3 = text.indexOf(QStringLiteral("type=\"application/pdf\""), p1 + 5);
                    if (p3 > p1 && p3 < p2) {
                        const int p4 = text.indexOf(QStringLiteral("src=\""), p1 + 5);
                        if (p4 > p1 && p4 < p2) {
                            const int p5 = text.indexOf(QStringLiteral("\""), p4 + 5);
                            if (p5 > p4 && p5 < p2) {
                                const QString src{text.mid(p4 + 5, p5 - p4)};
                                QUrl nextUrl;
                                if (src.startsWith(QStringLiteral("http")))
                                    nextUrl = QUrl::fromUserInput(src);
                                else if (src.startsWith(QStringLiteral("//")))
                                    nextUrl = QUrl::fromUserInput(baseUrl.scheme() + QStringLiteral(":") + src);
                                else if (src.startsWith(QStringLiteral("/")))
                                    nextUrl = QUrl::fromUserInput(baseUrl.scheme() + QStringLiteral("://") + baseUrl.host() + src);
                                else
                                    nextUrl = baseUrl.resolved(QUrl(src));
                                queueUrl(nextUrl, term, origin, depth - 1);
                            }
                        }
                    }
                    p1 = text.indexOf(QStringLiteral("<embed"), p2);
                } else
                    break;
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
        static const QRegularExpression fulltextPDFlink(QStringLiteral("href=\"([^\"]+/fulltext.pdf)\""));
        const QRegularExpressionMatch match = fulltextPDFlink.match(text);
        if (match.hasMatch()) {
            bool ok = false;
            int depth = reply->property(depthProperty).toInt(&ok);
            if (!ok) depth = 0;

            const QUrl url(match.captured(1));
            queueUrl(reply->url().resolved(url), QString(), QStringLiteral("springerlink"), depth - 1);
        }
    }

    void processCiteSeerX(QNetworkReply *reply, const QString &text)
    {
        static const QRegularExpression downloadPDFlink(QStringLiteral("href=\"(/viewdoc/download[^\"]+type=pdf)\""));
        const QRegularExpressionMatch match = downloadPDFlink.match(text);
        if (match.hasMatch()) {
            bool ok = false;
            int depth = reply->property(depthProperty).toInt(&ok);
            if (!ok) depth = 0;

            const QUrl url = QUrl::fromEncoded(match.captured(1).toLatin1());
            queueUrl(reply->url().resolved(url), QString(), QStringLiteral("citeseerx"), depth - 1);
        }
    }

    void processACMDigitalLibrary(QNetworkReply *reply, const QString &text)
    {
        static const QRegularExpression downloadPDFlink(QStringLiteral("href=\"(ft_gateway.cfm\\?id=\\d+&ftid=\\d+&dwn=1&CFID=\\d+&CFTOKEN=\\d+)\""));
        const QRegularExpressionMatch match = downloadPDFlink.match(text);
        if (match.hasMatch()) {
            bool ok = false;
            int depth = reply->property(depthProperty).toInt(&ok);
            if (!ok) depth = 0;

            const QUrl url = QUrl::fromEncoded(match.captured(1).toLatin1());
            queueUrl(reply->url().resolved(url), QString(), QStringLiteral("acmdl"), depth - 1);
        }
    }

    bool processPDF(QNetworkReply *reply, const QByteArray &data)
    {
        bool progress = false;
        const QString origin = reply->property(originProperty).toString();
        const QUrl url = reply->url();

        /// Search for duplicate URLs
        bool containsUrl = false;
        for (const ResultItem &ri :  const_cast<const QList<ResultItem> &>(result)) {
            containsUrl |= ri.url == url;
            /// Skip already visited URLs
            if (containsUrl) break;
        }

        if (!containsUrl) {
#ifdef HAVE_POPPLERQT5
            QScopedPointer<Poppler::Document> doc(Poppler::Document::loadFromData(data));
#else // not HAVE_POPPLERQT5
#ifdef HAVE_POPPLERQT6
            std::unique_ptr<Poppler::Document> doc = Poppler::Document::loadFromData(data);
#endif // HAVE_POPPLERQT6
#endif // HAVE_POPPLERQT5

            ResultItem resultItem;
            resultItem.tempFilename = new QTemporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QStringLiteral("kbibtex_findpdf_XXXXXX.pdf"));
            resultItem.tempFilename->setAutoRemove(true);
            if (resultItem.tempFilename->open()) {
                const int lenDataWritten = static_cast<int>(resultItem.tempFilename->write(data));
                resultItem.tempFilename->close();
                if (lenDataWritten != data.length()) {
                    /// Failed to write to temporary file
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to write to temporary file for filename" << resultItem.tempFilename->fileName();
                    delete resultItem.tempFilename;
                    resultItem.tempFilename = nullptr;
                }
            } else {
                /// Failed to create temporary file
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to create temporary file for templaet" << resultItem.tempFilename->fileTemplate();
                delete resultItem.tempFilename;
                resultItem.tempFilename = nullptr;
            }
            resultItem.url = url;
            resultItem.textPreview = doc->info(QStringLiteral("Title")).simplified();
            static const int maxTextLen = 1024;
            for (int i = 0; i < doc->numPages() && resultItem.textPreview.length() < maxTextLen; ++i) {
#ifdef HAVE_POPPLERQT5
                QScopedPointer<Poppler::Page> page(doc->page(i));
#else // not HAVE_POPPLERQT5
#ifdef HAVE_POPPLERQT6
                std::unique_ptr<Poppler::Page> page = doc->page(i);
#endif // HAVE_POPPLERQT6
#endif // HAVE_POPPLERQT5
                if (!resultItem.textPreview.isEmpty()) resultItem.textPreview += QLatin1Char(' ');
                resultItem.textPreview += QStringView{page->text(QRect()).simplified()}.left(maxTextLen);
            }
            resultItem.textPreview.remove(QStringLiteral("Microsoft Word - ")); ///< Some word processors need to put their name everywhere ...
            resultItem.downloadMode = DownloadMode::No;
            resultItem.relevance = origin == Entry::ftDOI ? 1.0 : (origin == QStringLiteral("eprint") ? 0.75 : 0.5);
            result << resultItem;
            progress = true;
        }

        return progress;
    }

    QUrl ieeeDocumentUrlToDownloadUrl(const QUrl &url) {
        /// Basic checking if provided URL is from IEEE Xplore
        if (!url.host().contains(QStringLiteral("ieeexplore.ieee.org")))
            return url;

        /// Assuming URL looks like this:
        ///    https://ieeexplore.ieee.org/document/8092651
        static const QRegularExpression documentIdRegExp(QStringLiteral("/(\\d{6,})[/]?$"));
        const QRegularExpressionMatch documentIdRegExpMatch = documentIdRegExp.match(url.path());
        if (!documentIdRegExpMatch.hasMatch())
            return url;

        /// Use document id extracted above to build URL to PDF file
        return QUrl(QStringLiteral("https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=") + documentIdRegExpMatch.captured(1));
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

    Q_EMIT progress(0, d->aliveCounter, 0);

    /// Generate a string which contains the title's beginning
    QString searchWords;
    if (entry.contains(Entry::ftTitle)) {
#if QT_VERSION >= 0x050e00
        const QStringList titleChunks = PlainTextValue::text(entry.value(Entry::ftTitle)).split(QStringLiteral(" "), Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
        const QStringList titleChunks = PlainTextValue::text(entry.value(Entry::ftTitle)).split(QStringLiteral(" "), QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
        if (!titleChunks.isEmpty()) {
            searchWords = titleChunks[0];
            for (int i = 1; i < titleChunks.count() && searchWords.length() < 64; ++i)
                searchWords += QLatin1Char(' ') + titleChunks[i];
        }
    }
    const QStringList authors = entry.authorsLastName();
    for (int i = 0; i < authors.count() && searchWords.length() < 96; ++i)
        searchWords += QLatin1Char(' ') + authors[i];

    searchWords.remove(QLatin1Char('{')).remove(QLatin1Char('}'));

    QStringList urlFields {Entry::ftDOI, Entry::ftUrl, QStringLiteral("ee")};
    for (int i = 2; i < 256; ++i)
        urlFields << QString(QStringLiteral("%1%2")).arg(Entry::ftDOI).arg(i) << QString(QStringLiteral("%1%2")).arg(Entry::ftUrl).arg(i);
    for (const QString &field : const_cast<const QStringList &>(urlFields)) {
        if (entry.contains(field)) {
            const QString fieldText = PlainTextValue::text(entry.value(field));
            QRegularExpressionMatchIterator doiRegExpMatchIt = KBibTeX::doiRegExp.globalMatch(fieldText);
            while (doiRegExpMatchIt.hasNext()) {
                const QRegularExpressionMatch doiRegExpMatch = doiRegExpMatchIt.next();
                const QString doiNumber{doiRegExpMatch.captured(QStringLiteral("doi"))};
                d->queueUrl(QUrl(KBibTeX::doiUrlPrefix + doiNumber), fieldText, Entry::ftDOI, maxDepth);
#ifdef HAVE_SCIHUB
                static const QVector<QString> sciHubUrlPrefixes {{QStringLiteral("https://sci-hub.se/")}, {QStringLiteral("https://sci-hub.st/")}, {QStringLiteral("https://sci-hub.ru/")}};
                d->queueUrl(QUrl::fromUserInput(sciHubUrlPrefixes[QRandomGenerator::global()->bounded(sciHubUrlPrefixes.length())] + doiNumber), fieldText, Entry::ftDOI, maxDepth);
#endif // HAVE_SCIHUB
            }

            QRegularExpressionMatchIterator urlRegExpMatchIt = KBibTeX::urlRegExp.globalMatch(fieldText);
            while (urlRegExpMatchIt.hasNext()) {
                QRegularExpressionMatch urlRegExpMatch = urlRegExpMatchIt.next();
                d->queueUrl(QUrl(urlRegExpMatch.captured(0)), searchWords, Entry::ftUrl, maxDepth);
            }
        }
    }

    if (entry.contains(QStringLiteral("eprint"))) {
        /// check eprint fields as used for arXiv
        const QString eprintId = PlainTextValue::text(entry.value(QStringLiteral("eprint")));
        if (!eprintId.isEmpty()) {
            const QUrl arxivUrl = QUrl::fromUserInput(QStringLiteral("https://arxiv.org/search/advanced?terms-0-term=") + eprintId + QStringLiteral("&terms-0-field=report_num&size=50&order=-announced_date_first"));
            d->queueUrl(arxivUrl, eprintId, QStringLiteral("eprint"), maxDepth);
        }
    }

    if (!searchWords.isEmpty()) {
        /// Search in Google
        const QUrl googleUrl = QUrl::fromUserInput(QStringLiteral("https://www.google.com/search?hl=en&sa=G&q=filetype:pdf ") + searchWords);
        d->queueUrl(googleUrl, searchWords, QStringLiteral("www.google"), maxDepth);

        /// Search in Google Scholar
        const QUrl googleScholarUrl = QUrl::fromUserInput(QStringLiteral("https://scholar.google.com/scholar?hl=en&btnG=Search&as_sdt=1&q=filetype:pdf ") + searchWords);
        d->queueUrl(googleScholarUrl, searchWords, QStringLiteral("scholar.google"), maxDepth);

        /// Search in Bing
        const QUrl bingUrl = QUrl::fromUserInput(QStringLiteral("https://www.bing.com/search?setlang=en-US&q=filetype:pdf ") + searchWords);
        d->queueUrl(bingUrl, searchWords, QStringLiteral("bing"), maxDepth);

        /// Search in CiteSeerX
        const QUrl citeseerXurl = QUrl::fromUserInput(QStringLiteral("https://citeseerx.ist.psu.edu/search?submit=Search&sort=rlv&t=doc&q=") + searchWords);
        d->queueUrl(citeseerXurl, searchWords, QStringLiteral("citeseerx"), maxDepth);

        /// Search in StartPage
        const QUrl startPageUrl = QUrl::fromUserInput(QStringLiteral("https://www.startpage.com/do/asearch?cat=web&cmd=process_search&language=english&engine0=v1all&abp=-1&t=white&nj=1&prf=23ad6aab054a88d3da5c443280cee596&suggestOn=0&query=filetype:pdf ") + searchWords);
        d->queueUrl(startPageUrl, searchWords, QStringLiteral("startpage"), maxDepth);
    }

    if (d->aliveCounter == 0) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Directly at start, no URLs are queue for a search -> this should never happen";
        Q_EMIT finished();
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
    static const char *htmlHead1 = "<html", *htmlHead2 = "<HTML", *htmlHead3 = "<!doctype html>" /** ACM Digital Library */;
    static const char *pdfHead = "%PDF-";

    --d->aliveCounter;
    Q_EMIT progress(d->knownUrls.count(), d->aliveCounter, d->result.count());

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
        redirUrl = redirUrl.isValid() ? reply->url().resolved(redirUrl) : QUrl();
        qCDebug(LOG_KBIBTEX_NETWORKING) << "finished Downloading " << reply->url().toDisplayString() << "   depth=" << depth  << "  d->aliveCounter=" << d->aliveCounter << "  data.size=" << data.size() << "  redirUrl=" << redirUrl.toDisplayString() << "   origin=" << origin;

        if (redirUrl.isValid()) {
            redirUrl = d->ieeeDocumentUrlToDownloadUrl(redirUrl);
            d->queueUrl(redirUrl, term, origin, depth - 1);
        } else if (data.contains(htmlHead1) || data.contains(htmlHead2) || data.contains(htmlHead3)) {
            /// returned data is a HTML file, i.e. contains "<html"

            /// check for limited depth before continuing
            if (depthOk && depth > 0) {
                /// Get webpage as plain text
                /// Assume UTF-8 data
                const QString text = QString::fromUtf8(data.constData());

                /// regular expression to check if this is a Google Scholar result page
                static const QRegularExpression googleScholarTitleRegExp(QStringLiteral("<title>[^>]* - Google Scholar</title>"));
                /// regular expression to check if this is a SpringerLink page
                static const QRegularExpression springerLinkTitleRegExp(QStringLiteral("<title>[^>]* - Springer - [^>]*</title>"));
                /// regular expression to check if this is a CiteSeerX page
                static const QRegularExpression citeseerxTitleRegExp(QStringLiteral("<title>CiteSeerX &mdash; [^>]*</title>"));
                /// regular expression to check if this is a ACM Digital Library page
                static const QString acmDigitalLibraryString(QStringLiteral("The ACM Digital Library is published by the Association for Computing Machinery"));

                if (googleScholarTitleRegExp.match(text).hasMatch())
                    d->processGoogleResult(reply, text);
                else if (springerLinkTitleRegExp.match(text).hasMatch())
                    d->processSpringerLink(reply, text);
                else if (citeseerxTitleRegExp.match(text).hasMatch())
                    d->processCiteSeerX(reply, text);
                else if (text.contains(acmDigitalLibraryString))
                    d->processACMDigitalLibrary(reply, text);
                else {
                    /// regular expression to extract title
                    static const QRegularExpression titleRegExp(QStringLiteral("<title>(.*?)</title>"));
                    const QRegularExpressionMatch match = titleRegExp.match(text);
                    if (match.hasMatch())
                        qCDebug(LOG_KBIBTEX_NETWORKING) << "Using general HTML processor for page" << match.captured(1) << " URL=" << reply->url().toDisplayString();
                    else
                        qCDebug(LOG_KBIBTEX_NETWORKING) << "Using general HTML processor for URL=" << reply->url().toDisplayString();
                    d->processGeneralHTML(reply, text);
                }
            }
        } else if (data.contains(pdfHead)) {
            /// looks like a PDF file -> grab it
            const bool gotPDFfile = d->processPDF(reply, data);
            if (gotPDFfile)
                Q_EMIT progress(d->knownUrls.count(), d->aliveCounter, d->result.count());
        } else {
            /// Assume UTF-8 data
            const QString text = QString::fromUtf8(data.constData());
            qCWarning(LOG_KBIBTEX_NETWORKING) << "don't know how to handle " << text.left(256);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "error from reply: " << reply->errorString() << "(" << reply->url().toDisplayString() << ")" << "  term=" << term << "  origin=" << origin << "  depth=" << depth;

    if (d->aliveCounter == 0) {
        /// no more running downloads left
        Q_EMIT finished();
    }
}
