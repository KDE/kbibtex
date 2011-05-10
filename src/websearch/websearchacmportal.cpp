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
#include <QWebPage>
#include <QWebFrame>
#include <QWebElement>
#include <QWebElementCollection>
#include <QNetworkRequest>

#include <KLocale>
#include <KDebug>
#include <kio/job.h>
#include <KMessageBox>

#include <fileimporterbibtex.h>
#include <file.h>
#include <entry.h>
#include "websearchacmportal.h"

class WebSearchAcmPortal::WebSearchAcmPortalPrivate
{
private:
    WebSearchAcmPortal *p;

public:
    QString joinedQueryString;
    int numExpectedResults;
    QWebPage *page;
    const QString acmPortalBaseUrl;
    const QString acmPortalContinueUrl;
    int currentSearchPosition;
    QStringList bibTeXUrls;
    QString cfid, cftoken;
    QRegExp regExpId, regExpIdCFID, regExpIdCFTOKEN;

    WebSearchAcmPortalPrivate(WebSearchAcmPortal *parent)
            : p(parent), page(new QWebPage(parent)),
            acmPortalBaseUrl(QLatin1String("http://portal.acm.org/")),
            acmPortalContinueUrl(QLatin1String("http://portal.acm.org/results.cfm?query=%4&querydisp=%4&source_query=&start=%1&srt=score+dsc&short=1&source_disp=&since_month=&since_year=&before_month=&before_year=&coll=DL&dl=GUIDE&termshow=matchall&range_query=&CFID=%2&CFTOKEN=%3")),
            regExpId("(\\?|&)id=(\\d+)\\.(\\d+)(&|$)"), regExpIdCFID("(\\?|&)CFID=(\\d+)(&|$)"), regExpIdCFTOKEN("(\\?|&)CFTOKEN=(\\d+)(&|$)") {
        page->settings()->setAttribute(QWebSettings::PluginsEnabled, false);
        page->settings()->setAttribute(QWebSettings::JavaEnabled, false);
        page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
        page->settings()->setAttribute(QWebSettings::AutoLoadImages, false);
        page->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
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
};

WebSearchAcmPortal::WebSearchAcmPortal(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchAcmPortalPrivate(this))
{
    // nothing
}

void WebSearchAcmPortal::startSearch(const QMap<QString, QString> &query, int numResults)
{
    Q_UNUSED(numResults);

    m_hasBeenCanceled = false;
    d->joinedQueryString.clear();
    d->currentSearchPosition = 1;
    d->bibTeXUrls.clear();

    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->joinedQueryString.append(it.value() + ' ');
    }
    d->numExpectedResults = numResults;

    d->page->mainFrame()->load(d->acmPortalBaseUrl);
    connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingStartPage(bool)));
}

void WebSearchAcmPortal::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

QString WebSearchAcmPortal::label() const
{
    return i18n("ACM Digital Library");
}

QString WebSearchAcmPortal::favIconUrl() const
{
    return QLatin1String("http://portal.acm.org/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchAcmPortal::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchAcmPortal::homepage() const
{
    return KUrl("http://portal.acm.org/");
}

void WebSearchAcmPortal::cancel()
{
    WebSearchAbstract::cancel();
    d->page->triggerAction(QWebPage::Stop);
}

void WebSearchAcmPortal::doneFetchingStartPage(bool ok)
{
    disconnect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingStartPage(bool)));

    if (handleErrors(ok)) {
        QWebElement form = d->page->mainFrame()->findFirstElement("form[name=\"qiksearch\"]");
        if (!form.isNull()) {
            QString action = form.attribute(QLatin1String("action"));
            QString body = QString("Go=&query=%1").arg(d->joinedQueryString);

            QNetworkRequest nr(d->acmPortalBaseUrl + action);
            d->page->mainFrame()->load(nr, QNetworkAccessManager::PostOperation, body.toUtf8());
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingSearchPage(bool)));
        } else {
            kWarning() << "Search using" << label() << "failed.";
            KMessageBox::error(m_parent, i18n("Searching \"%1\" failed for unknown reason.", label()));
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << d->page->mainFrame()->url().toString();
}

void WebSearchAcmPortal::doneFetchingSearchPage(bool ok)
{
    disconnect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingSearchPage(bool)));

    if (handleErrors(ok)) {
        /// find the position where the search results begin using a CSS2 selector
        QWebElementCollection collection = d->page->mainFrame()->findAllElements("table[style=\"padding: 5px; 5px; 5px; 5px;\"]");

        foreach(QWebElement table, collection) {
            /// find individual search results using a CSS2 selector
            QWebElement titleElement = table.findFirst("td[colspan=\"3\"] > a");
            if (!titleElement.isNull()) {
                /// the title of each search hit contains a link
                QString url = titleElement.attribute(QLatin1String("HREF"));
/// extract information from this link
                if (d->regExpId.indexIn(url) >= 0 && d->regExpIdCFID.indexIn(url) >= 0 && d->regExpIdCFTOKEN.indexIn(url) >= 0) {
                    /// if information extraction was successful ...
                    QString id = d->regExpId.cap(3), parentId = d->regExpId.cap(2);
                    d->cfid = d->regExpIdCFID.cap(2);
                    d->cftoken = d->regExpIdCFTOKEN.cap(2);
                    /// ... build a new link pointing to the BibTeX file
                    QString bibTeXUrl = QString(QLatin1String("http://portal.acm.org/downformats.cfm?id=%1&parent_id=%2&expformat=bibtex&CFID=%3&CFTOKEN=%4")).arg(id).arg(parentId).arg(d->cfid).arg(d->cftoken);
                    d->bibTeXUrls << bibTeXUrl;
                }
            }
        }

        if (d->currentSearchPosition + 20 < d->numExpectedResults) {
            d->currentSearchPosition += 20;
            KUrl url(QString(d->acmPortalContinueUrl).arg(d->currentSearchPosition).arg(d->cfid).arg(d->cftoken).arg(d->joinedQueryString));
            d->page->mainFrame()->load(url);
            connect(d->page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingSearchPage(bool)));
        } else if (! d->bibTeXUrls.isEmpty()) {
            KIO::TransferJob *job = KIO::storedGet(d->bibTeXUrls.first());
            connect(job, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob*)));
            d->bibTeXUrls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << d->page->mainFrame()->url().toString();
}

void WebSearchAcmPortal::doneFetchingBibTeX(KJob *kJob)
{
    KIO::StoredTransferJob *job = static_cast<KIO::StoredTransferJob*>(kJob);
    if (handleErrors(kJob)) {
        QTextStream ts(job->data());
        QString bibTeXcode = ts.readAll();

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);
        d->sanitizeBibTeXCode(bibTeXcode);

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL)
                    emit foundEntry(entry);
            }
            delete bibtexFile;
        }

        if (! d->bibTeXUrls.isEmpty()) {
            KIO::TransferJob *newJob = KIO::storedGet(d->bibTeXUrls.first());
            connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob*)));
            d->bibTeXUrls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << job->url();
}
