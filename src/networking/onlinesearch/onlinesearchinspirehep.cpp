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

#include <KLocale>

#include "onlinesearchinspirehep.h"

OnlineSearchInpireHep::OnlineSearchInpireHep(QWidget *parent)
    : OnlineSearchSimpleBibTeXDownload(parent)
{
    // nothing
}

QString OnlineSearchInpireHep::label() const
{
    return i18n("Inpsire High-Energy Physics Literature Database");
}

OnlineSearchQueryFormAbstract *OnlineSearchInpireHep::customWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return NULL;
}

KUrl OnlineSearchInpireHep::homepage() const
{
    return KUrl("http://inspirehep.net/");
}

QString OnlineSearchInpireHep::favIconUrl() const
{
    return QLatin1String("inspirehep.net/favicon.ico");
}

KUrl OnlineSearchInpireHep::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
    static const QString typedSearch = QLatin1String("%1 %2");

    // TODO
    /// http://adsabs.harvard.edu/cgi-bin/basic_connect?qsearch=Hansen&version=1&data_type=BIBTEXPLUS&type=FILE&sort=NDATE&nr_to_return=5

    /// append search terms
    QStringList queryFragments;

    /// add words from "free text" field
    QStringList freeTextWords = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    for (QStringList::ConstIterator it = freeTextWords.constBegin(); it != freeTextWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QLatin1String("ft")).arg(*it));

    /// add words from "year" field
    QStringList yearWords = splitRespectingQuotationMarks(query[queryKeyYear]);
    for (QStringList::ConstIterator it = yearWords.constBegin(); it != yearWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QLatin1String("d")).arg(*it));

    /// add words from "title" field
    QStringList titleWords = splitRespectingQuotationMarks(query[queryKeyTitle]);
    for (QStringList::ConstIterator it = titleWords.constBegin(); it != titleWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QLatin1String("t")).arg(*it));

    /// add words from "author" field
    QStringList authorWords = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    for (QStringList::ConstIterator it = authorWords.constBegin(); it != authorWords.constEnd(); ++it)
        queryFragments.append(typedSearch.arg(QLatin1String("a")).arg(*it));

    /// Build URL
    QString urlText = QLatin1String("http://inspirehep.net/search?ln=en&ln=en&of=hx&action_search=Search&sf=&so=d&rm=&sc=0");
    /// Set number of expected results
    urlText.append(QString(QLatin1String("&rg=%1")).arg(numResults));
    /// Append actual query
    urlText.append(QLatin1String("&p="));
    urlText.append(queryFragments.join(" and "));
    /// URL-encode text
    urlText = urlText.replace(" ", "%20").replace("\"", "%22");

    return KUrl(urlText);
}
