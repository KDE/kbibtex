/***************************************************************************
 *   Copyright (C) 2004-2020 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchinspirehep.h"

#include <KLocalizedString>

OnlineSearchInspireHep::OnlineSearchInspireHep(QObject *parent)
        : OnlineSearchSimpleBibTeXDownload(parent)
{
    /// nothing
}

QString OnlineSearchInspireHep::label() const
{
    return i18n("Inspire High-Energy Physics Literature Database");
}

QUrl OnlineSearchInspireHep::homepage() const
{
    return QUrl(QStringLiteral("https://inspirehep.net/"));
}

QString OnlineSearchInspireHep::favIconUrl() const
{
    return QStringLiteral("https://inspirehep.net/favicon.ico");
}

QUrl OnlineSearchInspireHep::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
    static const QString typedSearch = QStringLiteral("%1 %2");
    static const QString typedSearchWithQuote = QStringLiteral("%1 \"%2\"");
#define generateTypedSearch(kw,txt) (txt.contains(QChar(0x20))?typedSearchWithQuote.arg(kw,txt):typedSearch.arg(kw,txt))

    const QStringList freeTextWords = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    const QStringList yearWords = splitRespectingQuotationMarks(query[queryKeyYear]);
    const QStringList titleWords = splitRespectingQuotationMarks(query[queryKeyTitle]);
    const QStringList authorWords = splitRespectingQuotationMarks(query[queryKeyAuthor]);

    /// append search terms
    QStringList queryFragments;
    queryFragments.reserve(freeTextWords.size() + yearWords.size() + titleWords.size() + authorWords.size());

    /// add words from "free text" field
    for (const QString &text : freeTextWords)
        queryFragments.append(generateTypedSearch(QStringLiteral("ft"), text));

    /// add words from "year" field
    for (const QString &text : yearWords)
        queryFragments.append(generateTypedSearch(QStringLiteral("d"), text));

    /// add words from "title" field
    for (const QString &text : titleWords)
        queryFragments.append(generateTypedSearch(QStringLiteral("t"), text));

    /// add words from "author" field
    for (const QString &text : authorWords)
        queryFragments.append(generateTypedSearch(QStringLiteral("a"), text));

    /// Build URL
    /// Documentation on REST API can be found here:
    ///   https://github.com/inspirehep/rest-api-doc
    ///   https://help.inspirehep.net/knowledge-base/inspire-paper-search/
    return QUrl::fromUserInput(QString(QStringLiteral("https://inspirehep.net/api/literature?size=%2&format=bibtex&sort=mostrecent&q=%1")).arg(queryFragments.join(QStringLiteral(" and ")), QString::number(numResults)));
}
