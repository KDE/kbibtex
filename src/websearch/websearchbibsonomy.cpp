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

#include <KLocale>
#include <KDebug>
#include <kio/job.h>

#include <fileimporterbibtex.h>
#include <file.h>
#include <entry.h>
#include "websearchbibsonomy.h"

void WebSearchBibsonomy::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_buffer.clear();

    m_job = KIO::get(buildQueryUrl(query, numResults));
    connect(m_job, SIGNAL(data(KIO::Job *, const QByteArray &)), this, SLOT(data(KIO::Job*, const QByteArray&)));
    connect(m_job, SIGNAL(result(KJob *)), this, SLOT(jobDone(KJob*)));
}

QString WebSearchBibsonomy::label() const
{
    return i18n("Bibsonomy");
}

KUrl WebSearchBibsonomy::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
    bool hasFreeText = !query[queryKeyFreeText].isEmpty();
    bool hasTitle = !query[queryKeyTitle].isEmpty();
    bool hasAuthor = !query[queryKeyAuthor].isEmpty();
    bool hasYear = !query[queryKeyYear].isEmpty();

    QString searchType = "search";
    if (hasAuthor && !hasFreeText && !hasTitle && !hasYear) {
        /// if only the author field is used, a special author search
        /// on BibSonomy can be used
        searchType = "author";
    }

    QStringList queryFragments;
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        queryFragments << it.value();
    }

    m_queryString = queryFragments.join("+");
    // FIXME: Number of results doesn't seem to be supported by BibSonomy
    KUrl url(QLatin1String("http://www.bibsonomy.org/bib/") + searchType + "/" + m_queryString + QString("?.entriesPerPage=%1").arg(numResults));
    kDebug() << label() << " URL = " << url;

    return url;
}

void WebSearchBibsonomy::cancel()
{
    m_job->kill(KJob::EmitResult);
}

void WebSearchBibsonomy::data(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)
    m_buffer.append(data);
}

void WebSearchBibsonomy::jobDone(KJob *job)
{
    if (job->error() == 0) {
        QBuffer buffer(&m_buffer);
        buffer.open(QBuffer::ReadOnly);
        FileImporterBibTeX importer;
        File *bibtexFile = importer.load(&buffer);
        buffer.close();

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    Value v;
                    v << new PlainText(i18n("%1 with search term \"%2\"", label(), m_queryString));
                    entry->insert(QLatin1String("kbibtex-websearch"), v);
                    emit foundEntry(entry);
                }
            }
            emit stoppedSearch(bibtexFile->isEmpty() ? resultUnspecifiedError : resultNoError);
            delete bibtexFile;
        } else
            emit stoppedSearch(resultUnspecifiedError);
    } else {
        kWarning() << "Search using " << label() << "failed: " << job->errorString();
        emit stoppedSearch(resultUnspecifiedError);
    }
}
