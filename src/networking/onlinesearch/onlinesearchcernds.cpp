/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de>  *
 *   Copyright (C) 2013 Yngve I. Levinsen <yngve.inntjore.levinsen@cern.ch> *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include "onlinesearchcernds.h"

#include <KLocale>
#include <KDebug>

OnlineSearchCERNDS::OnlineSearchCERNDS(QWidget *parent)
        : OnlineSearchSimpleBibTeXDownload(parent)
{
    // nothing
}

QString OnlineSearchCERNDS::label() const
{
    return i18n("CERN Document Server");
}

OnlineSearchQueryFormAbstract *OnlineSearchCERNDS::customWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return NULL;
}

QUrl OnlineSearchCERNDS::homepage() const
{
    return QUrl("http://cds.cern.ch/");
}

QString OnlineSearchCERNDS::favIconUrl() const
{
    return QLatin1String("http://cds.cern.ch/favicon.ico");
}

QUrl OnlineSearchCERNDS::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
    /// Example for a search URL:
    /// http://cds.cern.ch/search?action_search=Search&sf=&so=d&rm=&sc=0&of=hx&f=&rg=10&ln=en&as=1&m1=a&p1=stone&f1=title&op1=a&m2=a&p2=smith&f2=author&op2=a&m3=a&p3=&f3=

    /// of=hx  asks for BibTeX results
    /// rg=10  asks for 10 results
    /// c=CERN+Document+Server or c=Articles+%26+Preprints   to limit scope
    /// Search search argument (X={1,2,3,...}):
    ///   pX   search text
    ///   mX   a=all words; o=any; e=exact phrase; p=partial phrase; r=regular expression
    ///   opX  a=AND; o=OR; n=AND NOT
    ///   fX   ""=any field; title; author; reportnumber; year; fulltext

    /// Build URL
    QUrl url = QUrl(QLatin1String("http://cds.cern.ch/search?ln=en&action_search=Search&c=Articles+%26+Preprints&as=1&sf=&so=d&rm=&sc=0&of=hx&f="));
    /// Set number of expected results
    url.addQueryItem(QLatin1String("rg"), QString::number(numResults));

    /// Number search arguments
    int argumentCount = 0;

    /// add words from "free text" field
    QStringList freeTextWords = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    foreach(const QString &word, freeTextWords) {
        ++argumentCount;
        url.addQueryItem(QString(QLatin1String("p%1")).arg(argumentCount), word);
        url.addQueryItem(QString(QLatin1String("m%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("op%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("f%1")).arg(argumentCount), QLatin1String(""));
    }

    /// add words from "author" field
    QStringList authorWords = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    foreach(const QString &word, authorWords) {
        ++argumentCount;
        url.addQueryItem(QString(QLatin1String("p%1")).arg(argumentCount), word);
        url.addQueryItem(QString(QLatin1String("m%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("op%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("f%1")).arg(argumentCount), QLatin1String("author"));
    }

    /// add words from "title" field
    QStringList titleWords = splitRespectingQuotationMarks(query[queryKeyTitle]);
    foreach(const QString &word, titleWords) {
        ++argumentCount;
        url.addQueryItem(QString(QLatin1String("p%1")).arg(argumentCount), word);
        url.addQueryItem(QString(QLatin1String("m%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("op%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("f%1")).arg(argumentCount), QLatin1String("title"));
    }

    /// add words from "title" field
    const QString year = query[queryKeyYear];
    if (!year.isEmpty()) {
        ++argumentCount;
        url.addQueryItem(QString(QLatin1String("p%1")).arg(argumentCount), year);
        url.addQueryItem(QString(QLatin1String("m%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("op%1")).arg(argumentCount), QLatin1String("a"));
        url.addQueryItem(QString(QLatin1String("f%1")).arg(argumentCount), QLatin1String("year"));
    }

    kDebug() << "url=" << url.pathOrUrl();
    return url;
}
