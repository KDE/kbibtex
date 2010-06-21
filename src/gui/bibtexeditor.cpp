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

#include <KDialog>
#include <KLocale>

#include <entryviewer.h>
#include <entryeditor.h>
#include <entry.h>
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
    const Entry *entry = dynamic_cast<const Entry *>(element);

    if (entry != NULL) {
        KDialog dialog(this);
        EntryViewer entryViewer(entry, &dialog);
        dialog.setCaption(i18n("View Entry"));
        dialog.setMainWidget(&entryViewer);
        dialog.setButtons(KDialog::Close);
        dialog.exec();
    }
}

void BibTeXEditor::editElement(Element *element)
{
    Entry *entry = dynamic_cast<Entry *>(element);

    if (entry != NULL) {
        KDialog dialog(this);
        EntryEditor entryEditor(entry, &dialog);
        dialog.setCaption(i18n("Edit Entry"));
        dialog.setMainWidget(&entryEditor);
        dialog.setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Reset);
        dialog.enableButton(KDialog::Apply, false);

        connect(&entryEditor, SIGNAL(modified(bool)), &dialog, SLOT(enableButtonApply(bool)));
        connect(&dialog, SIGNAL(applyClicked()), &entryEditor, SLOT(apply()));
        connect(&dialog, SIGNAL(okClicked()), &entryEditor, SLOT(apply()));
        connect(&dialog, SIGNAL(resetClicked()), &entryEditor, SLOT(reset()));

        dialog.exec();
    }
}

const QList<Element*>& BibTeXEditor::selectedElements() const
{
    return m_selection;
}

const Element* BibTeXEditor::currentElement() const
{
    SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel != NULL);
    QModelIndex currentMapped = bibTeXFileModel->mapToSource(currentIndex());
    return bibTeXFileModel->bibTeXSourceModel()->element(currentMapped.row());
}

void BibTeXEditor::keyPressEvent(QKeyEvent *event)
{
    QTreeView::keyPressEvent(event);
    emit keyPressed(event);
}

void BibTeXEditor::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    QTreeView::currentChanged(current, previous);

    SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel != NULL);
    QModelIndex currentMapped = bibTeXFileModel->mapToSource(current);
    m_current = bibTeXFileModel == NULL ? NULL : bibTeXFileModel->bibTeXSourceModel()->element(currentMapped.row());

    emit currentElementChanged(m_current);
}

void BibTeXEditor::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel);

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        QModelIndex mi = bibTeXFileModel->mapToSource(*it);
        m_selection.append(bibTeXFileModel->bibTeXSourceModel()->element(mi.row()));
    }

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        QModelIndex mi = bibTeXFileModel->mapToSource(*it);
        m_selection.removeOne(bibTeXFileModel->bibTeXSourceModel()->element(mi.row()));
    }

    emit selectedElementsChanged();
}

void BibTeXEditor::itemActivated(const QModelIndex & index)
{
    SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel != NULL);
    QModelIndex indexMapped = bibTeXFileModel->mapToSource(index);
    emit elementExecuted(bibTeXFileModel->bibTeXSourceModel()->element(indexMapped.row()));
}
