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

#include <KDebug>
#include <KComboBox>
#include <KLineEdit>

#include <bibtexentries.h>
#include <entry.h>
#include <fieldinput.h>
#include "entryeditor.h"

class EntryEditor::EntryEditorPrivate
{
public:
    Entry *entry;
    EntryEditor *p;
    bool isModified;

    EntryEditorPrivate(Entry *e, EntryEditor *parent)
            : entry(e), p(parent) {
        isModified = false;
    }

    void apply() {
        apply(entry);
    }

    void apply(Entry *entry) {
        entry->clear();

        BibTeXEntries *be = BibTeXEntries::self();
        QVariant var = p->comboboxType->itemData(p->comboboxType->currentIndex());
        QString type = var.toString();
        if (p->comboboxType->lineEdit()->isModified())
            type = be->format(p->comboboxType->lineEdit()->text(), KBibTeX::cUpperCamelCase);
        entry->setType(type);
        entry->setId(p->entryId->text());

        for (QMap<QString, FieldInput*>::Iterator it = p->bibtexKeyToWidget.begin(); it != p->bibtexKeyToWidget.end(); ++it) {
            Value value;
            it.value()->applyTo(value);
            if (!value.isEmpty()) {
                entry->insert(it.key(), value);
            }
        }

        // TODO: other fields
    }
};

EntryEditor::EntryEditor(Entry *entry, QWidget *parent)
        : EntryViewer(entry, parent), d(new EntryEditorPrivate(entry, this))
{
    setReadOnly(false);

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
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

void EntryEditor::tabChanged(int index)
{
    if (index + 1 == count()) {
        Entry temp(*(d->entry)); // FIXME: switch to references
        d->apply(&temp);
        resetSource(&temp);
    } else {
    }
}
