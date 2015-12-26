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
#include <QUrlQuery>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "kbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

const QString OnlineSearchMRLookup::queryUrlStem = QStringLiteral("http://www.ams.org/mrlookup");

OnlineSearchMRLookup::OnlineSearchMRLookup(QWidget *parent)
        : OnlineSearchAbstract(parent)
{
    /// nothing
}

void OnlineSearchMRLookup::startSearch(const QMap<QString, QString> &query, int)
{
    m_hasBeenCanceled = false;

    QUrl url(queryUrlStem);
    QUrlQuery q(url);

    const QString title = query[queryKeyTitle];
    q.addQueryItem(QStringLiteral("ti"), title);

    const QString authors = query[queryKeyAuthor];
    q.addQueryItem(QStringLiteral("au"), authors);

    const QString year = query[queryKeyYear];
    if (!year.isEmpty())
        q.addQueryItem(QStringLiteral("year"), year);

    q.addQueryItem(QStringLiteral("format"), QStringLiteral("bibtex"));

    emit progress(0, 1);

    url.setQuery(q);
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
    return QStringLiteral("http://www.ams.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchMRLookup::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchMRLookup::homepage() const
{
    return QUrl(QStringLiteral("http://www.ams.org/mrlookup"));
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
        while ((p1 = htmlCode.indexOf(QStringLiteral("<pre>"), p2 + 1)) >= 0 && (p2 = htmlCode.indexOf(QStringLiteral("</pre>"), p1 + 1)) >= 0) {
            bibtexCode += htmlCode.midRef(p1 + 5, p2 - p1 - 5);
            bibtexCode += QLatin1Char('\n');
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
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchMRLookup::sanitizeEntry(QSharedPointer<Entry> entry)
{
    /// Rewrite 'fjournal' fields to become 'journal' fields
    /// (overwriting them if necessary)
    const QString ftFJournal = QStringLiteral("fjournal");
    if (entry->contains(ftFJournal)) {
        Value v = entry->value(ftFJournal);
        entry->remove(Entry::ftJournal);
        entry->remove(ftFJournal);
        entry->insert(Entry::ftJournal, v);
    }

    /// Remove URL from entry if contains a DOI and the DOI field is present
    if (entry->contains(Entry::ftDOI) && entry->contains(Entry::ftUrl)) {
        Value v = entry->value(Entry::ftUrl);
        if (v.containsPattern(QStringLiteral("http://dx.doi.org"))) {
            entry->remove(Entry::ftUrl);
        }
    }
}
