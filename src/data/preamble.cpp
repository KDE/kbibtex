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

#include "preamble.h"

#include <typeinfo>

#include <QDebug>
#include <QStringList>

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Preamble::PreamblePrivate
{
public:
    Value value;
};

Preamble::Preamble(const Value &value)
        : Element(), d(new Preamble::PreamblePrivate)
{
    d->value = value;
}

Preamble::Preamble(const Preamble &other)
        : Element(), d(new Preamble::PreamblePrivate)
{
    operator=(other);
}

Preamble::~Preamble()
{
    delete d;
}

bool Preamble::operator==(const Preamble &other) const
{
    return d->value == other.d->value;
}

bool Preamble::operator!=(const Preamble &other) const
{
    return !operator ==(other);
}

Preamble &Preamble::operator= (const Preamble &other)
{
    if (this != &other)
        d->value = other.d->value;
    return *this;
}

Value &Preamble::value()
{
    return d->value;
}

const Value &Preamble::value() const
{
    return d->value;
}

void Preamble::setValue(const Value &value)
{
    d->value = value;

}

bool Preamble::isPreamble(const Element &other) {
    return typeid(other) == typeid(Preamble);
}

QDebug operator<<(QDebug dbg, const Preamble &preamble) {
    dbg.nospace() << "Preamble: " << preamble.value();
    return dbg;
}
