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

#include "kbibtex.h"

#include <QString>
#include <QRegularExpression>

const QString KBibTeX::extensionTeX = QStringLiteral(".tex");
const QString KBibTeX::extensionAux = QStringLiteral(".aux");
const QString KBibTeX::extensionBBL = QStringLiteral(".bbl");
const QString KBibTeX::extensionBLG = QStringLiteral(".blg");
const QString KBibTeX::extensionBibTeX = QStringLiteral(".bib");
const QString KBibTeX::extensionPDF = QStringLiteral(".pdf");
const QString KBibTeX::extensionPostScript = QStringLiteral(".ps");
const QString KBibTeX::extensionRTF = QStringLiteral(".rtf");

const QString KBibTeX::Months[] = {
    QStringLiteral("January"), QStringLiteral("February"), QStringLiteral("March"), QStringLiteral("April"), QStringLiteral("May"), QStringLiteral("June"), QStringLiteral("July"), QStringLiteral("August"), QStringLiteral("September"), QStringLiteral("October"), QStringLiteral("November"), QStringLiteral("December")
};

const QString KBibTeX::MonthsTriple[] = {
    QStringLiteral("jan"), QStringLiteral("feb"), QStringLiteral("mar"), QStringLiteral("apr"), QStringLiteral("may"), QStringLiteral("jun"), QStringLiteral("jul"), QStringLiteral("aug"), QStringLiteral("sep"), QStringLiteral("oct"), QStringLiteral("nov"), QStringLiteral("dec")
};

const QRegularExpression KBibTeX::fileListSeparatorRegExp(QStringLiteral("[ \\t]*[;\\n]+[ \\t]*"));
const QRegularExpression KBibTeX::fileRegExp(QStringLiteral("(\\bfile:)?[^{}\\t]+\\.\\w{2,4}\\b"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::urlRegExp(QStringLiteral("\\b(http|s?ftp|webdav|file)s?://[^ {}\"]+(\\b|[/])"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::doiRegExp(QStringLiteral("10([.][0-9]+)+/[/-a-z0-9.()<>_:;\\\\]+"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::mendeleyFileRegExp(QStringLiteral(":(.*):pdf"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::domainNameRegExp(QStringLiteral("[a-z0-9.-]+\\.((a[cdefgilmnoqrstuwxz]|aero|arpa)|(b[abdefghijmnorstvwyz]|biz)|(c[acdfghiklmnorsuvxyz]|cat|com|coop)|d[ejkmoz]|(e[ceghrstu]|edu)|f[ijkmor]|(g[abdefghilmnpqrstuwy]|gov)|h[kmnrtu]|(i[delmnoqrst]|info|int)|(j[emop]|jobs)|k[eghimnprwyz]|l[abcikrstuvy]|(m[acdghklmnopqrstuvwxyz]|me|mil|mobi|museum)|(n[acefgilopruz]|name|net)|(om|org)|(p[aefghklmnrstwy]|pro)|qa|r[eouw]|s[abcdeghijklmnortvyz]|(t[cdfghjklmnoprtvwz]|travel)|u[agkmsyz]|v[aceginu]|w[fs]|y[etu]|z[amw])\\b"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::htmlRegExp(QStringLiteral("</?(a|pre|p|br|span|i|b|italic)\\b[^>{}]{,32}>"), QRegularExpression::CaseInsensitiveOption);
const QString KBibTeX::doiUrlPrefix(QStringLiteral("https://dx.doi.org/")); ///< for internal use in FileInfo::doiUrlPrefix, use FileInfo::doiUrlPrefix() instead

bool KBibTeX::isLocalOrRelative(const QUrl &url)
{
    return url.isLocalFile() || url.isRelative() || url.scheme().isEmpty();
}

/**
 * Poor man's variant of a text-squeezing function.
 * Effect is similar as observed in KSqueezedTextLabel:
 * If the text is longer as n characters, the middle part
 * will be cut away and replaced by "..." to get a
 * string of max n characters.
 */
QString KBibTeX::squeezeText(const QString &text, int n)
{
    return text.length() <= n ? text : text.left(n / 2 - 1) + QStringLiteral("...") + text.right(n / 2 - 2);
}

QString KBibTeX::leftSqueezeText(const QString &text, int n)
{
    return text.length() <= n ? text : text.left(n) + QStringLiteral("...");
}

int KBibTeX::validateCurlyBracketContext(const QString &text)
{
    int openingCB = 0, closingCB = 0;

    for (int i = 0; i < text.length(); ++i) {
        if (i == 0 || text[i - 1] != QLatin1Char('\\')) {
            if (text[i] == QLatin1Char('{')) ++openingCB;
            else if (text[i] == QLatin1Char('}')) ++closingCB;
        }
    }

    return openingCB - closingCB;
}
