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
#include <QNetworkRequest>
#include <QRegExp>
#include <QTextStream>

#include <poppler-qt4.h>

#include <KApplication>
#include <KDebug>
#include <KTemporaryFile>

#include <kbibtexnamespace.h>
#include <internalnetworkaccessmanager.h>
#include <value.h>
#include "findpdf.h"

int FindPDF::fileCounter = 0;

int maxDepth = 5;
const char *depthProperty = "depth";
const char *termProperty = "term";
const char *originProperty = "origin";

FindPDF::FindPDF(QObject *parent)
        : QObject(parent), aliveCounter(0)
{
    // TODO
}

bool FindPDF::search(const Entry &entry)
{
    if (aliveCounter > 0) return false;

    m_knownUrls.clear();
    m_result.clear();
    m_currentEntry = entry;

    emit progress(0, 0);

    /// generate a string which contains the title's beginning
    QString titleBeginning;
    if (entry.contains(Entry::ftTitle)) {
        QStringList titleChunks = PlainTextValue::text(entry.value(Entry::ftTitle)).split(QChar(' '), QString::SkipEmptyParts);
        if (!titleChunks.isEmpty()) {
            titleBeginning = titleChunks[0];
            for (int i = 1; i < titleChunks.count() && titleBeginning.length() < 32;++i)
                titleBeginning += QChar(' ') + titleChunks[i];
        }
    }

    QStringList urlFields = QStringList() << Entry::ftDOI << Entry::ftUrl << QLatin1String("ee");
    for (int i = 2; i < 256; ++i)
        urlFields << QString(QLatin1String("%1%2")).arg(Entry::ftDOI).arg(i) << QString(QLatin1String("%1%2")).arg(Entry::ftUrl).arg(i);
    foreach(const QString &field, urlFields) {
        if (entry.contains(field)) {
            const QString fieldText = PlainTextValue::text(entry.value(field));
            int p = -1;
            while ((p = KBibTeX::doiRegExp.indexIn(fieldText, p + 1)) >= 0)
                queueUrl(QUrl(QLatin1String("http://dx.doi.org/") + KBibTeX::doiRegExp.cap(0)), fieldText, field, maxDepth);

            p = -1;
            while ((p = KBibTeX::urlRegExp.indexIn(fieldText, p + 1)) >= 0)
                queueUrl(QUrl(KBibTeX::urlRegExp.cap(0)), titleBeginning, field, maxDepth);
        }
    }

    if (entry.contains(QLatin1String("eprint"))) {
        /// check eprint fields as used for arXiv
        const QString fieldText = PlainTextValue::text(entry.value(QLatin1String("eprint")));
        if (!fieldText.isEmpty())
            queueUrl(QUrl(QLatin1String("http://arxiv.org/pdf/") + fieldText), fieldText, QLatin1String("eprint"), maxDepth);
    }

    if (!titleBeginning.isEmpty()) {
        QString fieldText = PlainTextValue::text(entry.value(Entry::ftTitle));

        /// use title to search in Google
        if (!fieldText.isEmpty()) {
            KUrl url(QLatin1String("http://scholar.google.com/scholar?hl=en&btnG=Search&as_sdt=1"));
            url.addQueryItem(QLatin1String("q"), QString(fieldText + QLatin1String(" filetype:pdf")).replace(QChar(' '), QChar('+')));
            queueUrl(url, titleBeginning, QLatin1String("scholar.google"), maxDepth);
        }

        /// use title to search in Bing
        if (!fieldText.isEmpty()) {
            KUrl url(QLatin1String("http://www.bing.com/search?setmkt=en-IE&setlang=match"));
            url.addQueryItem(QLatin1String("q"), QString(fieldText + QLatin1String(" filetype:pdf")).replace(QChar(' '), QChar('+')));
            queueUrl(url, titleBeginning, QLatin1String("bing"), maxDepth);
        }
    }

    if (aliveCounter == 0)
        emit finished();

    return true;
}

bool FindPDF::queueUrl(const QUrl &url, const QString &term, const QString &origin, int depth)
{
    if (!m_knownUrls.contains(url) && depth > 0) {
        kDebug() << "Starting download for" << url.toString() << "(" << origin << ")";
        m_knownUrls.insert(url);
        QNetworkRequest request = QNetworkRequest(url);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        reply->setProperty(depthProperty, QVariant::fromValue<int>(depth));
        reply->setProperty(termProperty, term);
        reply->setProperty(originProperty, origin);
        connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
        ++aliveCounter;
        return true;
    } else
        return false;
}

QList<FindPDF::ResultItem> FindPDF::results()
{
    return m_result;
}

void FindPDF::downloadFinished()
{
    static const char *htmlHead1 = "<html", *htmlHead2 = "<HTML";
    static const char *pdfHead = "%PDF-";

    emit progress(m_knownUrls.count(), m_result.count());

    --aliveCounter;
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    const QString term = reply->property(termProperty).toString();
    const QString origin = reply->property(originProperty).toString();
    bool depthOk = false;
    int depth = reply->property(depthProperty).toInt(&depthOk);
    if (!depthOk) depth = 0;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();

        QUrl redirUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        kDebug() << "finished Downloading " << reply->url().toString() << "   depth=" << depth  << "  aliveCounter=" << aliveCounter << "  data.size=" << data.size() << "  redirUrl=" << redirUrl.toString() << "   origin=" << origin;

        if (data.contains(htmlHead1) || data.contains(htmlHead2)) {
            /// returned data is a HTML file, i.e. contains "<html"

            QFile htmlFile(QString("/tmp/file%1.html").arg(++fileCounter));
            if (htmlFile.open(QFile::WriteOnly)) {
                htmlFile.write(reply->url().toString().toAscii());
                htmlFile.write("\n\n\n");
                htmlFile.write(data);
                htmlFile.close();
            }

            /// check for limited depth before continuing
            if (depthOk && depth > 0) {
                /// get webpage as plain text
                QTextStream ts(data);
                const QString text = ts.readAll();

                /// regular expression to check if this is a Google Scholar result page
                static const QRegExp googleScholarTitleRegExp(QLatin1String("<title>[^>]* - Google Scholar</title>"));
                /// regular expression to check if this is a SpringerLink page
                static const QRegExp springerLinkTitleRegExp(QLatin1String("<title>[^>]*SpringerLink - [^>]*</title>"));

                if (text.indexOf(googleScholarTitleRegExp) > 0)
                    processGoogleResult(reply, text);
                else if (text.indexOf(springerLinkTitleRegExp) > 0)
                    processSpringerLink(reply, text);
                else
                    processGeneralHTML(reply, text);
            }
        } else if (data.contains(pdfHead)) {
            /// looks like a PDF file -> grab it
            processPDF(reply, data);
        } else if (redirUrl.isValid())
            queueUrl(redirUrl, term, origin, depth - 1);
        else {
            QTextStream ts(data);
            const QString text = ts.readAll();
            kDebug() << "don't know how to handle " << text.left(256);
        }
    } else
        kDebug() << "error from reply: " << reply->errorString();

    if (aliveCounter == 0) {
        /// no more running downloads left
        emit finished();
    }
}

void FindPDF::processGeneralHTML(QNetworkReply *reply, const QString &text)
{
    /// fetch some properties from Reply object
    const QString term = reply->property(termProperty).toString();
    const QString origin = reply->property(originProperty).toString();
    bool ok = false;
    int depth = reply->property(depthProperty).toInt(&ok);
    if (!ok) depth = 0;

    /// regular expressions to guess links to follow
    const QRegExp anchorRegExp[5] = {
        QRegExp(QString(QLatin1String("<a[^>]*href=\"([^\"]*%1[^\"]*[.]pdf)\"")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
        QRegExp(QString(QLatin1String("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*%1[^<]*[.]pdf")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
        QRegExp(QString(QLatin1String("<a[^>]*href=\"([^\"]*%1[^\"]*)\"")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
        QRegExp(QString(QLatin1String("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*%1[^<]*\\b")).arg(QRegExp::escape(term)), Qt::CaseInsensitive),
        QRegExp(QLatin1String("<a[^>]*href=\"([^\"]+)\""), Qt::CaseInsensitive) /// any link
    };

    bool gotLink = false;
    for (int i = 0; !gotLink && i < 4; ++i) {
        if (anchorRegExp[i].indexIn(text) >= 0) {
            QUrl url = QUrl::fromEncoded(anchorRegExp[i].cap(1).toAscii());
            queueUrl(reply->url().resolved(url), term, origin, depth - 1);
            gotLink = true;
        }
    }

    if (!gotLink && text.count(anchorRegExp[4]) == 1) {
        /// this is only the last resort:
        /// to follow the first link found in the HTML document
        if (anchorRegExp[4].indexIn(text) >= 0) {
            QUrl url = QUrl::fromEncoded(anchorRegExp[4].cap(1).toAscii());
            queueUrl(reply->url().resolved(url), term, origin, depth - 1);
        }
    }
}

void FindPDF::processGoogleResult(QNetworkReply *reply, const QString &text)
{
    static const QString h3Tag(QLatin1String("<h3"));
    static const QString aTag(QLatin1String("<a"));
    static const QString hrefAttrib(QLatin1String("href=\""));

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
            int p2 = text.indexOf(QChar('"'), p1 + 1);
            QUrl url(text.mid(p1, p2 - p1));
            kDebug() << "Google URL" << i << " : " << url.toString();
            queueUrl(reply->url().resolved(url), term, QLatin1String("scholar.google"), depth - 1);
        }
    }
}

void FindPDF::processSpringerLink(QNetworkReply *reply, const QString &text)
{
    static const QRegExp fulltextPDFlink(QLatin1String("href=\"([^\"]+/fulltext.pdf)\""));

    if (fulltextPDFlink.indexIn(text) > 0) {
        bool ok = false;
        int depth = reply->property(depthProperty).toInt(&ok);
        if (!ok) depth = 0;

        QUrl url(fulltextPDFlink.cap(1));
        queueUrl(reply->url().resolved(url), QString::null, QLatin1String("springerlink"), depth - 1);
    }
}

void FindPDF::processCiteSeerX(QNetworkReply *reply, const QString &text)
{
    static const QRegExp downloadPDFlink(QLatin1String("href=\"(/viewdoc/download[^\"]+type=pdf)\""));

    if (downloadPDFlink.indexIn(text) > 0) {
        bool ok = false;
        int depth = reply->property(depthProperty).toInt(&ok);
        if (!ok) depth = 0;

        QUrl url(QUrl::fromEncoded(downloadPDFlink.cap(1).toAscii()));
        queueUrl(reply->url().resolved(url), QString::null, QLatin1String("citeseerx"), depth - 1);
    }
}

void FindPDF::processPDF(QNetworkReply *reply, const QByteArray &data)
{
    const QString origin = reply->property(originProperty).toString();

    Poppler::Document *doc = Poppler::Document::loadFromData(data);

    kDebug() << "PDF title:" << doc->info("Title") << endl;

    QUrl url = reply->url();

    /// search for duplicate URLs
    bool containsUrl = false;
    foreach(const ResultItem &ri, m_result) {
        containsUrl |= ri.url == url;
        if (containsUrl) break;
    }

    if (!containsUrl) {
        ResultItem result;
        result.tempFilename = new KTemporaryFile();
        result.tempFilename->setAutoRemove(true);
        result.tempFilename->setSuffix(QLatin1String(".pdf"));
        if (result.tempFilename->open()) {
            result.tempFilename->write(data);
            result.tempFilename->close();
        } else {
            delete result.tempFilename;
            result.tempFilename = NULL;
        }
        result.url = url;
        result.textPreview = doc->info("Title").simplified();
        const int maxTextLen = 1024;
        for (int i = 0; i < doc->numPages() && result.textPreview.length() < maxTextLen; ++i)
            result.textPreview += QChar(' ') + doc->page(i)->text(QRect()).simplified().left(maxTextLen);
        result.downloadMode = NoDownload;
        result.relevance = origin == QLatin1String("doi") ? 1.0 : (origin == QLatin1String("eprint") ? 0.75 : 0.5);
        m_result << result;

        emit progress(m_knownUrls.count(), m_result.count());
    }
}

