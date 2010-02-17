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
#ifndef BIBTEXBIBTEXENTRY_H
#define BIBTEXBIBTEXENTRY_H

#include <QMap>

#include "element.h"
#include "value.h"

namespace KBibTeX
{
namespace IO {

/**
 * This class represents an entry in a BibTeX file such as an article
 * or a book. This class is essentially a map from keys such as title,
 * year or other bibliography data to corresponding values.
 * @see Value
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT Entry : public Element, public QMap<QString, Value>
{
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString type READ type WRITE setType)

public:
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

    /** Representation of the BibTeX entry type "Article" */
    static const QLatin1String etArticle;
    /** Representation of the BibTeX entry type "Book" */
    static const QLatin1String etBook;
    /** Representation of the BibTeX entry type "InBook" */
    static const QLatin1String etInBook;
    /** Representation of the BibTeX entry type "InProceedings" */
    static const QLatin1String etInProceedings;
    /** Representation of the BibTeX entry type "Misc" */
    static const QLatin1String etMisc;
    /** Representation of the BibTeX entry type "TechReport" */
    static const QLatin1String etTechReport;
    /** Representation of the BibTeX entry type "PhDThesis" */
    static const QLatin1String etPhDThesis;

    /**
     * Create a new entry type. Both type and id are optionally,
     * allowing to call the constructor as Entry() only.
     * Both type and id can be set and retrieved later.
     * @param type type of this entry
     */
    Entry(const QString& type = QString::null, const QString &id = QString::null);

    /**
     * Copy constructor cloning another entry object.
     * @param other entry object to clone
     */
    Entry(const Entry &other);

    virtual ~Entry();

    /**
     * Assignment operator, working similar to a copy constructor,
     * but overwrites the current object's values.
     */
    Entry& operator= (const Entry& other);

    /**
     * Set the type of this entry. Common values are "article" or "book".
     * @param type type of this entry
     */
    void setType(const QString& type);

    /**
     * Retrieve the type of this entry. Common values are "article" or "book".
     * @return type of this entry
     */
    QString type() const;

    /**
     * Set the id of this entry. In LaTeX, this id is used to refer to a BibTeX
     * entry using the "ref" command.
     * @param id id of this entry
     */
    void setId(const QString& id);

    /**
     * Retrieve the id of this entry. In LaTeX, this id is used to refer to a BibTeX
     * entry using the "ref" command.
     * @return id of this entry
     */
    QString id() const;

    /**
     * Re-implementation of QMap's value function, but performing a case-insensitive
     * match on the key. Querying for key "title" will find a key-value pair with
     * key "TITLE".
     * @param key field name to search for
     * @return found value or Value() if nothing found
     */
    const Value value(const QString& key) const;

private:
    class EntryPrivate;
    EntryPrivate * const d;
};

}
}

#endif // BIBTEXBIBTEXENTRY_H
