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

#include <entrylistmodel.h>
#include "entryeditor.h"

using namespace KBibTeX::GUI::Dialogs;

class EntryEditor::EntryEditorPrivate
{
public:
    EntryEditor *p;
    KBibTeX::IO::Entry *entry;
    bool isModified;

    EntryEditorPrivate(KBibTeX::IO::Entry *e, EntryEditor *parent)
            : p(parent), entry(e) {
        isModified = false;
    }

    void apply() {
        p->model()->applyToEntry(*entry);
    }
};

EntryEditor::EntryEditor(KBibTeX::IO::Entry *entry, QWidget *parent)
        : EntryViewer(entry, parent), d(new EntryEditorPrivate(entry, this))
{
    connect((QObject*)delegate(), SIGNAL(modified()), this, SLOT(fieldModified())); // FIXME: For some reason, delegate has to be cast to QObject* ...
}

void EntryEditor::apply()
{
    d->apply();
}

void EntryEditor::reset()
{
    EntryViewer::reset();
    d->isModified = false;
    emit modified(false);
}

void EntryEditor::fieldModified()
{
    d->isModified = true;
    emit modified(true);
}
