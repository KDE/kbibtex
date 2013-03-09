/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer                             *
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
#ifndef KBIBTEX_IO_PREAMBLE_H
#define KBIBTEX_IO_PREAMBLE_H

#include "element.h"
#include "value.h"

/**
 * This class represents a preamble in a BibTeX file. Preables contain
 * LaTeX commands required for the bibliography, such as hyphenation commands.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT Preamble : public Element
{
    Q_PROPERTY(Value value READ value WRITE setValue)

public:
    Preamble(const Value &value = Value());
    Preamble(const Preamble &other);
    ~Preamble();

    /**
     * Assignment operator, working similar to a copy constructor,
     * but overwrites the current object's values.
     */
    Preamble &operator= (const Preamble &other);

    Value &value();
    const Value &value() const;
    void setValue(const Value &value);

    // bool containsPattern(const QString& pattern, Field::FieldType fieldType = Field::ftUnknown, FilterType filterType = Element::ftExact, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const; // FIXME: Rewrite filtering code

private:
    class PreamblePrivate;
    PreamblePrivate *const d;
};

#endif // KBIBTEX_IO_PREAMBLE_H
