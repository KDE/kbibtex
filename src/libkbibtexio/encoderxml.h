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
#ifndef KBIBTEX_IO_ENCODERXML_H
#define KBIBTEX_IO_ENCODERXML_H

#include <encoder.h>

class QString;
class QRegExp;

namespace KBibTeX
{
namespace IO {

/**
@author Thomas Fischer
*/
class EncoderXML : public Encoder
{
public:
    EncoderXML();
    ~EncoderXML();

    QString decode(const QString &text);
    QString encode(const QString &text);
    QString encodeSpecialized(const QString &text, const EntryField::FieldType fieldType = EntryField::ftUnknown);

    static EncoderXML *currentEncoderXML();

private:
    struct CharMappingItem {
        QRegExp regExp;
        QChar unicode;
        QString latex;
    };

    QLinkedList<CharMappingItem> m_charMapping;

    void buildCharMapping();
};

}
}

#endif // KBIBTEX_IO_ENCODERXML_H
