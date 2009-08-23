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
#ifndef ENCODERLATEX_H
#define ENCODERLATEX_H

#include <QLinkedList>

#include <encoder.h>
#include <entry.h>

class QString;
class QRegExp;

namespace KBibTeX
{
namespace IO {

class EncoderLaTeX: public Encoder
{
public:
    EncoderLaTeX();
    ~EncoderLaTeX();

    QString decode(const QString &text);
    QString encode(const QString &text);
    QString encode(const QString &text, const QChar &replace);
    QString encodeSpecialized(const QString &text, const Field::FieldType fieldType = Field::ftUnknown);
    QString& decomposedUTF8toLaTeX(QString &text);

    static EncoderLaTeX *currentEncoderLaTeX();
    static void deleteCurrentEncoderLaTeX();

private:
    struct CombinedMappingItem {
        QRegExp regExp;
        QString latex;
    };

    QLinkedList<CombinedMappingItem> m_combinedMapping;

    void buildCombinedMapping();

    struct CharMappingItem {
        QRegExp regExp;
        QString unicode;
        QString latex;
    };

    QLinkedList<CharMappingItem> m_charMapping;

    void buildCharMapping();
};

}
}

#endif // ENCODERLATEX_H
