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
public:
    explicit Preamble(const Value &value = Value());
    Preamble(const Preamble &other);
    ~Preamble() override;

    /**
     * Assignment operator, working similar to a copy constructor,
     * but overwrites the current object's values.
     */
    Preamble &operator= (const Preamble &other);

    Value &value();
    const Value &value() const;
    void setValue(const Value &value);

    // bool containsPattern(const QString& pattern, Field::FieldType fieldType = Field::ftUnknown, FilterType filterType = Element::ftExact, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const; // FIXME: Rewrite filtering code

    /**
     * Cheap and fast test if another Element is a Preamble object.
     * @param other another Element object to test
     * @return true if Element is actually a Preamble
     */
    static bool isPreamble(const Element &other);

private:
    class PreamblePrivate;
    PreamblePrivate *const d;
};

QDebug operator<<(QDebug dbg, const Preamble &preamble);

#endif // KBIBTEX_IO_PREAMBLE_H
