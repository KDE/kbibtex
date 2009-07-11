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
#ifndef BIBTEXELEMENT_H
#define BIBTEXELEMENT_H

#include <file.h>
#include <entryfield.h>

#include "kbibtexio_export.h"

namespace KBibTeX
{
namespace IO {

class KBIBTEXIO_EXPORT Element
{
public:
    enum FilterType {ftExact, ftEveryWord, ftAnyWord};

    Element();
    virtual ~Element();

    virtual bool containsPattern(const QString& /* pattern */, EntryField::FieldType /* fieldType */, FilterType /* filterType */ = Element::ftExact, Qt::CaseSensitivity /* caseSensitive */ = Qt::CaseInsensitive) const = 0;
    virtual Element* clone() const = 0;
    virtual QString text() const = 0;

    static bool isSimpleString(const QString &text);

};

}
}

#endif
