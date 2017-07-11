/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "encoderxml.h"

#include <QStringList>
#include <QRegExp>
#include <QList>

EncoderXML *encoderXML = NULL;

static const struct EncoderXMLCharMapping {
    const char *regexp;
    unsigned int unicode;
    const char *latex;
}
charmappingdataxml[] = {
    {"&quot;", 0x0022, "&quot;"}, /** FIXME: is this one required? */
    {"&amp;", 0x0026, "&amp;"},
    {"&lt;", 0x003C, "&lt;"},
    {"&gt;", 0x003E, "&gt;"}
};
static const int charmappingdataxmlcount = sizeof(charmappingdataxml) / sizeof(charmappingdataxml[ 0 ]) ;

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class EncoderXML::EncoderXMLPrivate
{
public:
    static const QStringList backslashSymbols;

    struct CharMappingItem {
        QRegExp regExp;
        QChar unicode;
        QString xml;
    };

    QList<CharMappingItem> charMapping;

    void buildCharMapping() {
        for (int i = 0; i < charmappingdataxmlcount; i++) {
            CharMappingItem charMappingItem;
            charMappingItem.regExp = QRegExp(charmappingdataxml[ i ].regexp);
            charMappingItem.unicode = QChar(charmappingdataxml[ i ].unicode);
            charMappingItem.xml = QString(charmappingdataxml[ i ].latex);
            charMapping.append(charMappingItem);
        }
    }

};

const QStringList EncoderXML::EncoderXMLPrivate::backslashSymbols = QStringList() << QLatin1String("\\&") << QLatin1String("\\%") << QLatin1String("\\_");


EncoderXML::EncoderXML()
        : Encoder(), d(new EncoderXML::EncoderXMLPrivate)
{
    d->buildCharMapping();
}

EncoderXML::~EncoderXML()
{
    delete d;
}

QString EncoderXML::decode(const QString &text) const
{
    QString result = text;

    for (QList<EncoderXMLPrivate::CharMappingItem>::ConstIterator it = d->charMapping.constBegin(); it != d->charMapping.constEnd(); ++it)
        result.replace((*it).regExp, (*it).unicode);

    /**
     * Find and replace all characters written as hexadecimal number
     */
    int p = -1;
    while ((p = result.indexOf("&#x", p + 1)) >= 0) {
        int p2 = result.indexOf(";", p + 1);
        if (p2 < 0) break;
        bool ok = false;
        int hex = result.mid(p + 3, p2 - p - 3).toInt(&ok, 16);
        if (ok && hex > 0)
            result.replace(result.mid(p, p2 - p + 1), QChar(hex));
    }

    /**
      * Find and replace all characters written as decimal number
      */
    p = -1;
    while ((p = result.indexOf("&#", p + 1)) >= 0) {
        int p2 = result.indexOf(";", p + 1);
        if (p2 < 0) break;
        bool ok = false;
        int dec = result.mid(p + 2, p2 - p - 2).toInt(&ok, 10);
        if (ok && dec > 0)
            result.replace(result.mid(p, p2 - p + 1), QChar(dec));
    }

    /// Replace special symbols with backslash-encoded variant (& --> \&)
    foreach (const QString &backslashSymbol, EncoderXMLPrivate::backslashSymbols) {
        int p = -1;
        while ((p = result.indexOf(backslashSymbol[1], p + 1)) >= 0) {
            if (p == 0 || result[p - 1] != QChar('\\')) {
                /// replace only symbols which have no backslash on their right
                result = result.left(p) + QChar('\\') + result.mid(p);
                ++p;
            }
        }
    }

    return result;
}

QString EncoderXML::encode(const QString &text) const
{
    QString result = text;

    for (QList<EncoderXMLPrivate::CharMappingItem>::ConstIterator it = d->charMapping.constBegin(); it != d->charMapping.constEnd(); ++it)
        result.replace((*it).unicode, (*it).xml);

    /// Replace backlash-encoded symbols with plain text (\& --> &)
    foreach (const QString &backslashSymbol, EncoderXMLPrivate::backslashSymbols) {
        result.replace(backslashSymbol, backslashSymbol[1]);
    }

    return result;
}

EncoderXML *EncoderXML::currentEncoderXML()
{
    if (encoderXML == NULL)
        encoderXML = new EncoderXML();

    return encoderXML;
}
