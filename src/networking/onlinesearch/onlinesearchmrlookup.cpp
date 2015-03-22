/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                 2014 Pavel Zorin-Kranich <pzorin@math.uni-bonn.de>      *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "onlinesearchmrlookup.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QMap>
#include <QDebug>

#include <KLocale>

#include "fileimporterbibtex.h"
#include "kbibtexnamespace.h"
#include "internalnetworkaccessmanager.h"

const QString OnlineSearchMRLookup::queryUrlStem = QLatin1String("http://www.ams.org/mrlookup");

OnlineSearchMRLookup::OnlineSearchMRLookup(QWidget *parent)
        : OnlineSearchAbstract(parent)
{
    /// nothing
}

void OnlineSearchMRLookup::startSearch(const QMap<QString, QString> &query, int)
{
    m_hasBeenCanceled = false;

    QUrl url(queryUrlStem);

    const QString title = query[queryKeyTitle];
    url.addQueryItem(QLatin1String("ti"), title);

    const QString authors = query[queryKeyAuthor];
    url.addQueryItem(QLatin1String("au"), authors);

    const QString year = query[queryKeyYear];
    if (!year.isEmpty())
        url.addQueryItem(QLatin1String("year"), year);

    url.addQueryItem(QLatin1String("format"), QLatin1String("bibtex"));

    emit progress(0, 1);

    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
}

void OnlineSearchMRLookup::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

QString OnlineSearchMRLookup::label() const
{
    return i18n("MR Lookup");
}

QString OnlineSearchMRLookup::favIconUrl() const
{
    return QLatin1String("http://www.ams.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchMRLookup::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchMRLookup::homepage() const
{
    return QUrl("http://www.ams.org/mrlookup");
}

void OnlineSearchMRLookup::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchMRLookup::doneFetchingResultPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(1, 1);

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlCode = QString::fromUtf8(reply->readAll().data());

        QString bibtexCode;
        int p1 = -1, p2 = -1;
        while ((p1 = htmlCode.indexOf(QLatin1String("<pre>"), p2 + 1)) >= 0 && (p2 = htmlCode.indexOf(QLatin1String("</pre>"), p1 + 1)) >= 0) {
            bibtexCode += htmlCode.mid(p1 + 5, p2 - p1 - 5) + QChar('\n');
        }

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibtexCode);

        bool hasEntry = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntry |= publishEntry(entry);
            }
            delete bibtexFile;
        }

        emit stoppedSearch(hasEntry ? resultNoError : resultUnspecifiedError);
    } else
        qWarning() << "url was" << reply->url().toString();
}

void OnlineSearchMRLookup::sanitizeEntry(QSharedPointer<Entry> entry)
{
    /// Rewrite 'fjournal' fields to become 'journal' fields
    /// (overwriting them if necessary)
    const QString ftFJournal = QLatin1String("fjournal");
    if (entry->contains(ftFJournal)) {
        Value v = entry->value(ftFJournal);
        entry->remove(Entry::ftJournal);
        entry->remove(ftFJournal);
        entry->insert(Entry::ftJournal, v);
    }

    /// Remove URL from entry if contains a DOI and the DOI field is present
    if (entry->contains(Entry::ftDOI) && entry->contains(Entry::ftUrl)) {
        Value v = entry->value(Entry::ftUrl);
        if (v.containsPattern("http://dx.doi.org")) {
            entry->remove(Entry::ftUrl);
        }
    }
}
