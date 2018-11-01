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

#include "macro.h"

#include <typeinfo>

#include <QDebug>
#include <QStringList>

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Macro::MacroPrivate
{
public:
    QString key;
    Value value;
};

Macro::Macro(const QString &key, const Value &value)
        : Element(), d(new Macro::MacroPrivate)
{
    d->key = key;
    d->value = value;
}

Macro::Macro(const Macro &other)
        : Element(), d(new Macro::MacroPrivate)
{
    d->key = other.d->key;
    d->value = other.d->value;
}

Macro::~Macro()
{
    delete d;
}

bool Macro::operator==(const Macro &other) const
{
    return d->key == other.d->key && d->value == other.d->value;
}

bool Macro::operator!=(const Macro &other) const
{
    return !operator ==(other);
}

Macro &Macro::operator= (const Macro &other)
{
    if (this != &other) {
        d->key = other.key();
        d->value = other.value();
    }
    return *this;
}

void Macro::setKey(const QString &key)
{
    d->key = key;
}

QString Macro::key() const
{
    return d->key;
}

Value &Macro::value()
{
    return d->value;
}

const Value &Macro::value() const
{
    return d->value;
}

void Macro::setValue(const Value &value)
{
    d->value = value;
}

bool Macro::isMacro(const Element &other) {
    return typeid(other) == typeid(Macro);
}

QDebug operator<<(QDebug dbg, const Macro &macro) {
    dbg.nospace() << "Macro " << macro.key() << " = " << macro.value();
    return dbg;
}
