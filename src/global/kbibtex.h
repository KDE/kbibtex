/***************************************************************************
 *   Copyright (C) 2004-2016 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#define squeeze_text(text, n) ((text).length()<=(n)?(text):(text).left((n)/2-1)+QStringLiteral("...")+(text).right((n)/2-2))

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBibTeX {
public:
    static const QString extensionTeX;
    static const QString extensionAux;
    static const QString extensionBBL;
    static const QString extensionBLG;
    static const QString extensionBibTeX;
    static const QString extensionPDF;
    static const QString extensionPostScript;
    static const QString extensionRTF;

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
        CrossRef = 10,
        StarRating = 11,
        Edition = 12
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

    static const QString Months[], MonthsTriple[];

    static const QRegularExpression fileListSeparatorRegExp;
    static const QRegularExpression fileRegExp;
    static const QRegularExpression urlRegExp;
    static const QRegularExpression doiRegExp;
    static const QRegularExpression mendeleyFileRegExp;
    static const QRegularExpression domainNameRegExp;
    static const QRegularExpression htmlRegExp;
    static const QString doiUrlPrefix; ///< use FileInfo::doiUrlPrefix() instead

    static bool isLocalOrRelative(const QUrl &url);

    /**
     * Poor man's variant of a text-squeezing function.
     * Effect is similar as observed in KSqueezedTextLabel:
     * If the text is longer as n characters, the middle part
     * will be cut away and replaced by "..." to get a
     * string of max n characters.
     */
    static QString squeezeText(const QString &text, int n);

    static QString leftSqueezeText(const QString &text, int n);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(KBibTeX::TypeFlags)

#endif // KBIBTEX_GLOBAL_KBIBTEX_H
