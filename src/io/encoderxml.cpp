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

#include "encoderxml.h"

#include <QStringList>
#include <QList>

static const struct EncoderXMLCharMapping {
    QChar unicode;
    const QString xml;
}
charmappingdataxml[] = {
    {QChar(0x0026), QStringLiteral("&amp;")},
    {QChar(0x0022), QStringLiteral("&quot;")},
    {QChar(0x003C), QStringLiteral("&lt;")},
    {QChar(0x003E), QStringLiteral("&gt;")}
};

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class EncoderXML::EncoderXMLPrivate
{
public:
    static const QStringList backslashSymbols;
};

const QStringList EncoderXML::EncoderXMLPrivate::backslashSymbols {QStringLiteral("\\&"), QStringLiteral("\\%"), QStringLiteral("\\_")};

EncoderXML::EncoderXML()
        : Encoder(), d(new EncoderXML::EncoderXMLPrivate)
{
    /// nothing
}

EncoderXML::~EncoderXML()
{
    delete d;
}

QString EncoderXML::decode(const QString &text) const
{
    QString result = text;

    for (const auto &item : charmappingdataxml)
        result.replace(item.xml, item.unicode);

    /**
     * Find and replace all characters written as hexadecimal number
     */
    int p = -1;
    while ((p = result.indexOf(QStringLiteral("&#x"), p + 1)) >= 0) {
        int p2 = result.indexOf(QStringLiteral(";"), p + 1);
        if (p2 < 0 || p2 > p + 8) break;
        bool ok = false;
        int hex = result.midRef(p + 3, p2 - p - 3).toInt(&ok, 16);
        if (ok && hex > 0)
            result.replace(result.mid(p, p2 - p + 1), QChar(hex));
    }

    /**
      * Find and replace all characters written as decimal number
      */
    p = -1;
    while ((p = result.indexOf(QStringLiteral("&#"), p + 1)) >= 0) {
        int p2 = result.indexOf(QStringLiteral(";"), p + 1);
        if (p2 < 0 || p2 > p + 8) break;
        bool ok = false;
        int dec = result.midRef(p + 2, p2 - p - 2).toInt(&ok, 10);
        if (ok && dec > 0)
            result.replace(result.mid(p, p2 - p + 1), QChar(dec));
    }

    /// Replace special symbols with backslash-encoded variant (& --> \&)
    for (const QString &backslashSymbol : EncoderXMLPrivate::backslashSymbols) {
        int p = -1;
        while ((p = result.indexOf(backslashSymbol[1], p + 1)) >= 0) {
            if (p == 0 || result[p - 1] != QLatin1Char('\\')) {
                /// replace only symbols which have no backslash on their right
                result = result.left(p) + QLatin1Char('\\') + result.mid(p);
                ++p;
            }
        }
    }

    return result;
}

QString EncoderXML::encode(const QString &text, const TargetEncoding targetEncoding) const
{
    QString result = text;

    for (const auto &item : charmappingdataxml)
        result.replace(item.unicode, item.xml);

    if (targetEncoding == TargetEncodingASCII) {
        /// Replace all non-ASCII characters (code >=128) with an entity code,
        /// for example a-umlaut becomes '&#228;'.
        for (int i = result.length() - 1; i >= 0; --i) {
            const auto code = result[i].unicode();
            if (code > 127)
                result = result.left(i) + QStringLiteral("&#") + QString::number(code) + QStringLiteral(";") + result.mid(i + 1);
        }
    }

    /// Replace backlash-encoded symbols with plain text (\& --> &)
    for (const QString &backslashSymbol : EncoderXMLPrivate::backslashSymbols) {
        result.replace(backslashSymbol, backslashSymbol[1]);
    }

    return result;
}

const EncoderXML &EncoderXML::instance()
{
    static const EncoderXML self;
    return self;
}
