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
#include <QStringList>

#include "preamble.h"

using namespace KBibTeX::IO;

Preamble::Preamble()
        : Element()
{
// nothing
}

Preamble::Preamble(const Preamble& other)
        : Element()
{
    m_value = other.m_value;
}

Value& Preamble::value()
{
    return m_value;
}

const Value& Preamble::value() const
{
    return m_value;
}

void Preamble::setValue(const Value& value)
{
    m_value = value;

}

/*
bool Preamble::containsPattern(const QString& pattern, Field::FieldType fieldType, FilterType filterType, Qt::CaseSensitivity caseSensitive) const
{
    if (fieldType != Field::ftUnknown)
        return false;

    if (filterType == ftExact) {
        ** check for exact match *
        return m_value.containsPattern(pattern, caseSensitive);
    } else {
        ** for each word in the search pattern ... *
        QStringList words = pattern.split(QRegExp("\\s+"));
        int hits = 0;
        for (QStringList::Iterator it = words.begin(); it != words.end(); ++it) {
            ** check if word is contained in text *
            if (m_value.containsPattern(*it, caseSensitive))
                ++hits;
        }

        ** return success depending on filter type and number of hits *
        return ((filterType == ftAnyWord && hits > 0) || (filterType == ftEveryWord && hits == words.count()));
    }
}
*/
