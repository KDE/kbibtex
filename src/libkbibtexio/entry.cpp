/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#include <QList>

#include <entry.h>
#include <file.h>

#define max(a,b) ((a)>(b)?(a):(b))

using namespace KBibTeX::IO;

// FIXME: Check if using those constants in the program is really necessary
// or can be replace by config files
const QLatin1String Entry::ftAbstract = QLatin1String("abstract");
const QLatin1String Entry::ftAddress = QLatin1String("address");
const QLatin1String Entry::ftAuthor = QLatin1String("author");
const QLatin1String Entry::ftBookTitle = QLatin1String("booktitle");
const QLatin1String Entry::ftChapter = QLatin1String("chapter");
const QLatin1String Entry::ftCrossRef = QLatin1String("crossref");
const QLatin1String Entry::ftDOI = QLatin1String("doi");
const QLatin1String Entry::ftEditor = QLatin1String("editor");
const QLatin1String Entry::ftISSN = QLatin1String("issn");
const QLatin1String Entry::ftISBN = QLatin1String("isbn");
const QLatin1String Entry::ftJournal = QLatin1String("journal");
const QLatin1String Entry::ftKeywords = QLatin1String("Keywords");
const QLatin1String Entry::ftLocation = QLatin1String("location");
const QLatin1String Entry::ftMonth = QLatin1String("month");
const QLatin1String Entry::ftNote = QLatin1String("note");
const QLatin1String Entry::ftNumber = QLatin1String("number");
const QLatin1String Entry::ftPages = QLatin1String("pages");
const QLatin1String Entry::ftPublisher = QLatin1String("publisher");
const QLatin1String Entry::ftSeries = QLatin1String("series");
const QLatin1String Entry::ftTitle = QLatin1String("title");
const QLatin1String Entry::ftUrl = QLatin1String("url");
const QLatin1String Entry::ftVolume = QLatin1String("volume");
const QLatin1String Entry::ftYear = QLatin1String("year");

const QLatin1String Entry::etArticle = QLatin1String("article");
const QLatin1String Entry::etBook = QLatin1String("book");
const QLatin1String Entry::etInBook = QLatin1String("inbook");
const QLatin1String Entry::etInProceedings = QLatin1String("inproceedings");
const QLatin1String Entry::etMisc = QLatin1String("misc");
const QLatin1String Entry::etPhDThesis = QLatin1String("phdthesis");
const QLatin1String Entry::etTechReport = QLatin1String("techreport");

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Entry::EntryPrivate
{
public:
    QString type;
    QString id;
};

Entry::Entry(const QString& type, const QString& id)
        : Element(), QMap<QString, Value>(), d(new Entry::EntryPrivate)
{
    d->type = type;
    d->id = id;
}

Entry::Entry(const Entry &other)
        : Element(), QMap<QString, Value>(), d(new Entry::EntryPrivate)
{
    operator=(other);
}

Entry::~Entry()
{
    // nothing
}

Entry& Entry::operator= (const Entry & other)
{
    if (this != &other) {
        d->type = other.type();
        d->id = other.id();
        clear();
        for (QMap<QString, Value>::ConstIterator it = other.begin(); it != other.end(); ++it)
            insert(it.key(), it.value());
    }
    return *this;
}

void Entry::setType(const QString& type)
{
    d->type = type;
}

QString Entry::type() const
{
    return d->type;
}

void Entry::setId(const QString& id)
{
    d->id = id;
}

QString Entry::id() const
{
    return d->id;
}

const Value Entry::value(const QString& key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return it.value();

    return Value();
}

bool Entry::contains(const QString& key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return true;

    return false;
}


