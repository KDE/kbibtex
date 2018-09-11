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

#ifndef BIBTEXBIBTEXENTRY_H
#define BIBTEXBIBTEXENTRY_H

#include <QMap>

#include "element.h"
#include "value.h"

class File;

/**
 * This class represents an entry in a BibTeX file such as an article
 * or a book. This class is essentially a map from keys such as title,
 * year or other bibliography data to corresponding values.
 * @see Value
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT Entry : public Element, public QMap<QString, Value>
{
public:
    /** Representation of the BibTeX field key "abstract" */
    static const QString ftAbstract;
    /** Representation of the BibTeX field key "address" */
    static const QString ftAddress;
    /** Representation of the BibTeX field key "author" */
    static const QString ftAuthor;
    /** Representation of the BibTeX field key "booktitle" */
    static const QString ftBookTitle;
    /** Representation of the BibTeX field key "chapter" */
    static const QString ftChapter;
    /** Representation of the BibTeX field key "x-color" */
    static const QString ftColor;
    /** Representation of the BibTeX field key "comment" */
    static const QString ftComment;
    /** Representation of the BibTeX field key "crossref" */
    static const QString ftCrossRef;
    /** Representation of the BibTeX field key "doi" */
    static const QString ftDOI;
    /** Representation of the BibTeX field key "editor" */
    static const QString ftEditor;
    /** Representation of the BibTeX field key "file" */
    static const QString ftFile;
    /** Representation of the BibTeX field key "issn" */
    static const QString ftISSN;
    /** Representation of the BibTeX field key "isbn" */
    static const QString ftISBN;
    /** Representation of the BibTeX field key "journal" */
    static const QString ftJournal;
    /** Representation of the BibTeX field key "keywords" */
    static const QString ftKeywords;
    /** Representation of the BibTeX field key "localfile" */
    static const QString ftLocalFile;
    /** Representation of the BibTeX field key "location" */
    static const QString ftLocation;
    /** Representation of the BibTeX field key "month" */
    static const QString ftMonth;
    /** Representation of the BibTeX field key "note" */
    static const QString ftNote;
    /** Representation of the BibTeX field key "number" */
    static const QString ftNumber;
    /** Representation of the BibTeX field key "pages" */
    static const QString ftPages;
    /** Representation of the BibTeX field key "publisher" */
    static const QString ftPublisher;
    /** Representation of the BibTeX field key "school" */
    static const QString ftSchool;
    /** Representation of the BibTeX field key "series" */
    static const QString ftSeries;
    /** Representation of the BibTeX field key "x-stars" */
    static const QString ftStarRating;
    /** Representation of the BibTeX field key "title" */
    static const QString ftTitle;
    /** Representation of the BibTeX field key "url" */
    static const QString ftUrl;
    /** Representation of the BibLaTeX field key "urldate" */
    static const QString ftUrlDate;
    /** Representation of the BibTeX field key "volume" */
    static const QString ftVolume;
    /** Representation of the BibTeX field key "year" */
    static const QString ftYear;

    /** Representation of the BibTeX entry type "Article" */
    static const QString etArticle;
    /** Representation of the BibTeX entry type "Book" */
    static const QString etBook;
    /** Representation of the BibTeX entry type "InBook" */
    static const QString etInBook;
    /** Representation of the BibTeX entry type "InProceedings" */
    static const QString etInProceedings;
    /** Representation of the BibTeX entry type "Misc" */
    static const QString etMisc;
    /** Representation of the BibTeX entry type "TechReport" */
    static const QString etTechReport;
    /** Representation of the BibTeX entry type "PhDThesis" */
    static const QString etPhDThesis;
    /** Representation of the BibTeX entry type "MastersThesis" */
    static const QString etMastersThesis;
    /** Representation of the BibTeX entry type "Unpublished" */
    static const QString etUnpublished;

    /**
     * Create a new entry type. Both type and id are optionally,
     * allowing to call the constructor as Entry() only.
     * Both type and id can be set and retrieved later.
     * @param type type of this entry
     */
    explicit Entry(const QString &type = QString(), const QString &id = QString());

    /**
     * Copy constructor cloning another entry object.
     * @param other entry object to clone
     */
    Entry(const Entry &other);

    ~Entry() override;

    bool operator==(const Entry &other) const;
    bool operator!=(const Entry &other) const;

    /**
     * Assignment operator, working similar to a copy constructor,
     * but overwrites the current object's values.
     */
    Entry &operator= (const Entry &other);

    /**
     * Set the type of this entry. Common values are "article" or "book".
     * @param type type of this entry
     */
    void setType(const QString &type);

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
    void setId(const QString &id);

    /**
     * Retrieve the id of this entry. In LaTeX, this id is used to refer to a BibTeX
     * entry using the "ref" command.
     * @return id of this entry
     */
    QString id() const;

    /**
     * Re-implementation of QMap's value function, but performing a case-insensitive
     * match on the key. E.g. querying for key "title" will find a key-value pair with
     * key "TITLE".
     * @see #contains(const QString&)
     * @param key field name to search for
     * @return found value or Value() if nothing found
     */
    const Value value(const QString &key) const;

    int remove(const QString &key);

    /**
     * Re-implementation of QMap's contains function, but performing a case-insensitive
     * match on the key. E.g. querying for key "title" will find a key-value pair with
     * key "TITLE".
     * @see #value(const QString&)
     * @param key field name to search for
     * @return true if value with key found, else false
     */
    bool contains(const QString &key) const;

    Entry *resolveCrossref(const File *bibTeXfile) const;
    static Entry *resolveCrossref(const Entry &original, const File *bibTeXfile);

    static QStringList authorsLastName(const Entry &entry);
    QStringList authorsLastName() const;

    quint64 uniqueId() const;

    /**
     * Cheap and fast test if another Element is a Entry object.
     * @param other another Element object to test
     * @return true if Element is actually a Entry
     */
    static bool isEntry(const Element &other);

private:
    /// Unique numeric identifier
    const quint64 internalUniqueId;
    /// Keeping track of next available unique numeric identifier
    static quint64 internalUniqueIdCounter;

    class EntryPrivate;
    EntryPrivate *const d;
};

Q_DECLARE_METATYPE(Entry)
Q_DECLARE_METATYPE(Entry *)

QDebug operator<<(QDebug dbg, const Entry &Entry);

/**
 * Comparison operator, necessary for QMap operations.
 */
static inline bool operator< (const QSharedPointer<Entry> &a, const QSharedPointer<Entry> &b)
{
    return a->uniqueId() < b->uniqueId();
}

#endif // BIBTEXBIBTEXENTRY_H
