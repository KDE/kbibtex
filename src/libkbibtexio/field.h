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
#ifndef BIBTEXBIBTEXENTRYFIELD_H
#define BIBTEXBIBTEXENTRYFIELD_H

#include <QString>

#include <value.h>

#include "kbibtexio_export.h"

namespace KBibTeX
{
namespace IO {

/**
  Representation for a key-value combination in a bibliography file.
  An example for a key-value pair is a field where the key is "author" and the value is a list of names.
  By default, keys and values follow the BibTeX style, i.e. importer libraries have to translate other bibliographies' keys and values.
  For example, a RIS importer would translate several "AU" key-value pairs into one Field object with key "author" and a value holding a list of person names.

  @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
  */
class KBIBTEXIO_EXPORT Field
{
public:
    /** Create a new Field object. */
    Field(const QString& key, const Value& value = Value());

    /** Copy an existing Field object */
    Field(const Field& other);

    ~Field();

    /** Get the object's key. The key will be always in lower case letters. */
    QString key() const;

    /** Set the object's key. The key will be automatically converted to lower case. */
    void setKey(const QString& key);

    /** Return a reference to this object's value, which can be modified from the outside. */
    Value& value();

    /** Return a const reference to this object's value, which cannot be modified from the outside. Use this function only if this object is const. */
    const Value& value() const;

    /** Set this object's value. Internally, a copy of the passed Value object will be created. */
    void setValue(const Value& value);

    /** Representation of the BibTeX field key "abstract" */
    static const QLatin1String ftAbstract;
    /** Representation of the BibTeX field key "address" */
    static const QLatin1String ftAddress;
    /** Representation of the BibTeX field key "author" */
    static const QLatin1String ftAuthor;
    /** Representation of the BibTeX field key "booktitle" */
    static const QLatin1String ftBookTitle;
    /** Representation of the BibTeX field key "chapter" */
    static const QLatin1String ftChapter;
    /** Representation of the BibTeX field key "crossref" */
    static const QLatin1String ftCrossRef;
    /** Representation of the BibTeX field key "doi" */
    static const QLatin1String ftDOI;
    /** Representation of the BibTeX field key "editor" */
    static const QLatin1String ftEditor;
    /** Representation of the BibTeX field key "issn" */
    static const QLatin1String ftISSN;
    /** Representation of the BibTeX field key "isbn" */
    static const QLatin1String ftISBN;
    /** Representation of the BibTeX field key "journal" */
    static const QLatin1String ftJournal;
    /** Representation of the BibTeX field key "keywords" */
    static const QLatin1String ftKeywords;
    /** Representation of the BibTeX field key "location" */
    static const QLatin1String ftLocation;
    /** Representation of the BibTeX field key "month" */
    static const QLatin1String ftMonth;
    /** Representation of the BibTeX field key "note" */
    static const QLatin1String ftNote;
    /** Representation of the BibTeX field key "number" */
    static const QLatin1String ftNumber;
    /** Representation of the BibTeX field key "pages" */
    static const QLatin1String ftPages;
    /** Representation of the BibTeX field key "publisher" */
    static const QLatin1String ftPublisher;
    /** Representation of the BibTeX field key "series" */
    static const QLatin1String ftSeries;
    /** Representation of the BibTeX field key "title" */
    static const QLatin1String ftTitle;
    /** Representation of the BibTeX field key "url" */
    static const QLatin1String ftUrl;
    /** Representation of the BibTeX field key "volume" */
    static const QLatin1String ftVolume;
    /** Representation of the BibTeX field key "year" */
    static const QLatin1String ftYear;

private:
    class FieldPrivate;
    FieldPrivate * const d;
};

}
}

#endif
