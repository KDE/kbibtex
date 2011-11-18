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

#include <QBuffer>
#include <QLayout>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <KLocale>
#include <KDebug>
#include <kio/job.h>
#include <KMessageBox>

#include <internalnetworkaccessmanager.h>
#include <fileimporterbibtex.h>
#include <file.h>
#include <entry.h>
#include "onlinesearchacmportal.h"

class OnlineSearchAcmPortal::OnlineSearchAcmPortalPrivate
{
private:
    OnlineSearchAcmPortal *p;

public:
    QString joinedQueryString;
    int numExpectedResults, numFoundResults;
    const QString acmPortalBaseUrl;
    int currentSearchPosition;
    QStringList bibTeXUrls;

    int curStep, numSteps;

    OnlineSearchAcmPortalPrivate(OnlineSearchAcmPortal *parent)
            : p(parent), numExpectedResults(0), numFoundResults(0),
            acmPortalBaseUrl(QLatin1String("http://dl.acm.org/")) {
        // nothing
    }

    void sanitizeBibTeXCode(QString &code) {
        const QRegExp htmlEncodedChar("&#(\\d+);");
        while (htmlEncodedChar.indexIn(code) >= 0) {
            bool ok = false;
            QChar c(htmlEncodedChar.cap(1).toInt(&ok));
            if (ok) {
                code = code.replace(htmlEncodedChar.cap(0), c);
            }
        }
    }

    void sanitizeEntry(Entry *entry) {
        if (entry->contains(QLatin1String("issue"))) {
            /// ACM's Digital Library uses "issue" instead of "number" -> fix that
            Value v = entry->value(QLatin1String("issue"));
            entry->remove(QLatin1String("issue"));
            entry->insert(Entry::ftNumber, v);
        }
    }
};

OnlineSearchAcmPortal::OnlineSearchAcmPortal(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchAcmPortalPrivate(this))
{
    // nothing
}

OnlineSearchAcmPortal::~OnlineSearchAcmPortal()
{
    delete d;
}

void OnlineSearchAcmPortal::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->joinedQueryString.clear();
    d->currentSearchPosition = 1;
    d->bibTeXUrls.clear();
    d->numFoundResults = 0;
    d->curStep = 0;
    d->numSteps = numResults + 2;

    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->joinedQueryString.append(it.value() + ' ');
    }
    d->numExpectedResults = numResults;

    QNetworkRequest request(d->acmPortalBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
    emit progress(0, d->numSteps);
}

void OnlineSearchAcmPortal::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

QString OnlineSearchAcmPortal::label() const
{
    return i18n("ACM Digital Library");
}

QString OnlineSearchAcmPortal::favIconUrl() const
{
    return QLatin1String("http://portal.acm.org/favicon.ico");
}

OnlineSearchQueryFormAbstract* OnlineSearchAcmPortal::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchAcmPortal::homepage() const
{
    return KUrl("http://portal.acm.org/");
}

void OnlineSearchAcmPortal::cancel()
{
    OnlineSearchAbstract::cancel();
    // FIXME d->page->triggerAction(QWebPage::Stop);
}

void OnlineSearchAcmPortal::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString htmlSource = reply->readAll();
        int p1 = -1, p2 = -1, p3 = -1;
        if ((p1 = htmlSource.indexOf("<form name=\"qiksearch\"")) >= 0
                && (p2 = htmlSource.indexOf("action=", p1)) >= 0
                && (p3 = htmlSource.indexOf("\"", p2 + 8)) >= 0) {
            QString action = decodeURL(htmlSource.mid(p2 + 8, p3 - p2 - 8));
            KUrl url(d->acmPortalBaseUrl + action);
            QString body = QString("Go=&query=%1").arg(d->joinedQueryString).simplified();

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->post(request, body.toUtf8());
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSearchPage()));
        } else {
            kWarning() << "Search using" << label() << "failed.";
            KMessageBox::error(m_parent, i18n("Searching \"%1\" failed: Could not extract form from ACM's start page.", label()));
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchAcmPortal::doneFetchingSearchPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString htmlSource = reply->readAll();
        static QRegExp paramRegExp("<a [^>]+\\?id=([0-9]+)\\.([0-9]+).*CFID=([0-9]+).*CFTOKEN=([0-9]+)", Qt::CaseInsensitive);
        int p1 = -1;
        while ((p1 = htmlSource.indexOf(paramRegExp, p1 + 1)) >= 0) {
            d->bibTeXUrls << d->acmPortalBaseUrl + QString("/downformats.cfm?id=%1&parent_id=%2&expformat=bibtex&CFID=%3&CFTOKEN=%4").arg(paramRegExp.cap(2)).arg(paramRegExp.cap(1)).arg(paramRegExp.cap(3)).arg(paramRegExp.cap(4));
        }

        if (d->currentSearchPosition + 20 < d->numExpectedResults) {
            d->currentSearchPosition += 20;
            KUrl url(reply->url());
            QMap<QString, QString> queryItems = url.queryItems();
            queryItems["start"] = QString::number(d->currentSearchPosition);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSearchPage()));
        } else if (!d->bibTeXUrls.isEmpty()) {
            QNetworkRequest request(d->bibTeXUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->bibTeXUrls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    }  else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchAcmPortal::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString bibTeXcode = reply->readAll();

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);
        d->sanitizeBibTeXCode(bibTeXcode);

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

        if (!d->bibTeXUrls.isEmpty() && d->numFoundResults < d->numExpectedResults) {
            QNetworkRequest request(d->bibTeXUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->bibTeXUrls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
