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

class KBIBTEXIO_EXPORT EntryField
{
public:
    enum FieldType {ftAbstract, ftAddress, ftAnnote, ftAuthor, ftBookTitle, ftChapter, ftCrossRef, ftDoi, ftEdition, ftEditor, ftHowPublished, ftInstitution, ftISBN, ftISSN, ftJournal, ftKey, ftKeywords, ftLocalFile, ftLocation, ftMonth, ftNote, ftNumber, ftOrganization, ftPages, ftPublisher, ftSchool, ftSeries, ftTitle, ftType, ftURL, ftVolume, ftYear, ftUnknown = -1};

    EntryField(FieldType fieldType);
    EntryField(const QString &fieldTypeName);
    EntryField(EntryField *other);
    ~EntryField();

    QString fieldTypeName() const;
    FieldType fieldType() const;
    void setFieldType(FieldType fieldType, const QString& fieldTypeName);

    static QString fieldTypeToString(const FieldType fieldType);
    static FieldType fieldTypeFromString(const QString &fieldTypeString);

    Value *value();
    void setValue(const Value *value);

private:
    FieldType m_fieldType;
    QString m_fieldTypeName;
    Value *m_value;
};

}
}

#endif
