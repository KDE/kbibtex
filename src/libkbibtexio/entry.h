/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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

#include <QLinkedList>
#include <QStringList>

#include <element.h>
#include <entryfield.h>

class QString;

namespace KBibTeX
{
namespace IO {

class Entry : public Element
{
public:
    typedef QLinkedList<EntryField*> EntryFields;

    enum EntryType {etArticle, etBook, etBooklet, etCollection, etElectronic, etInBook, etInCollection, etInProceedings, etManual, etMastersThesis, etMisc, etPhDThesis, etProceedings, etTechReport, etUnpublished, etUnknown};

    enum FieldRequireStatus {frsRequired, frsOptional, frsIgnored};

    enum MergeSemantics {msIgnoreOther, msAddNew, msForceAdding};

    Entry();
    Entry(const EntryType entryType, const QString &id);
    Entry(const QString& entryTypeString, const QString& id);
    Entry(const Entry *other);
    virtual ~Entry();
    Element* clone() const;
    bool equals(const Entry &other);
    QString text() const;

    void setEntryType(const EntryType elementType);
    void setEntryTypeString(const QString& elementTypeString);
    EntryType entryType() const;
    QString entryTypeString() const;
    void setId(const QString& id);
    QString id() const;

    bool containsPattern(const QString& pattern, EntryField::FieldType fieldType = EntryField::ftUnknown, Element::FilterType filterType = Element::ftExact, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;
    QStringList urls() const;

    bool addField(EntryField *field);
    EntryField* getField(const EntryField::FieldType fieldType) const;
    EntryField* getField(const QString& fieldName) const;
    bool deleteField(const EntryField::FieldType fieldType);
    bool deleteField(const QString &fieldName);

    EntryFields::const_iterator begin() const;
    EntryFields::const_iterator end() const;
    int getFieldCount() const;
    void clearFields();

    void copyFrom(const Entry *other);
    void merge(Entry *other, MergeSemantics mergeSemantics);

    static QString entryTypeToString(const EntryType entryType);
    static EntryType entryTypeFromString(const QString &entryTypeString);
    static Entry::FieldRequireStatus getRequireStatus(Entry::EntryType entryType, EntryField::FieldType fieldType);

private:
    EntryType m_entryType;
    QString m_entryTypeString;
    QString m_id;
    EntryFields m_fields;
};

}
}

#endif
