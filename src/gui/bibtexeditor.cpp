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
#include <KDebug>

#include <entryviewer.h>
#include <entry.h>
#include <bibtexfilemodel.h>
#include "bibtexeditor.h"

using namespace KBibTeX::GUI;

BibTeXEditor::BibTeXEditor(QWidget *parent)
        : KBibTeX::GUI::Widgets::BibTeXFileView(parent)
{
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
}

void BibTeXEditor::viewCurrentElement()
{
    viewElement(currentElement());
}

void BibTeXEditor::viewElement(const KBibTeX::IO::Element *element)
{
    const KBibTeX::IO::Entry *entry = dynamic_cast<const KBibTeX::IO::Entry *>(element);

    if (entry != NULL) {
        KDialog dialog(this);
        KBibTeX::GUI::Dialogs::EntryViewer entryViewer(entry, &dialog);
        dialog.setMainWidget(&entryViewer);
        dialog.setButtons(KDialog::Close);
        dialog.exec();
    }
}

const QList<KBibTeX::IO::Element*>& BibTeXEditor::selectedElements() const
{
    return m_selection;
}

const KBibTeX::IO::Element* BibTeXEditor::currentElement() const
{
    KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel != NULL);
    return bibTeXFileModel->bibTeXSourceModel()->element(currentIndex().row());
}

void BibTeXEditor::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    QTreeView::currentChanged(current, previous);

    KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel != NULL);
    m_current = bibTeXFileModel == NULL ? NULL : bibTeXFileModel->bibTeXSourceModel()->element(current.row());

    emit currentElementChanged(m_current);
}

void BibTeXEditor::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel);

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it)
        m_selection.append(bibTeXFileModel->bibTeXSourceModel()->element(it->row()));

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it)
        m_selection.removeOne(bibTeXFileModel->bibTeXSourceModel()->element(it->row()));

    emit selectedElementsChanged();
}

void BibTeXEditor::itemActivated(const QModelIndex & index)
{
    KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *bibTeXFileModel = dynamic_cast<KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel*>(model());
    Q_ASSERT(bibTeXFileModel != NULL);
    emit elementExecuted(bibTeXFileModel->bibTeXSourceModel()->element(index.row()));
}

