/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchsoanasaads.h"

#include <KLocalizedString>

OnlineSearchSOANASAADS::OnlineSearchSOANASAADS(QWidget *parent)
        : OnlineSearchSimpleBibTeXDownload(parent)
{
    /// nothing
}

QString OnlineSearchSOANASAADS::label() const
{
    return i18n("SAO/NASA Astrophysics Data System (ADS)");
}

QUrl OnlineSearchSOANASAADS::homepage() const
{
    return QUrl(QStringLiteral("http://adswww.harvard.edu/"));
}

QString OnlineSearchSOANASAADS::favIconUrl() const
{
    return QStringLiteral("http://adsabs.harvard.edu/favicon.ico");
}

QUrl OnlineSearchSOANASAADS::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
    static const QString globalSearch = QStringLiteral("\"%1\"");
    static const QString rangeSearch = QStringLiteral("%1:\"%2\"");

    // TODO
    /// http://adsabs.harvard.edu/cgi-bin/basic_connect?qsearch=Hansen&version=1&data_type=BIBTEXPLUS&type=FILE&sort=NDATE&nr_to_return=5

    const QStringList freeTextWords = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    const QStringList yearWords = splitRespectingQuotationMarks(query[queryKeyYear]);
    const QStringList titleWords = splitRespectingQuotationMarks(query[queryKeyTitle]);
    const QStringList authorWords = splitRespectingQuotationMarks(query[queryKeyAuthor]);

    /// append search terms
    QStringList queryFragments;
    queryFragments.reserve(freeTextWords.size() + yearWords.size() + titleWords.size() + authorWords.size());

    /// add words from "free text" field
    for (const QString &word : freeTextWords)
        queryFragments.append(globalSearch.arg(word));

    /// add words from "year" field
    for (const QString &word : yearWords)
        queryFragments.append(word); ///< no quotation marks around years

    /// add words from "title" field
    for (const QString &word : titleWords)
        queryFragments.append(rangeSearch.arg(QStringLiteral("intitle"), word));

    /// add words from "author" field
    for (const QString &word : authorWords)
        queryFragments.append(rangeSearch.arg(QStringLiteral("author"), word));

    /// Build URL
    QString urlText = QStringLiteral("http://adsabs.harvard.edu/cgi-bin/basic_connect?version=1&data_type=BIBTEXPLUS&type=FILE&sort=NDATE&qsearch=");
    urlText.append(queryFragments.join(QStringLiteral("+")).replace(QLatin1Char('"'), QStringLiteral("%22")));
    /// set number of expected results
    urlText.append(QString(QStringLiteral("&nr_to_return=%1")).arg(numResults));

    return QUrl(urlText);
}

QString OnlineSearchSOANASAADS::processRawDownload(const QString &download) {
    /// Skip HTML header and some other non-BibTeX content
    const int p1 = download.indexOf(QStringLiteral("abstracts, starting"));
    if (p1 > 1) {
        const int p2 = download.indexOf(QStringLiteral("\n"), p1);
        if (p2 > p1)
            return download.mid(p2 + 1);
    }

    /// Skipping header failed, return input text as it was
    return download;
}
