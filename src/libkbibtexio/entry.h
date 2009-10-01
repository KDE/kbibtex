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
#ifndef BIBTEXBIBTEXENTRY_H
#define BIBTEXBIBTEXENTRY_H

#include <QMap>

#include "element.h"
#include "value.h"

namespace KBibTeX
{
namespace IO {

class KBIBTEXIO_EXPORT Entry : public Element, public QMap<QString, Value>
{
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

    static const QLatin1String etArticle;
    static const QLatin1String etBook;
    static const QLatin1String etInBook;
    static const QLatin1String etInProceedings;
    static const QLatin1String etMisc;
    static const QLatin1String etTechReport;
    static const QLatin1String etPhDThesis;

    //enum FieldRequireStatus {frsRequired, frsOptional, frsIgnored};

    //enum MergeSemantics {msAddNew, msForceAdding};

    Entry(const QString& type, const QString &id);
    Entry(const Entry &other);
    virtual ~Entry();
    //Element* clone() const;
    // bool equals(const Entry &other); // FIXME Is this function required?
    // QString text() const; // FIXME: Is this function required?

    void setType(const QString& type);
    QString type() const;

    void setId(const QString& id);
    QString id() const;

    // bool containsPattern(const QString& pattern, Field::FieldType fieldType = Field::ftUnknown, Element::FilterType filterType = Element::ftExact, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const; // FIXME: Rewrite filtering code
    //QStringList urls() const;

//    void copyFrom(const Entry *other);
//   void merge(Entry *other, MergeSemantics mergeSemantics);

//    static Entry::FieldRequireStatus getRequireStatus(Entry::EntryType entryType, const QString& fieldType);

private:
    class EntryPrivate;
    EntryPrivate * const d;
};

}
}

#endif // BIBTEXBIBTEXENTRY_H
