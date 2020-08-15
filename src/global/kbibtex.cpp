/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QHash>
#include <QDebug>

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
const QRegularExpression KBibTeX::urlRegExp(QStringLiteral("\\b((http|s?ftp|(web)?dav|imap|ipp|ldap|rtsp|sip|stun|turn)s?|mailto|jabber|xmpp|info|file)://[^ {}\"]+(\\b|[/])"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::doiRegExp(QStringLiteral("10([.][0-9]+)+/[-/a-z0-9.()<>_:;\\\\]+"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::arXivRegExpWithPrefix(QStringLiteral("arXiv:(([0-9]+[.][0-9]+|[a-z-]+/[0-9]+)(v[0-9]+)?)"));
const QRegularExpression KBibTeX::arXivRegExpWithoutPrefix(QStringLiteral("([0-9]+[.][0-9]+|[a-z-]+/[0-9]+)(v[0-9]+)?"));
const QRegularExpression KBibTeX::mendeleyFileRegExp(QStringLiteral(":(.*):pdf"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::domainNameRegExp(QStringLiteral("[a-z0-9.-]+\\.((a[cdefgilmnoqrstuwxz]|aero|arpa)|(b[abdefghijmnorstvwyz]|biz)|(c[acdfghiklmnorsuvxyz]|cat|com|coop)|d[ejkmoz]|(e[ceghrstu]|edu)|f[ijkmor]|(g[abdefghilmnpqrstuwy]|gov)|h[kmnrtu]|(i[delmnoqrst]|info|int)|(j[emop]|jobs)|k[eghimnprwyz]|l[abcikrstuvy]|(m[acdghklmnopqrstuvwxyz]|me|mil|mobi|museum)|(n[acefgilopruz]|name|net)|(om|org)|(p[aefghklmnrstwy]|pro)|qa|r[eouw]|s[abcdeghijklmnortvyz]|(t[cdfghjklmnoprtvwz]|travel)|u[agkmsyz]|v[aceginu]|w[fs]|y[etu]|z[amw])\\b"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::htmlRegExp(QStringLiteral("</?(a|pre|p|br|span|i|b|italic)\\b[^>{}]{,32}>"), QRegularExpression::CaseInsensitiveOption);
const QString KBibTeX::doiUrlPrefix(QStringLiteral("https://dx.doi.org/")); ///< for internal use in FileInfo::doiUrlPrefix, use FileInfo::doiUrlPrefix() instead

QDebug operator<<(QDebug dbg, const KBibTeX::TypeFlag &typeFlag)
{
    static const auto pairs = QHash<int, const char *> {
        {static_cast<int>(KBibTeX::TypeFlag::Invalid), "Invalid"},
        {static_cast<int>(KBibTeX::TypeFlag::PlainText), "PlainText"},
        {static_cast<int>(KBibTeX::TypeFlag::Reference), "Reference"},
        {static_cast<int>(KBibTeX::TypeFlag::Person), "Person"},
        {static_cast<int>(KBibTeX::TypeFlag::Keyword), "Keyword"},
        {static_cast<int>(KBibTeX::TypeFlag::Verbatim), "Verbatim"},
        {static_cast<int>(KBibTeX::TypeFlag::Source), "Source"}
    };
    dbg.nospace();
    const int typeFlagInt = static_cast<int>(typeFlag);
    dbg << (pairs.contains(typeFlagInt) ? pairs[typeFlagInt] : "???");
    return dbg;
}

QDebug operator<<(QDebug dbg, const KBibTeX::TypeFlags &typeFlags)
{
    static const auto pairs = QHash<const char *, KBibTeX::TypeFlag> {
        {"Invalid", KBibTeX::TypeFlag::Invalid},
        {"PlainText", KBibTeX::TypeFlag::PlainText},
        {"Reference", KBibTeX::TypeFlag::Reference},
        {"Person", KBibTeX::TypeFlag::Person},
        {"Keyword", KBibTeX::TypeFlag::Keyword},
        {"Verbatim", KBibTeX::TypeFlag::Verbatim},
        {"Source", KBibTeX::TypeFlag::Source}
    };
    dbg.nospace();
    bool first = true;
    for (QHash<const char *, KBibTeX::TypeFlag>::ConstIterator it = pairs.constBegin(); it != pairs.constEnd(); ++it) {
        if (typeFlags.testFlag(it.value())) {
            if (first) dbg << "|";
            dbg << it.key();
            first = false;
        }
    }
    return dbg;
}
