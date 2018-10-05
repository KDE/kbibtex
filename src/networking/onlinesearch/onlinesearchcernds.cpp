/****************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de>  *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.  *
 ****************************************************************************/

#include "onlinesearchcernds.h"

#include <QUrlQuery>

#include <KLocalizedString>

#include "logging_networking.h"

OnlineSearchCERNDS::OnlineSearchCERNDS(QWidget *parent)
        : OnlineSearchSimpleBibTeXDownload(parent)
{
    /// nothing
}

QString OnlineSearchCERNDS::label() const
{
    return i18n("CERN Document Server");
}

QUrl OnlineSearchCERNDS::homepage() const
{
    return QUrl(QStringLiteral("http://cds.cern.ch/"));
}

QString OnlineSearchCERNDS::favIconUrl() const
{
    return QStringLiteral("http://cds.cern.ch/favicon.ico");
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
    QUrl url = QUrl(QStringLiteral("http://cds.cern.ch/search?ln=en&action_search=Search&c=Articles+%26+Preprints&as=1&sf=&so=d&rm=&sc=0&of=hx&f="));
    QUrlQuery q(url);
    /// Set number of expected results
    q.addQueryItem(QStringLiteral("rg"), QString::number(numResults));

    /// Number search arguments
    int argumentCount = 0;

    /// add words from "free text" field
    const QStringList freeTextWords = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    for (const QString &word : freeTextWords) {
        ++argumentCount;
        q.addQueryItem(QString(QStringLiteral("p%1")).arg(argumentCount), word);
        q.addQueryItem(QString(QStringLiteral("m%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("op%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(argumentCount), QString());
    }

    /// add words from "author" field
    const QStringList authorWords = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    for (const QString &word : authorWords) {
        ++argumentCount;
        q.addQueryItem(QString(QStringLiteral("p%1")).arg(argumentCount), word);
        q.addQueryItem(QString(QStringLiteral("m%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("op%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(argumentCount), QStringLiteral("author"));
    }

    /// add words from "title" field
    const QStringList titleWords = splitRespectingQuotationMarks(query[queryKeyTitle]);
    for (const QString &word : titleWords) {
        ++argumentCount;
        q.addQueryItem(QString(QStringLiteral("p%1")).arg(argumentCount), word);
        q.addQueryItem(QString(QStringLiteral("m%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("op%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(argumentCount), QStringLiteral("title"));
    }

    /// add words from "title" field
    const QString year = query[queryKeyYear];
    if (!year.isEmpty()) {
        ++argumentCount;
        q.addQueryItem(QString(QStringLiteral("p%1")).arg(argumentCount), year);
        q.addQueryItem(QString(QStringLiteral("m%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("op%1")).arg(argumentCount), QStringLiteral("a"));
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(argumentCount), QStringLiteral("year"));
    }

    url.setQuery(q);
    return url;
}
