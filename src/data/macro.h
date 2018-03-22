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

#ifndef KBIBTEX_IO_MACRO_H
#define KBIBTEX_IO_MACRO_H

#include "element.h"
#include "value.h"

class QString;

/**
 * This class represents a macro in a BibTeX file. Macros in BibTeX
 * are similar to variables, allowing to use the same value such as
 * journal titles in several entries.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT Macro : public Element
{
public:
    /**
     * Create a new macro with a given key-value pair.
     * @param key macro's key
     * @param value macro's value
     */
    explicit Macro(const QString &key = QString(), const Value &value = Value());

    /**
     * Copy constructor cloning another macro object.
     * @param other macro object to clone
     */
    Macro(const Macro &other);

    ~Macro() override;

    /**
     * Assignment operator, working similar to a copy constructor,
     * but overwrites the current object's values.
     */
    Macro &operator= (const Macro &other);

    /**
     * Set the key of this macro.
     * @param key new key of this macro
     */
    void setKey(const QString &key);

    /**
     * Retrieve the key of this macro.
     * @return key of this macro
     */
    QString key() const;

    /**
     * Retrieve the key of this macro. Returns a reference which may not be modified.
     * @return key of this macro
     */
    const Value &value() const;

    /**
     * Retrieve the key of this macro. Returns a reference which may be modified.
     * @return key of this macro
     */
    Value &value();

    /**
     * Set the value of this macro.
     * @param value new value of this macro
     */
    void setValue(const Value &value);

    /**
     * Cheap and fast test if another Element is a Macro object.
     * @param other another Element object to test
     * @return true if Element is actually a Macro
     */
    static bool isMacro(const Element &other);

private:
    class MacroPrivate;
    MacroPrivate *const d;
};

QDebug operator<<(QDebug dbg, const Macro &macro);

#endif
