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
#ifndef KBIBTEX_NAMESPACE_H
#define KBIBTEX_NAMESPACE_H

#include <QVariant>

#include <KUrl>

namespace KBibTeX
{

enum Casing {
    cLowerCase = 0,
    cInitialCapital = 1,
    cUpperCamelCase = 2,
    cLowerCamelCase = 3,
    cUpperCase = 4
};

enum FieldInputType {
    SingleLine = 1,
    MultiLine = 2,
    List = 3,
    URL = 4,
    Month = 5,
    Color = 6,
    PersonList = 7,
    UrlList = 8,
    KeywordList = 9,
    CrossRef = 10
};

enum TypeFlag {
    tfPlainText = 0x1,
    tfReference = 0x2,
    tfPerson = 0x4,
    tfKeyword = 0x8,
    tfVerbatim = 0x10,
    tfSource = 0x100
};
Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(TypeFlags)

static const QString MonthsTriple[] = {
    QLatin1String("jan"), QLatin1String("feb"), QLatin1String("mar"), QLatin1String("apr"), QLatin1String("may"), QLatin1String("jun"), QLatin1String("jul"), QLatin1String("aug"), QLatin1String("sep"), QLatin1String("oct"), QLatin1String("nov"), QLatin1String("dec")
};

static const QRegExp fileListSeparatorRegExp("[ \\t]*[;\\n]+[ \\t]*");
static const QRegExp fileRegExp("(\\bfile:)?[^{}\\t]+\\.\\w{2,4}\\b", Qt::CaseInsensitive);
static const QRegExp urlRegExp("\\b(http|s?ftp|webdav|file)s?://[^ {}\"]+\\b", Qt::CaseInsensitive);
static const QRegExp doiRegExp("\\b10\\.\\d{4}([.][0-9]+)*/[/-a-z0-9.()<>_:;\\\\]+", Qt::CaseInsensitive);
static const QString doiUrlPrefix = QLatin1String("http://dx.doi.org/"); ///< use FileInfo::doiUrlPrefix() instead
static const QRegExp domainNameRegExp("[a-z0-9.-]+\\.((a[cdefgilmnoqrstuwxz]|aero|arpa)|(b[abdefghijmnorstvwyz]|biz)|(c[acdfghiklmnorsuvxyz]|cat|com|coop)|d[ejkmoz]|(e[ceghrstu]|edu)|f[ijkmor]|(g[abdefghilmnpqrstuwy]|gov)|h[kmnrtu]|(i[delmnoqrst]|info|int)|(j[emop]|jobs)|k[eghimnprwyz]|l[abcikrstuvy]|(m[acdghklmnopqrstuvwxyz]|me|mil|mobi|museum)|(n[acefgilopruz]|name|net)|(om|org)|(p[aefghklmnrstwy]|pro)|qa|r[eouw]|s[abcdeghijklmnortvyz]|(t[cdfghjklmnoprtvwz]|travel)|u[agkmsyz]|v[aceginu]|w[fs]|y[etu]|z[amw])\\b", Qt::CaseInsensitive);

inline static bool isLocalOrRelative(const KUrl &url)
{
    return url.isLocalFile() || url.isRelative() || url.protocol().isEmpty();
}

inline static bool isLocalOrRelative(KUrl *url)
{
    return url->isLocalFile() || url->isRelative() || url->protocol().isEmpty();
}

}

/**
 * Poor man's variant of a text-squeezing function.
 * Effect is similar as observed in KSqueezedTextLabel:
 * If the text is longer as n characters, the middle part
 * will be cut away and replaced by "..." to get a
 * string of max n characters.
 */
inline static QString squeezeText(const QString &text, int n)
{
    return text.length() <= n ? text : text.left(n / 2 - 1) + QLatin1String("...") + text.right(n / 2 - 2);
}

#define squeeze_text(text, n) ((text).length()<=(n)?(text):(text).left((n)/2-1)+QLatin1String("...")+(text).right((n)/2-2))

#endif // KBIBTEX_NAMESPACE_H
