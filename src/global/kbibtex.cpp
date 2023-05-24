/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
const QRegularExpression KBibTeX::doiRegExp(QStringLiteral("(?<doi>10([.][0-9]+)+/[-/a-z0-9.()<>_:;\\\\]+)"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::arXivRegExp(QStringLiteral("\\b(?<arxiv>((astro-ph.CO|astro-ph.EP|astro-ph.GA|astro-ph.HE|astro-ph.IM|astro-ph|astro-ph.SR|cond-mat.dis-nn|cond-mat.mes-hall|cond-mat.mtrl-sci|cond-mat.other|cond-mat.quant-gas|cond-mat|cond-mat.soft|cond-mat.stat-mech|cond-mat.str-el|cond-mat.supr-con|cs.AI|cs.AR|cs.CC|cs.CE|cs.CG|cs.CL|cs.CR|cs.CV|cs.CY|cs.DB|cs.DC|cs.DL|cs.DM|cs.DS|cs.ET|cs.FL|cs.GL|cs.GR|cs.GT|cs.HC|cs.IR|cs.IT|cs.LG|cs.LO|cs.MA|cs.MM|cs.MS|cs.NA|cs.NE|cs.NI|cs.OH|cs.OS|cs.PF|cs.PL|cs|cs.RO|cs.SC|cs.SD|cs.SE|cs.SI|cs.SY|econ.EM|econ.GN|econ|econ.TH|eess.AS|eess.IV|eess|eess.SP|eess.SY|gr-qc|hep-ex|hep-lat|hep-ph|hep-th|math.AC|math.AG|math.AP|math.AT|math.CA|math.CO|math.CT|math.CV|math.DG|math.DS|math.FA|math.GM|math.GN|math.GR|math.GT|math.HO|math.IT|math.KT|math.LO|math.MG|math.MP|math.NA|math.NT|math.OA|math.OC|math-ph|math.PR|math.QA|math.RA|math|math.RT|math.SG|math.SP|math.ST|nlin.AO|nlin.CD|nlin.CG|nlin.PS|nlin|nlin.SI|nucl-ex|nucl-th|physics.acc-ph|physics.ao-ph|physics.app-ph|physics.atm-clus|physics.atom-ph|physics.bio-ph|physics.chem-ph|physics.class-ph|physics.comp-ph|physics.data-an|physics.ed-ph|physics.flu-dyn|physics.gen-ph|physics.geo-ph|physics.hist-ph|physics.ins-det|physics.med-ph|physics.optics|physics.plasm-ph|physics.pop-ph|physics|physics.soc-ph|physics.space-ph|q-bio.BM|q-bio.CB|q-bio.GN|q-bio.MN|q-bio.NC|q-bio.OT|q-bio.PE|q-bio.QM|q-bio|q-bio.SC|q-bio.TO|q-fin.CP|q-fin.EC|q-fin.GN|q-fin.MF|q-fin.PM|q-fin.PR|q-fin|q-fin.RM|q-fin.ST|q-fin.TR|quant-ph|stat.AP|stat.CO|stat.ME|stat.ML|stat.OT|stat|stat.TH|[0-9]{2}(0[1-9]|1[0-2]))/[0-9]{4,7})(v[1-9][0-9]*)?)"));
const QRegularExpression KBibTeX::domainNameRegExp(QStringLiteral("[a-z0-9.-]+\\.((a[cdefgilmnoqrstuwxz]|aero|arpa)|(b[abdefghijmnorstvwyz]|biz)|(c[acdfghiklmnorsuvxyz]|cat|com|coop)|d[ejkmoz]|(e[ceghrstu]|edu)|f[ijkmor]|(g[abdefghilmnpqrstuwy]|gov)|h[kmnrtu]|(i[delmnoqrst]|info|int)|(j[emop]|jobs)|k[eghimnprwyz]|l[abcikrstuvy]|(m[acdghklmnopqrstuvwxyz]|me|mil|mobi|museum)|(n[acefgilopruz]|name|net)|(om|org)|(p[aefghklmnrstwy]|pro)|qa|r[eouw]|s[abcdeghijklmnortvyz]|(t[cdfghjklmnoprtvwz]|travel)|u[agkmsyz]|v[aceginu]|w[fs]|y[etu]|z[amw])\\b"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression KBibTeX::htmlRegExp(QStringLiteral("</?(a|pre|p|br|span|i|b|italic)\\b[^>{}]{,32}>"), QRegularExpression::CaseInsensitiveOption);
const QString KBibTeX::doiUrlPrefix(QStringLiteral("https://doi.org/")); ///< for internal use in FileInfo::doiUrlPrefix, use FileInfo::doiUrlPrefix() instead

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
