/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchsimplebibtexdownload.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>

#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

OnlineSearchSimpleBibTeXDownload::OnlineSearchSimpleBibTeXDownload(QObject *parent)
        : OnlineSearchAbstract(parent)
{
    /// nothing
}

void OnlineSearchSimpleBibTeXDownload::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 2);

    QNetworkRequest request(buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchSimpleBibTeXDownload::downloadDone);

    refreshBusyProperty();
}

QString OnlineSearchSimpleBibTeXDownload::processRawDownload(const QString &download) {
    /// Default implementation does not do anything
    return download;
}

void OnlineSearchSimpleBibTeXDownload::downloadDone()
{
    Q_EMIT progress(++curStep, numSteps = 2);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (bibTeXcode.contains(QStringLiteral("<html")) || bibTeXcode.contains(QStringLiteral("<HTML"))) {
            /// Replace all linebreak-like characters, in case they occur inside the BibTeX code
            static const QRegularExpression htmlLinebreakRegExp(QStringLiteral("<[/]?(br|p)[^>]*[/]?>"));
            bibTeXcode = bibTeXcode.remove(htmlLinebreakRegExp);

            /// Find first BibTeX entry in HTML code, clip away all HTML code before that
            static const QRegularExpression elementTypeRegExp(QStringLiteral("[@]\\S+\\{"));
            int p1 = -1;
            QRegularExpressionMatchIterator elementTypeRegExpMatchIt = elementTypeRegExp.globalMatch(bibTeXcode);
            while (p1 < 0 && elementTypeRegExpMatchIt.hasNext()) {
                const QRegularExpressionMatch elementTypeRegExpMatch = elementTypeRegExpMatchIt.next();
                /// Hop over JavaScript's "@import" statements
                if (elementTypeRegExpMatch.captured(0) != QStringLiteral("@import{"))
                    p1 = elementTypeRegExpMatch.capturedStart();
            }
            if (p1 > 1)
                bibTeXcode = bibTeXcode.mid(p1);

            /// Find HTML code after BibTeX code, clip that away, too
            static const QRegularExpression htmlContinuationRegExp(QStringLiteral("<[/]?\\S+"));
            p1 = bibTeXcode.indexOf(htmlContinuationRegExp);
            if (p1 > 1)
                bibTeXcode = bibTeXcode.left(p1 - 1);
        }

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer(this);
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                    hasEntries |= publishEntry(entry);
                }

                stopSearch(resultNoError);

                delete bibtexFile;
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                stopSearch(resultUnspecifiedError);
            }
        } else {
            /// returned file is empty
            stopSearch(resultNoError);
        }
    }

    refreshBusyProperty();
}
