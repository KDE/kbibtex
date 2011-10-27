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

#ifndef KBIBTEX_GUI_BIBTEXENTRIES_H
#define KBIBTEX_GUI_BIBTEXENTRIES_H

#include "kbibtexio_export.h"

#include "kbibtexnamespace.h"

typedef struct {
    QString upperCamelCase;
    QString upperCamelCaseAlt;
    QString label;
    QStringList requiredItems;
    QStringList optionalItems;
} EntryDescription;

bool operator==(const EntryDescription &a, const EntryDescription &b);
uint qHash(const EntryDescription &a);

/**
@author Thomas Fischer
*/
class KBIBTEXIO_EXPORT BibTeXEntries : public QList<EntryDescription>
{
public:
    virtual ~BibTeXEntries();

    static BibTeXEntries *self();

    /**
     * Change the casing of a given entry name to one of the predefine formats.
     */
    QString format(const QString& name, KBibTeX::Casing casing) const;

    QString label(const QString& name) const;

protected:
    BibTeXEntries();
    void load();

private:
    class BibTeXEntriesPrivate;
    BibTeXEntriesPrivate *d;
};

#endif // KBIBTEX_GUI_BIBTEXENTRIES_H
