/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *   SPDX-FileCopyrightText: 2014 Pavel Zorin-Kranich <pzorin@math.uni-bonn.de>
 *   SPDX-FileCopyrightText: 2018 Alexander Dunlap <alexander.dunlap@gmail.com>
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "onlinesearchmrlookup.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QMap>
#include <QUrlQuery>

#include <KLocalizedString>

#include <KBibTeX>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

const QString OnlineSearchMRLookup::queryUrlStem = QStringLiteral("https://mathscinet.ams.org/mrlookup");

OnlineSearchMRLookup::OnlineSearchMRLookup(QObject *parent)
        : OnlineSearchAbstract(parent)
{
    /// nothing
}

void OnlineSearchMRLookup::startSearch(const QMap<QueryKey, QString> &query, int)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QUrl url(queryUrlStem);
    QUrlQuery q(url);

    const QString title = query[QueryKey::Title];
    q.addQueryItem(QStringLiteral("ti"), title);

    const QString authors = query[QueryKey::Author];
    q.addQueryItem(QStringLiteral("au"), authors);

    const QString year = query[QueryKey::Year];
    if (!year.isEmpty())
        q.addQueryItem(QStringLiteral("year"), year);

    q.addQueryItem(QStringLiteral("format"), QStringLiteral("bibtex"));

    url.setQuery(q);
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchMRLookup::doneFetchingResultPage);

    refreshBusyProperty();
}

QString OnlineSearchMRLookup::label() const
{
    return i18n("MR Lookup");
}

QUrl OnlineSearchMRLookup::homepage() const
{
    return QUrl(QStringLiteral("https://mathscinet.ams.org/mrlookup"));
}

void OnlineSearchMRLookup::doneFetchingResultPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(curStep = numSteps, numSteps);

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlCode = QString::fromUtf8(reply->readAll().constData());

        QString bibtexCode;
        int p1 = -1, p2 = -1;
        while ((p1 = htmlCode.indexOf(QStringLiteral("<pre>"), p2 + 1)) >= 0 && (p2 = htmlCode.indexOf(QStringLiteral("</pre>"), p1 + 1)) >= 0) {
            bibtexCode += htmlCode.midRef(p1 + 5, p2 - p1 - 5);
            bibtexCode += QLatin1Char('\n');
        }

        FileImporterBibTeX importer(this);
        File *bibtexFile = importer.fromString(bibtexCode);

        bool hasEntry = false;
        if (bibtexFile != nullptr) {
            for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                const QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                hasEntry |= publishEntry(entry);
            }
            delete bibtexFile;
        }

        stopSearch(hasEntry ? resultNoError : resultUnspecifiedError);
    }

    refreshBusyProperty();
}

void OnlineSearchMRLookup::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

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
        if (v.containsPattern(QStringLiteral("https://dx.doi.org"))) {
            entry->remove(Entry::ftUrl);
        }
    }
}
