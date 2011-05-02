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

#include <QWebPage>
#include <QWebFrame>
#include <QWebElement>
#include <QFile>
#include <QNetworkRequest>

#include <KDebug>
#include <KLocale>
#include <kio/job.h>

#include "fileimporterbibtex.h"
#include "websearchsciencedirect.h"


class WebSearchScienceDirect::WebSearchScienceDirectPrivate
{
private:
    WebSearchScienceDirect *p;

public:
    QWebPage *page;
    QString joinedQueryString;
    int currentSearchPosition;
    int numExpectedResults;
    const QString scienceDirectBaseUrl;
    QStringList bibTeXUrls;

    WebSearchScienceDirectPrivate(WebSearchScienceDirect *parent)
            : p(parent), page(new QWebPage(parent)),
            scienceDirectBaseUrl(QLatin1String("http://www.sciencedirect.com/")) {
        page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
    }

    ~WebSearchScienceDirectPrivate() {
        delete page;
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
        : WebSearchAbstract(parent), d(new WebSearchScienceDirect::WebSearchScienceDirectPrivate(this))
{
    // nothing
}

void WebSearchScienceDirect::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

void WebSearchScienceDirect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->bibTeXUrls.clear();
    d->currentSearchPosition = 0;

    d->joinedQueryString.clear();
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->joinedQueryString.append(it.value() + ' ');
    }
    d->joinedQueryString = d->joinedQueryString.trimmed();
    kDebug() << "trimmed=|" << d->joinedQueryString << "|";
    d->numExpectedResults = numResults;

    d->page->mainFrame()->load(d->scienceDirectBaseUrl);
    connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingStartPage(bool)));
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
    d->page->triggerAction(QWebPage::Stop);
}

void WebSearchScienceDirect::doneFetchingStartPage(bool ok)
{
    disconnect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingStartPage(bool)));

    if (handleErrors(ok)) {
        QWebElement form = d->page->mainFrame()->findFirstElement(QLatin1String("form[name=\"qkSrch\"]"));
        if (!form.isNull()) {
            QWebElementCollection allInputs = form.findAll(QLatin1String("input"));
            QString queryString;
            foreach(QWebElement input, allInputs) {
                if (!queryString.isEmpty()) queryString += '&';
                if (input.attribute(QLatin1String("name")) == QLatin1String("qs_all"))
                    queryString += QLatin1String("qs_all=") + d->joinedQueryString;
                else
                    queryString += input.attribute(QLatin1String("name")) + '=' + input.attribute(QLatin1String("value"));
            }

            KUrl url(d->scienceDirectBaseUrl + form.attribute(QLatin1String("action")) + '?' + queryString);
            d->page->mainFrame()->load(url);
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingResultPage(bool)));
        } else {
            kWarning() << "Form is null";
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kWarning() << "handleError returned false";
}

void WebSearchScienceDirect::doneFetchingResultPage(bool ok)
{
    disconnect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingResultPage(bool)));

    if (handleErrors(ok)) {
        QWebElementCollection links = d->page->mainFrame()->findAllElements(QLatin1String("div[id=\"searchResults\"] > div[id=\"bodyMainResults\"] > table > tbody > tr > td > a"));
        foreach(QWebElement link, links) {
            QString linkText = link.attribute(QLatin1String("href"));
            d->bibTeXUrls << linkText;
            if (d->bibTeXUrls.count() >= d->numExpectedResults)
                break;
        }

        d->currentSearchPosition += 25;
        QWebElement form;
        if (d->currentSearchPosition < d->numExpectedResults && !(form = d->page->mainFrame()->findFirstElement(QLatin1String("form[name=\"Tag\"]"))).isNull()) {
            QWebElementCollection allInputs = form.findAll(QLatin1String("input[name!=\"prev\"]"));
            QString queryString;
            foreach(QWebElement input, allInputs) {
                if (!queryString.isEmpty()) queryString += '&';
                queryString += input.attribute(QLatin1String("name")) + '=' + input.attribute(QLatin1String("value"));
            }

            KUrl url(d->scienceDirectBaseUrl + form.attribute(QLatin1String("action")) + '?' + queryString);
            d->page->mainFrame()->load(url);
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingResultPage(bool)));
        } else if (!d->bibTeXUrls.isEmpty()) {
            d->page->mainFrame()->load(d->bibTeXUrls.first());
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingAbstractPage(bool)));
            d->bibTeXUrls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    } else
        kWarning() << "handleError returned false";
}

void WebSearchScienceDirect::doneFetchingAbstractPage(bool ok)
{
    disconnect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingAbstractPage(bool)));

    if (handleErrors(ok)) {
        QWebElement exportCitationLink = d->page->mainFrame()->findFirstElement(QLatin1String("a[title=\"Export Citation\"]"));
        if (!exportCitationLink.isNull()) {
            KUrl url(d->scienceDirectBaseUrl + exportCitationLink.attribute(QLatin1String("href")));
            d->page->mainFrame()->load(url);
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingExportCitationPage(bool)));
        } else if (!d->bibTeXUrls.isEmpty()) {
            kWarning() << "did not find \"Export Citation\"";
            d->page->mainFrame()->load(d->bibTeXUrls.first());
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingAbstractPage(bool)));
            d->bibTeXUrls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    } else
        kWarning() << "handleError returned false";
}

void WebSearchScienceDirect::doneFetchingExportCitationPage(bool ok)
{
    disconnect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingExportCitationPage(bool)));

    QWebElement form;
    if (handleErrors(ok)) {
        QWebElement form = d->page->mainFrame()->findFirstElement(QLatin1String("form[name=\"exportCite\"]"));
        if (!form.isNull()) {
            QWebElementCollection allInputs = form.findAll(QLatin1String("input"));
            QString queryString = QLatin1String("format=cite-abs&citation-type=BIBTEX");
            foreach(QWebElement input, allInputs) {
                if (input.attribute(QLatin1String("type")) == QLatin1String("radio")) {
                    /// ignore radio buttons, will be set automatically
                } else if (input.attribute(QLatin1String("name")) == QLatin1String("RETURN_URL")) {
                    /// ignore strange input values
                }  else if (input.attribute(QLatin1String("name")) == QLatin1String("JAVASCRIPT_ON")) {
                    /// disable JavaScript support
                    queryString += QLatin1String("&JAVASCRIPT_ON=");
                } else
                    queryString += '&' + input.attribute(QLatin1String("name")) + '=' + input.attribute(QLatin1String("value"));
            }

            KUrl url(d->scienceDirectBaseUrl + form.attribute(QLatin1String("action")));
            KIO::TransferJob *job = KIO::storedHttpPost(queryString.toUtf8(), url);
            job->addMetaData(QLatin1String("content-type"), QLatin1String("application/x-www-form-urlencoded"));
            job->addMetaData(QLatin1String("referrer"), d->page->mainFrame()->baseUrl().toString());
            connect(job, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob *)));
        } else if (!d->bibTeXUrls.isEmpty()) {
            kWarning() << "did not find Form";
            d->page->mainFrame()->load(d->bibTeXUrls.first());
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingAbstractPage(bool)));
            d->bibTeXUrls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    }  else
        kWarning() << "handleError returned false";
}

void WebSearchScienceDirect::doneFetchingBibTeX(KJob * kJob)
{
    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *job = static_cast<KIO::StoredTransferJob*>(kJob);
        QTextStream ts(job->data());
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

        if (!d->bibTeXUrls.isEmpty()) {
            d->page->mainFrame()->load(d->bibTeXUrls.first());
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingAbstractPage(bool)));
            d->bibTeXUrls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    } else
        kWarning() << "handleError returned false";
}
