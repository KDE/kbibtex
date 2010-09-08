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

#include <QtDBus/QtDBus>

#include <KLocale>
#include <KMessageBox>
#include <KDebug>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <fileimporterbibtex.h>
#include "websearchgooglescholar.h"

static const QString startPageUrl = QLatin1String("http://scholar.google.com/?hl=en");
static const QString configPageUrl = QLatin1String("http://%1/scholar_preferences?hl=en");
static const QString setConfigPageUrl = QLatin1String("http://%1/scholar_setprefs?hl=en&inststart=0&lang=all&scis=yes&scisf=4&num=%2");
static const QString queryPageUrl = QLatin1String("http://%1/scholar?hl=en&num=%2&q=%3&btnG=Search");
static const QRegExp linkToBib(QLatin1String("/scholar.bib\\?[^\" >]+"));

FileImporterBibTeX importer;

void WebSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_numResults = numResults;
    m_hasBeenCancelled = false;
    m_currentJob = NULL;

    QStringList queryFragments;
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        queryFragments << it.value();
    }
    m_queryString = queryFragments.join(" ");

    KIO::TransferJob *job = KIO::get(startPageUrl, KIO::Reload);
    job->addMetaData("cookies", "auto");
    job->addMetaData("cache", "reload");
    connect(job, SIGNAL(result(KJob *)), this, SLOT(doneFetchingStartPage(KJob*)));
    m_currentJob = job;
}

void WebSearchGoogleScholar::doneFetchingStartPage(KJob *kJob)
{
    m_currentJob = NULL;
    if (m_hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::TransferJob *transferJob = static_cast<KIO::TransferJob *>(kJob);
    kDebug() << "Google host: " << transferJob->url().host();

    KIO::TransferJob * newJob = KIO::get(configPageUrl.arg(transferJob->url().host()), KIO::Reload);
    newJob->addMetaData("cookies", "auto");
    newJob->addMetaData("cache", "reload");
    connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingConfigPage(KJob*)));
    m_currentJob = newJob;
}

void WebSearchGoogleScholar::doneFetchingConfigPage(KJob *kJob)
{
    m_currentJob = NULL;
    if (m_hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::TransferJob *transferJob = static_cast<KIO::TransferJob *>(kJob);

    KIO::TransferJob * newJob = KIO::get(setConfigPageUrl.arg(transferJob->url().host()).arg(m_numResults), KIO::Reload);
    newJob->addMetaData("cookies", "auto");
    newJob->addMetaData("cache", "reload");
    connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingSetConfigPage(KJob*)));
    m_currentJob = newJob;
}

void WebSearchGoogleScholar::doneFetchingSetConfigPage(KJob *kJob)
{
    m_currentJob = NULL;
    if (m_hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        emit stoppedSearch(resultCancelled);
        return;
    } else  if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::TransferJob *transferJob = static_cast<KIO::TransferJob *>(kJob);

    KIO::TransferJob * newJob = KIO::storedGet(queryPageUrl.arg(transferJob->url().host()).arg(m_numResults).arg(m_queryString), KIO::Reload);
    newJob->addMetaData("cookies", "auto");
    newJob->addMetaData("cache", "reload");
    connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingQueryPage(KJob*)));
    m_currentJob = newJob;
}

void WebSearchGoogleScholar::doneFetchingQueryPage(KJob *kJob)
{
    m_currentJob = NULL;
    if (m_hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

    QString htmlText(transferJob->data());
    int pos = 0;
    m_listBibTeXurls.clear();
    while ((pos = linkToBib.indexIn(htmlText, pos)) != -1) {
        m_listBibTeXurls << "http://" + transferJob->url().host() + linkToBib.cap(0).replace("&amp;", "&");
        pos += linkToBib.matchedLength();
    }

    if (!m_listBibTeXurls.isEmpty()) {
        KIO::TransferJob * newJob = KIO::storedGet(m_listBibTeXurls.first(), KIO::Reload);
        m_listBibTeXurls.removeFirst();
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob*)));
        m_currentJob = newJob;
    } else {
        kWarning() << "Searching " << label() << " resulted in no hits";
        QStringList domainList;
        domainList << "scholar.google.com";
        QString ccSpecificDomain = transferJob->url().host();
        if (!domainList.contains(ccSpecificDomain))
            domainList << ccSpecificDomain;
        KMessageBox::information(m_parent, i18n("<qt><p>No hits were found in Google Scholar.</p><p>One possible reason is that cookies are disabled for the following domains:</p><ul><li>%1</li></ul><p>In Konqueror's cookie settings, these domains can be allowed to set cookies in your system.</p></qt>", domainList.join("</li><li>")), label());
        emit stoppedSearch(resultUnspecifiedError);
    }
}

void WebSearchGoogleScholar::doneFetchingBibTeX(KJob *kJob)
{
    m_currentJob = NULL;
    if (m_hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    // FIXME has this section to be protected by a mutex?
    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);
    QByteArray ba(transferJob->data());
    QBuffer buffer(&ba);
    buffer.open(QIODevice::ReadOnly);
    File *bibtexFile = importer.load(&buffer);
    buffer.close();

    Entry *entry = NULL;
    if (bibtexFile != NULL) {
        for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
            entry = dynamic_cast<Entry*>(*it);
            if (entry != NULL) {
                Value v;
                v << new PlainText(i18n("%1 with search term \"%2\"", label(), m_queryString));
                entry->insert(QLatin1String("kbibtex-websearch"), v);
                emit foundEntry(entry);
            }
        }
        delete bibtexFile;
    }

    if (entry == NULL) {
        kWarning() << "Searching " << label() << " resulted in invalid BibTeX data: " << QString(transferJob->data());
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    if (!m_listBibTeXurls.isEmpty()) {
        KIO::TransferJob * newJob = KIO::storedGet(m_listBibTeXurls.first(), KIO::Reload);
        m_listBibTeXurls.removeFirst();
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob*)));
        m_currentJob = newJob;
    } else
        emit stoppedSearch(resultNoError);
}

QString WebSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

void WebSearchGoogleScholar::cancel()
{
    if (m_currentJob != NULL)
        m_currentJob->kill(KJob::EmitResult);
    m_hasBeenCancelled = true;
}
