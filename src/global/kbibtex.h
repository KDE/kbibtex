/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GLOBAL_KBIBTEX_H
#define KBIBTEX_GLOBAL_KBIBTEX_H

#include <QMap>
#include <QUrl>

#ifdef HAVE_KF5
#include "kbibtexglobal_export.h"
#endif // HAVE_KF5

#define squeeze_text(text, n) ((text).length()<=(n)?(text):(text).left((n)/2-1)+QStringLiteral("...")+(text).right((n)/2-2))

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGLOBAL_EXPORT KBibTeX {
public:
    static const QString extensionTeX;
    static const QString extensionAux;
    static const QString extensionBBL;
    static const QString extensionBLG;
    static const QString extensionBibTeX;
    static const QString extensionPDF;
    static const QString extensionPostScript;
    static const QString extensionRTF;

    enum class Casing { LowerCase, InitialCapital, UpperCamelCase, LowerCamelCase, UpperCase };

    enum class FieldInputType { SingleLine, MultiLine, List, Url, Month, Color, PersonList, UrlList, KeywordList, CrossRef, StarRating, Edition };

    enum class TypeFlag {
        Invalid = 0x0,
        PlainText = 0x1,
        Reference = 0x2,
        Person = 0x4,
        Keyword = 0x8,
        Verbatim = 0x10,
        Source = 0x100
    };
    Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

    static const QString Months[];
    static const QString MonthsTriple[];

    static const QRegularExpression fileListSeparatorRegExp;
    static const QRegularExpression fileRegExp;
    static const QRegularExpression urlRegExp;
    static const QRegularExpression doiRegExp;
    static const QRegularExpression arXivRegExpWithPrefix;
    static const QRegularExpression arXivRegExpWithoutPrefix;
    static const QRegularExpression mendeleyFileRegExp;
    static const QRegularExpression domainNameRegExp;
    static const QRegularExpression htmlRegExp;
    static const QString doiUrlPrefix; ///< use FileInfo::doiUrlPrefix() instead
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KBibTeX::TypeFlags)

KBIBTEXGLOBAL_EXPORT QDebug operator<<(QDebug dbg, const KBibTeX::TypeFlag &typeFlag);
KBIBTEXGLOBAL_EXPORT QDebug operator<<(QDebug dbg, const KBibTeX::TypeFlags &typeFlags);

#endif // KBIBTEX_GLOBAL_KBIBTEX_H
