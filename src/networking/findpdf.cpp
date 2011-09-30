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
#include <httpequivcookiejar.h>
#include <value.h>
#include "findpdf.h"

int FindPDF::maxDepth = 10;
const char *FindPDF::depthProperty = "depth";
const char *FindPDF::originProperty = "origin";

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

    const QStringList urlFields = QStringList() << QLatin1String("doi") << QLatin1String("url") << QLatin1String("ee");
    foreach(const QString &field, urlFields) {
        if (entry.contains(field)) {
            const QString doi = PlainTextValue::text(entry.value(field));
            if (KBibTeX::urlRegExp.indexIn(doi) >= 0) {
                QUrl url(KBibTeX::urlRegExp.cap(0));
                if (!m_knownUrls.contains(url)) {
                    m_knownUrls.insert(url);
                    QNetworkReply *reply = HTTPEquivCookieJar::networkAccessManager()->get(QNetworkRequest(url));
                    reply->setProperty(depthProperty, QVariant::fromValue<int>(maxDepth));
                    reply->setProperty(originProperty, field);
                    connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
                    ++aliveCounter;
                }
            } else if (KBibTeX::doiRegExp.indexIn(doi) >= 0) {
                QUrl url(QUrl(QLatin1String("http://dx.doi.org/") + KBibTeX::doiRegExp.cap(0)));
                if (!m_knownUrls.contains(url)) {
                    m_knownUrls.insert(url);
                    QNetworkReply *reply = HTTPEquivCookieJar::networkAccessManager()->get(QNetworkRequest(url));
                    reply->setProperty(depthProperty, QVariant::fromValue<int>(maxDepth));
                    reply->setProperty(originProperty, field);
                    connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
                    ++aliveCounter;
                }
            }
        }
    }

    if (aliveCounter == 0)
        emit finished();

    return true;
}

QList<FindPDF::ResultItem> FindPDF::results()
{
    return m_result;
}

void FindPDF::downloadFinished()
{
    static const QRegExp anchorRegExp[5] = {
        QRegExp(QLatin1String("<a[^>]*href=\"([^\"]+[.]pdf)\""), Qt::CaseInsensitive),
        QRegExp(QLatin1String("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*[.]pdf"), Qt::CaseInsensitive),
        QRegExp(QLatin1String("<a[^>]*href=\"([^\"]+)\"[^>]*>[^<]*10[.]"), Qt::CaseInsensitive),
        QRegExp(QLatin1String("<a[^>]*href=\"([^\"]+10[.][^\"]+)\""), Qt::CaseInsensitive),
        QRegExp(QLatin1String("<a[^>]*href=\"([^\"]+)\""), Qt::CaseInsensitive)
    };
    const char *htmlHead1 = "<html", *htmlHead2 = "<HTML";
    const char *pdfHead = "%PDF-";

    --aliveCounter;
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    bool ok = false;
    int depth = reply->property(depthProperty).toInt(&ok);
    if (!ok) depth = 0;
    QString origin = reply->property(originProperty).toString();

    if (depth > 0) {
        if (reply->error() == QNetworkReply::NoError) {

            QByteArray data = reply->readAll();

            QUrl redirUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            kDebug() << "finished Downloading " << reply->url().toString() << "   depth=" << depth  << "  aliveCounter=" << aliveCounter << "  data.size=" << data.size() << "  redirUrl=" << redirUrl.toString() << "   origin=" << origin;
            if (data.contains(htmlHead1) || data.contains(htmlHead2)) {
                if (ok && depth > 0) {
                    QTextStream ts(data);
                    const QString text = ts.readAll();

                    kDebug() << "HTML : " << text.left(256);

                    bool gotLink = false;
                    for (int i = 0;!gotLink && i < 5; ++i) {
                        if (anchorRegExp[i].indexIn(text) >= 0) {
                            QUrl url = QUrl::fromEncoded(anchorRegExp[i].cap(1).toAscii());
                            url = reply->url().resolved(url);
                            if (!m_knownUrls.contains(url)) {
                                m_knownUrls.insert(url);
                                kDebug() << "starting download: " << url.toString();
                                QNetworkReply *reply = HTTPEquivCookieJar::networkAccessManager()->get(QNetworkRequest(url));
                                reply->setProperty(depthProperty, QVariant::fromValue<int>(depth - 1));
                                reply->setProperty(originProperty, origin);
                                connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
                                ++aliveCounter;
                                gotLink = true;
                            }
                        }
                    }
                }
            } else if (data.contains(pdfHead)) {
                Poppler::Document *doc = Poppler::Document::loadFromData(data);

                kDebug() << "PDF title:" << doc->info("Title") << endl << "Text: " << doc->page(0)->text(QRect()).left(1024);

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
                        delete    result.tempFilename;
                        result.tempFilename = NULL;
                    }
                    result.url = url;
                    result.downloadMode = NoDownload;
                    result.relevance = origin == QLatin1String("doi") ? 1.0 : 0.5;
                    m_result << result;
                }
            } else if (redirUrl.isValid()) {
                if (!m_knownUrls.contains(redirUrl)) {
                    m_knownUrls.insert(redirUrl);
                    QNetworkReply *reply = HTTPEquivCookieJar::networkAccessManager()->get(QNetworkRequest(redirUrl));
                    reply->setProperty(depthProperty, QVariant::fromValue<int>(depth - 1));
                    connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
                    ++aliveCounter;
                }
            } else {
                QString redirUrlStr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();

                QTextStream ts(data);
                const QString text = ts.readAll();
                kDebug() << "don't know how to handle "        << text.left(1024);
            }
        } else
            kDebug() << "error from reply";
    } else
        kDebug() << "max depth reached";

    if (aliveCounter == 0) {
        emit finished();
    }
}
