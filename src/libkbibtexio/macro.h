/***************************************************************************
*   Copyright (C) 2004-2006 by Thomas Fischer                             *
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
#ifndef BIBTEXMACRO_H
#define BIBTEXMACRO_H

#include <element.h>
#include <entryfield.h>
#include <value.h>

class QString;

namespace KBibTeX
{
namespace IO {

class KBIBTEXIO_EXPORT Macro : public Element
{
public:
    Macro(const QString &key);
    Macro(const Macro *other);
    virtual ~Macro();

    void setKey(const QString &key);
    QString key() const;

    Value *value() const;
    void setValue(Value *value);

    bool containsPattern(const QString& pattern, EntryField::FieldType fieldType = EntryField::ftUnknown, FilterType filterType = Element::ftExact, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

    Element* clone() const;
    void copyFrom(const Macro *other);
    QString text() const;

private:
    QString m_key;
    Value *m_value;
};

}
}

#endif
