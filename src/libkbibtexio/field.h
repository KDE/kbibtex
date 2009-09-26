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

class KBIBTEXIO_EXPORT Field
{
public:
    Field(const QString& fieldType, const Value& value = Value());
    Field(const Field& other);

    QString fieldType() const;
    void setFieldType(const QString& fieldTypeName);

    Value& value();
    const Value& value() const;
    void setValue(const Value& value);

    static const QLatin1String ftAbstract;
    static const QLatin1String ftAddress;
    static const QLatin1String ftAuthor;
    static const QLatin1String ftBookTitle;
    static const QLatin1String ftChapter;
    static const QLatin1String ftCrossRef;
    static const QLatin1String ftDOI;
    static const QLatin1String ftEditor;
    static const QLatin1String ftISSN;
    static const QLatin1String ftISBN;
    static const QLatin1String ftJournal;
    static const QLatin1String ftKeywords;
    static const QLatin1String ftLocation;
    static const QLatin1String ftMonth;
    static const QLatin1String ftNote;
    static const QLatin1String ftNumber;
    static const QLatin1String ftPages;
    static const QLatin1String ftPublisher;
    static const QLatin1String ftSeries;
    static const QLatin1String ftTitle;
    static const QLatin1String ftURL;
    static const QLatin1String ftVolume;
    static const QLatin1String ftYear;

private:
    QString m_fieldType;
    Value m_value;
};

}
}

#endif
