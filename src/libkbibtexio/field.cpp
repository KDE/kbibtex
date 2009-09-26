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
#include "field.h"

using namespace KBibTeX::IO;

// FIXME: Check if using those constants in the program is really necessary
// or can be replace by config files
const QLatin1String Field::ftAbstract = QLatin1String("abstract");
const QLatin1String Field::ftAddress = QLatin1String("address");
const QLatin1String Field::ftAuthor = QLatin1String("author");
const QLatin1String Field::ftBookTitle = QLatin1String("booktitle");
const QLatin1String Field::ftChapter = QLatin1String("chapter");
const QLatin1String Field::ftCrossRef = QLatin1String("crossref");
const QLatin1String Field::ftDOI = QLatin1String("doi");
const QLatin1String Field::ftEditor = QLatin1String("editor");
const QLatin1String Field::ftISSN = QLatin1String("issn");
const QLatin1String Field::ftISBN = QLatin1String("isbn");
const QLatin1String Field::ftJournal = QLatin1String("journal");
const QLatin1String Field::ftKeywords = QLatin1String("Keywords");
const QLatin1String Field::ftLocation = QLatin1String("location");
const QLatin1String Field::ftMonth = QLatin1String("month");
const QLatin1String Field::ftNote = QLatin1String("note");
const QLatin1String Field::ftNumber = QLatin1String("number");
const QLatin1String Field::ftPages = QLatin1String("pages");
const QLatin1String Field::ftPublisher = QLatin1String("publisher");
const QLatin1String Field::ftSeries = QLatin1String("series");
const QLatin1String Field::ftTitle = QLatin1String("title");
const QLatin1String Field::ftUrl = QLatin1String("url");
const QLatin1String Field::ftVolume = QLatin1String("volume");
const QLatin1String Field::ftYear = QLatin1String("year");

class Field::FieldPrivate
{
public:
    QString m_key;
    Value m_value;
};

Field::Field(const QString& key, const Value& value)
        : d(new Field::FieldPrivate())
{
    setKey(key);
    setValue(value);
}

Field::Field(const Field &other)
        : d(new Field::FieldPrivate())
{
    setKey(other.key());
    setValue(other.value());
}

Field::~Field()
{
    delete d;
}

QString Field::key() const
{
    return d->m_key;
}

void Field::setKey(const QString& key)
{
    d->m_key = key.toLower();
}

Value& Field::value()
{
    return d->m_value;
}

const Value& Field::value() const
{
    return d->m_value;
}

void Field::setValue(const Value& value)
{
    d->m_value = value;
}
