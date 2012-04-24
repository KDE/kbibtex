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

#include <internalnetworkaccessmanager.h>
#include "onlinesearchieeexplore.h"
#include "xsltransform.h"
#include "fileimporterbibtex.h"

class OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate
{
private:
    OnlineSearchIEEEXplore *p;
    QMap<QString, QString> originalCookiesSettings;
    bool originalCookiesEnabled;

public:
    int numResults;
    QStringList queryFragments;
    QStringList arnumberList;
    QString startPageUrl, searchRequestUrl, fullAbstractUrl, citationUrl, citationPostData;
    FileImporterBibTeX fileImporter;
    int numSteps, curStep;

    OnlineSearchIEEEXplorePrivate(OnlineSearchIEEEXplore *parent)
            : p(parent) {
        startPageUrl = QLatin1String("http://ieeexplore.ieee.org/");
        searchRequestUrl = QLatin1String("http://ieeexplore.ieee.org/search/searchresult.jsp?newsearch=true&x=0&y=0&queryText=");
        fullAbstractUrl = QLatin1String("http://ieeexplore.ieee.org/search/srchabstract.jsp?tp=&arnumber=");
        citationUrl = QLatin1String("http://ieeexplore.ieee.org/xpl/downloadCitations?fromPageName=searchabstract&citations-format=citation-abstract&download-format=download-bibtex&x=61&y=24&recordIds=");
    }

    void sanitize(QSharedPointer<Entry> entry, const QString &arnumber) {
        entry->setId(QLatin1String("ieee") + arnumber);

        Value v;
        v.append(QSharedPointer<PlainText>(new PlainText(arnumber)));
        entry->insert(QLatin1String("arnumber"), v);
    }

    void sanitizeBibTeXCode(QString &code) {
        const QRegExp htmlEncodedCharDec("&?#(\\d+);");
        const QRegExp htmlEncodedCharHex("&?#x([0-9a-f]+);", Qt::CaseInsensitive);

        while (htmlEncodedCharDec.indexIn(code) >= 0) {
            bool ok = false;
            QChar c(htmlEncodedCharDec.cap(1).toInt(&ok));
            if (ok) {
                code = code.replace(htmlEncodedCharDec.cap(0), c);
            }
        }

        while (htmlEncodedCharHex.indexIn(code) >= 0) {
            bool ok = false;
            QChar c(htmlEncodedCharHex.cap(1).toInt(&ok, 16));
            if (ok) {
                code = code.replace(htmlEncodedCharHex.cap(0), c);
            }
        }
    }
};

OnlineSearchIEEEXplore::OnlineSearchIEEEXplore(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate(this))
{
    // nothing
}

OnlineSearchIEEEXplore::~OnlineSearchIEEEXplore()
{
    delete d;
}

void OnlineSearchIEEEXplore::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

void OnlineSearchIEEEXplore::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->numResults = numResults;
    d->curStep = 0;
    d->numSteps = numResults * 2 + 2;

    d->queryFragments.clear();
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it)
        foreach(QString queryFragment, splitRespectingQuotationMarks(it.value())) {
        d->queryFragments.append(encodeURL(queryFragment));
    }

    QNetworkRequest request(d->startPageUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));

    emit progress(0, d->numSteps);
}

void OnlineSearchIEEEXplore::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
            /// redirection to another url
            QUrl redirUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            setNetworkReplyTimeout(reply);
            connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
        } else {
            QString url = d->searchRequestUrl + '"' + d->queryFragments.join("\"+AND+\"") + '"';
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSearchResults()));
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchIEEEXplore::doneFetchingSearchResults()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlText(reply->readAll());
        static const QRegExp arnumberRegExp("arnumber=(\\d+)[^0-9]");
        d->arnumberList.clear();
        int p = -1;
        while ((p = arnumberRegExp.indexIn(htmlText, p + 1)) >= 0) {
            QString arnumber = arnumberRegExp.cap(1);
            if (!d->arnumberList.contains(arnumber))
                d->arnumberList << arnumber;
            if (d->arnumberList.count() >= d->numResults)
                break;
        }

        if (d->arnumberList.isEmpty()) {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
            return;
        } else {
            QString url = d->fullAbstractUrl + d->arnumberList.first();
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstract()));
            d->arnumberList.removeFirst();
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchIEEEXplore::doneFetchingAbstract()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QString arnumber = reply->url().queryItemValue(QLatin1String("arnumber"));
        if (!arnumber.isEmpty()) {
            QString url = d->citationUrl + arnumber;
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibliography()));
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchIEEEXplore::doneFetchingBibliography()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QString plainText = QString(reply->readAll()).replace("<br>", "");
        d->sanitizeBibTeXCode(plainText);

        bool hasEntry = false;
        File *bibtexFile = d->fileImporter.fromString(plainText);
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (!entry.isNull()) {
                    QString arnumber = reply->url().queryItemValue(QLatin1String("recordIds"));
                    d->sanitize(entry, arnumber);

                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                    entry->insert("x-fetchedfrom", v);

                    emit foundEntry(entry);
                    hasEntry = true;
                }
            }
            delete bibtexFile;
        }

        if (!hasEntry) {
            kWarning() << "Searching" << label() << "(url:" << reply->url().toString() << ") resulted in invalid BibTeX data:" << QString(reply->readAll());
            emit stoppedSearch(resultUnspecifiedError);
            return;
        }

        if (!d->arnumberList.isEmpty()) {
            QString url = d->fullAbstractUrl + d->arnumberList.first();
            d->arnumberList.removeFirst();
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstract()));
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

QString OnlineSearchIEEEXplore::label() const
{
    return i18n("IEEEXplore");
}

QString OnlineSearchIEEEXplore::favIconUrl() const
{
    return QLatin1String("http://ieeexplore.ieee.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIEEEXplore::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchIEEEXplore::homepage() const
{
    return KUrl("http://ieeexplore.ieee.org/");
}

void OnlineSearchIEEEXplore::cancel()
{
    OnlineSearchAbstract::cancel();
}
