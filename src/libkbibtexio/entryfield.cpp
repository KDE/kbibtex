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
#include "entryfield.h"

using namespace KBibTeX::IO;

EntryField::EntryField(FieldType fieldType)
        : m_fieldType(fieldType)
{
    m_fieldTypeName = fieldTypeToString(m_fieldType);
    m_value = new Value();
}

EntryField::EntryField(const QString &fieldTypeName)
        : m_fieldTypeName(fieldTypeName)
{
    m_fieldType = fieldTypeFromString(m_fieldTypeName);
    m_value = new Value();
}

EntryField::EntryField(EntryField *other)
        : m_fieldType(other->m_fieldType), m_fieldTypeName(other->m_fieldTypeName), m_value(NULL)
{
    setValue(other->m_value);
}

EntryField::~EntryField()
{
    delete m_value;
}

QString EntryField::fieldTypeName() const
{
    return m_fieldTypeName;
}

void EntryField::setFieldType(FieldType fieldType, const QString& fieldTypeName)
{
    m_fieldType = fieldType;
    m_fieldTypeName = fieldTypeName;
}

EntryField::FieldType EntryField::fieldType() const
{
    return m_fieldType;
}

QString EntryField::fieldTypeToString(const FieldType fieldType)
{
    switch (fieldType) {
    case ftAbstract:
        return QString("abstract");
    case ftAddress:
        return QString("address");
    case ftAnnote:
        return QString("annote");
    case ftAuthor:
        return QString("author");
    case ftBookTitle:
        return QString("booktitle");
    case ftChapter:
        return QString("chapter");
    case ftCrossRef:
        return QString("crossref");
    case ftDoi:
        return QString("doi");
    case ftEdition:
        return QString("edition");
    case ftEditor:
        return QString("editor");
    case ftHowPublished:
        return QString("howpublished");
    case ftInstitution:
        return QString("institution");
    case ftISBN:
        return QString("isbn");
    case ftISSN:
        return QString("issn");
    case ftJournal:
        return QString("journal");
    case ftKey:
        return QString("key");
    case ftKeywords:
        return QString("keywords");
    case ftLocalFile:
        return QString("localfile");
    case ftLocation:
        return QString("location");
    case ftMonth:
        return QString("month");
    case ftNote:
        return QString("note");
    case ftNumber:
        return QString("number");
    case ftOrganization:
        return QString("organization");
    case ftPages:
        return QString("pages");
    case ftPublisher:
        return QString("publisher");
    case ftSeries:
        return QString("series");
    case ftSchool:
        return QString("school");
    case ftTitle:
        return QString("title");
    case ftType:
        return QString("type");
    case ftURL:
        return QString("url");
    case ftVolume:
        return QString("volume");
    case ftYear:
        return QString("year");
    default:
        return QString("unknown");
    }
}

EntryField::FieldType EntryField::fieldTypeFromString(const QString & fieldTypeString)
{
    QString fieldTypeStringLower = fieldTypeString.toLower();

    if (fieldTypeStringLower == "abstract")
        return ftAbstract;
    else if (fieldTypeStringLower == "address")
        return ftAddress;
    else if (fieldTypeStringLower == "annote")
        return ftAnnote;
    else if (fieldTypeStringLower == "author")
        return ftAuthor;
    else if (fieldTypeStringLower == "booktitle")
        return ftBookTitle;
    else if (fieldTypeStringLower == "chapter")
        return ftChapter;
    else if (fieldTypeStringLower == "crossref")
        return ftCrossRef;
    else if (fieldTypeStringLower == "doi")
        return ftDoi;
    else if (fieldTypeStringLower == "edition")
        return ftEdition;
    else if (fieldTypeStringLower == "editor")
        return ftEditor;
    else if (fieldTypeStringLower == "howpublished")
        return ftHowPublished;
    else if (fieldTypeStringLower == "institution")
        return ftInstitution;
    else if (fieldTypeStringLower == "isbn")
        return ftISBN;
    else if (fieldTypeStringLower == "issn")
        return ftISSN;
    else if (fieldTypeStringLower == "journal")
        return ftJournal;
    else if (fieldTypeStringLower == "key")
        return ftKey;
    else if (fieldTypeStringLower == "keywords")
        return ftKeywords;
    else if (fieldTypeStringLower == "localfile")
        return ftLocalFile;
    else if (fieldTypeStringLower == "location")
        return ftLocation;
    else if (fieldTypeStringLower == "month")
        return ftMonth;
    else if (fieldTypeStringLower == "note")
        return ftNote;
    else if (fieldTypeStringLower == "number")
        return ftNumber;
    else if (fieldTypeStringLower == "organization")
        return ftOrganization;
    else if (fieldTypeStringLower == "pages")
        return ftPages;
    else if (fieldTypeStringLower == "publisher")
        return ftPublisher;
    else if (fieldTypeStringLower == "series")
        return ftSeries;
    else if (fieldTypeStringLower == "school")
        return ftSchool;
    else if (fieldTypeStringLower == "title")
        return ftTitle;
    else if (fieldTypeStringLower == "type")
        return ftType;
    else if (fieldTypeStringLower == "url")
        return ftURL;
    else if (fieldTypeStringLower == "volume")
        return ftVolume;
    else if (fieldTypeStringLower == "year")
        return ftYear;
    else
        return ftUnknown;
}

Value *EntryField::value()
{
    return m_value;
}

void EntryField::setValue(const Value *value)
{
    if (value != m_value) {
        delete m_value;

        if (value != NULL) {
            m_value = new Value(value);
        } else
            m_value = new Value();
    }
}
