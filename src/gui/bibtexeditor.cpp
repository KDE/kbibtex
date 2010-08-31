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

#include <KDialog>
#include <KLocale>

#include <entryeditor.h>
#include <entry.h>
#include <macroeditor.h>
#include <macro.h>
#include <bibtexfilemodel.h>
#include "bibtexeditor.h"

BibTeXEditor::BibTeXEditor(QWidget *parent)
        : BibTeXFileView(parent)
{
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
}

void BibTeXEditor::viewCurrentElement()
{
    viewElement(currentElement());
}

void BibTeXEditor::viewElement(const Element *element)
{
    // FIXME: Re-use code better
    const Entry *entry = dynamic_cast<const Entry *>(element);

    if (entry != NULL) {
        KDialog dialog(this);
        EntryEditor entryEditor(entry, &dialog);
        entryEditor.setReadOnly(true);
        dialog.setCaption(i18n("View Entry"));
        dialog.setMainWidget(&entryEditor);
        dialog.setButtons(KDialog::Close);
        dialog.exec();
    } else {
        const Macro *macro = dynamic_cast<const Macro *>(element);
        if (macro != NULL) {
            KDialog dialog(this);
            MacroEditor macroEditor(macro, &dialog);
            macroEditor.setReadOnly(true);
            dialog.setCaption(i18n("View Entry"));
            dialog.setMainWidget(&macroEditor);
            dialog.setButtons(KDialog::Close);
            dialog.exec();
        }
    }
}

void BibTeXEditor::editElement(Element *element)
{
    // FIXME: Re-use code better
    Entry *entry = dynamic_cast<Entry *>(element);
    if (entry != NULL) {
        KDialog dialog(this);
        EntryEditor entryEditor(entry, &dialog);
        entryEditor.reset();
        dialog.setCaption(i18n("Edit Entry"));
        dialog.setMainWidget(&entryEditor);
        dialog.setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Reset);
        dialog.enableButton(KDialog::Apply, false);

        connect(&entryEditor, SIGNAL(modified(bool)), &dialog, SLOT(enableButtonApply(bool)));
        connect(&dialog, SIGNAL(applyClicked()), &entryEditor, SLOT(apply()));
        connect(&dialog, SIGNAL(okClicked()), &entryEditor, SLOT(apply()));
        connect(&dialog, SIGNAL(resetClicked()), &entryEditor, SLOT(reset()));

        dialog.exec();
    } else {
        Macro *macro = dynamic_cast<Macro *>(element);
        if (macro != NULL) {
            KDialog dialog(this);
            MacroEditor macroEditor(macro, &dialog);
            macroEditor.reset();
            dialog.setCaption(i18n("Edit Macro"));
            dialog.setMainWidget(&macroEditor);
            dialog.setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Reset);
            dialog.enableButton(KDialog::Apply, false);

            connect(&macroEditor, SIGNAL(modified(bool)), &dialog, SLOT(enableButtonApply(bool)));
            connect(&dialog, SIGNAL(applyClicked()), &macroEditor, SLOT(apply()));
            connect(&dialog, SIGNAL(okClicked()), &macroEditor, SLOT(apply()));
            connect(&dialog, SIGNAL(resetClicked()), &macroEditor, SLOT(reset()));

            dialog.exec();
        }
    }
}

const QList<Element*>& BibTeXEditor::selectedElements() const
{
    return m_selection;
}

void BibTeXEditor::setSelectedElements(QList<Element*> &list)
{
    m_selection = list;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    for (QList<Element*>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        int row = model()->row(*it);
        QModelIndex idx = model()->index(row, 0);
        selModel->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
    }
}

void BibTeXEditor::setSelectedElement(Element* element)
{
    m_selection.clear();
    m_selection << element;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    int row = model()->row(element);
    QModelIndex idx = model()->index(row, 0);
    selModel->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
}

const Element* BibTeXEditor::currentElement() const
{
    return m_current;
}

void BibTeXEditor::keyPressEvent(QKeyEvent *event)
{
    QTreeView::keyPressEvent(event);
    emit keyPressed(event);
}

void BibTeXEditor::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    QTreeView::currentChanged(current, previous);

    emit currentElementChanged(model()->element(current.row()), model()->bibTeXFile());
}

void BibTeXEditor::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.append(model()->element((*it).row()));
    }

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.removeOne(model()->element((*it).row()));
    }

    emit selectedElementsChanged();
}

void BibTeXEditor::itemActivated(const QModelIndex & index)
{
    emit elementExecuted(model()->element(index.row()));
}
