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

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextStream>

#include <KDebug>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "onlinesearchsimplebibtexdownload.h"

class OnlineSearchSimpleBibTeXDownload::OnlineSearchSimpleBibTeXDownloadPrivate
{
private:
    OnlineSearchSimpleBibTeXDownload *p;

public:
    int numSteps, curStep;

    OnlineSearchSimpleBibTeXDownloadPrivate(OnlineSearchSimpleBibTeXDownload *parent)
            : p(parent) {
        // nothing
    }

    void sanitizeEntry(QSharedPointer<Entry> entry) {
        /// if entry contains a description field but no abstract,
        /// rename description field to abstract
        const QString ftDescription = QLatin1String("description");
        if (!entry->contains(Entry::ftAbstract) && entry->contains(ftDescription)) {
            Value v = entry->value(QLatin1String("description"));
            entry->insert(Entry::ftAbstract, v);
            entry->remove(ftDescription);
        }
    }
};

OnlineSearchSimpleBibTeXDownload::OnlineSearchSimpleBibTeXDownload(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchSimpleBibTeXDownloadPrivate(this))
{
    // nothing
}

OnlineSearchSimpleBibTeXDownload::~OnlineSearchSimpleBibTeXDownload()
{
    delete d;
}

void OnlineSearchSimpleBibTeXDownload::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchSimpleBibTeXDownload::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 1;

    QNetworkRequest request(buildQueryUrl(query, numResults));
    kDebug()<<"request url="<<request.url().toString();
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);
}

void OnlineSearchSimpleBibTeXDownload::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchSimpleBibTeXDownload::downloadDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        ts.setCodec("utf-8");
        QString bibTeXcode = ts.readAll();

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    if (!entry.isNull()) {
                        Value v;
                        v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                        entry->insert("x-fetchedfrom", v);
                        d->sanitizeEntry(entry);
                        emit foundEntry(entry);
                        hasEntries = true;
                    }

                }

                if (!hasEntries)
                    kDebug() << "No hits found in" << reply->url().toString();
                emit stoppedSearch(resultNoError);
                emit progress(d->numSteps, d->numSteps);

                delete bibtexFile;
            } else {
                kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                emit stoppedSearch(resultUnspecifiedError);
            }
        } else {
            /// returned file is empty
            kDebug() << "No hits found in" << reply->url().toString();
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
