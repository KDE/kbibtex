/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

OnlineSearchInspireHep::OnlineSearchInspireHep(QWidget *parent)
        : OnlineSearchSimpleBibTeXDownload(parent)
{
    // nothing
}

QString OnlineSearchInspireHep::label() const
{
    return i18n("Inspire High-Energy Physics Literature Database");
}

QUrl OnlineSearchInspireHep::homepage() const
{
    return QUrl(QStringLiteral("http://inspirehep.net/"));
}

QString OnlineSearchInspireHep::favIconUrl() const
{
    return QStringLiteral("http://inspirehep.net/favicon.ico");
}

QUrl OnlineSearchInspireHep::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
    static const QString typedSearch = QStringLiteral("%1 %2"); ///< no quotation marks for search term?

    /// append search terms
    QStringList queryFragments;

    /// add words from "free text" field
    QStringList freeTextWords = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    for (QStringList::ConstIterator it = freeTextWords.constBegin(); it != freeTextWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QStringLiteral("ft"), *it));

    /// add words from "year" field
    QStringList yearWords = splitRespectingQuotationMarks(query[queryKeyYear]);
    for (QStringList::ConstIterator it = yearWords.constBegin(); it != yearWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QStringLiteral("d"), *it));

    /// add words from "title" field
    QStringList titleWords = splitRespectingQuotationMarks(query[queryKeyTitle]);
    for (QStringList::ConstIterator it = titleWords.constBegin(); it != titleWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QStringLiteral("t"), *it));

    /// add words from "author" field
    QStringList authorWords = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    for (QStringList::ConstIterator it = authorWords.constBegin(); it != authorWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QStringLiteral("a"), *it));

    /// Build URL
    QString urlText = QStringLiteral("http://inspirehep.net/search?ln=en&ln=en&of=hx&action_search=Search&sf=&so=d&rm=&sc=0");
    /// Set number of expected results
    urlText.append(QString(QStringLiteral("&rg=%1")).arg(numResults));
    /// Append actual query
    urlText.append(QStringLiteral("&p="));
    urlText.append(queryFragments.join(QStringLiteral(" and ")));
    /// URL-encode text
    urlText = urlText.replace(QLatin1Char(' '), QStringLiteral("%20")).replace(QLatin1Char('"'), QStringLiteral("%22"));

    return QUrl(urlText);
}
