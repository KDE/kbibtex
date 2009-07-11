/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <QRegExp>

#include "encoderxml.h"

using namespace KBibTeX::IO;

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

EncoderXML::EncoderXML()
        : Encoder()
{
    buildCharMapping();
}

EncoderXML::~EncoderXML()
{
    // nothing
}

QString EncoderXML::decode(const QString &text)
{
    QString result = text;

    for (QLinkedList<CharMappingItem>::ConstIterator it = m_charMapping.begin(); it != m_charMapping.end(); ++it)
        result.replace((*it).regExp, (*it).unicode);

    /**
      * Find and replace all characters written as hexadecimal number
      */
    int p = -1;
    while ((p = result.indexOf("&#x", p + 1)) >= 0) {
        int p2 = result.indexOf(";", p + 1);
        if (p2 < 0) break;
        bool ok = FALSE;
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
        bool ok = FALSE;
        int dec = result.mid(p + 2, p2 - p - 2).toInt(&ok, 10);
        if (ok && dec > 0)
            result.replace(result.mid(p, p2 - p + 1), QChar(dec));
    }

    return result;
}

QString EncoderXML::encode(const QString &text)
{
    QString result = text;

    for (QLinkedList<CharMappingItem>::ConstIterator it = m_charMapping.begin(); it != m_charMapping.end(); ++it)
        result.replace((*it).unicode, (*it).latex);

    return result;
}

QString EncoderXML::encodeSpecialized(const QString &text, const EntryField::FieldType  /* fieldType */)
{
    return encode(text);
}

void EncoderXML::buildCharMapping()
{
    for (int i = 0; i < charmappingdataxmlcount; i++) {
        CharMappingItem charMappingItem;
        charMappingItem.regExp = QRegExp(charmappingdataxml[ i ].regexp);
        charMappingItem.unicode = QChar(charmappingdataxml[ i ].unicode);
        charMappingItem.latex = QString(charmappingdataxml[ i ].latex);
        m_charMapping.append(charMappingItem);
    }
}

EncoderXML *EncoderXML::currentEncoderXML()
{
    if (encoderXML == NULL)
        encoderXML = new EncoderXML();

    return encoderXML;
}
