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
    Q_UNUSED(numResults) /// Bibsonomy does not allow to control number of results
// TODO
    QStringList queryFragments;
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        queryFragments << it.value();
    }
    KUrl url(QLatin1String("http://www.bibsonomy.org/bib/search/") + queryFragments.join(" "));
    kDebug() << "url = " << url;

    m_buffer.clear();

    m_job =  KIO::get(url);
    connect(m_job, SIGNAL(data(KIO::Job *, const QByteArray &)), this, SLOT(data(KIO::Job*, const QByteArray&)));
    connect(m_job, SIGNAL(result(KJob *)), this, SLOT(jobDone(KJob*)));
}

QString WebSearchBibsonomy::label() const
{
    return i18n("Bibsonomy");
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
                if (entry != NULL)
                    emit foundEntry(entry);
            }
        }
        emit stoppedSearch(bibtexFile == NULL || bibtexFile->isEmpty() ? resultUnspecifiedError : resultNoError);
        delete bibtexFile;
    } else {
        kWarning() << "Search using " << label() << "failed: " << job->errorString();
        emit stoppedSearch(resultUnspecifiedError);
    }
}
