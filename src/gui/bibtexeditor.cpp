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

#include <elementeditor.h>
#include <entry.h>
#include <macro.h>
#include <bibtexfilemodel.h>
#include "bibtexeditor.h"

BibTeXEditor::BibTeXEditor(QWidget *parent)
        : BibTeXFileView(parent), m_current(NULL)
{
    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
}

void BibTeXEditor::viewCurrentElement()
{
    viewElement(currentElement());
}

void BibTeXEditor::viewElement(const Element *element)
{
    KDialog dialog(this);
    ElementEditor elementEditor(element, &dialog);
    dialog.setCaption(i18n("View Element"));
    dialog.setMainWidget(&elementEditor);
    dialog.setButtons(KDialog::Close);
    dialog.exec();
}

void BibTeXEditor::editCurrentElement()
{
    editElement(currentElement());
}

void BibTeXEditor::editElement(Element *element)
{
    Q_ASSERT_X(element->uniqueId % 1000 == 42, "void BibTeXEditor::editElement(Element *element)", "Invalid Element passed as argument");

    KDialog dialog(this);
    ElementEditor elementEditor(element, &dialog);
    dialog.setCaption(i18n("Edit Element"));
    dialog.setMainWidget(&elementEditor);
    dialog.setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Reset);
    dialog.enableButton(KDialog::Apply, false);

    connect(&elementEditor, SIGNAL(modified(bool)), &dialog, SLOT(enableButtonApply(bool)));
    connect(&dialog, SIGNAL(applyClicked()), &elementEditor, SLOT(apply()));
    connect(&dialog, SIGNAL(okClicked()), &elementEditor, SLOT(apply()));
    connect(&dialog, SIGNAL(resetClicked()), &elementEditor, SLOT(reset()));

    dialog.exec();
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
        int row = bibTeXModel()->row(*it);
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
    int row = bibTeXModel()->row(element);
    QModelIndex idx = bibTeXModel()->index(row, 0);
    selModel->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
}

const Element* BibTeXEditor::currentElement() const
{
    return m_current;
}

Element* BibTeXEditor::currentElement()
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

    Element *element = bibTeXModel()->element(sortFilterProxyModel()->mapToSource(current).row());
    emit currentElementChanged(element, bibTeXModel()->bibTeXFile());
}

void BibTeXEditor::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.append(bibTeXModel()->element((*it).row()));
    }
    if (m_current == NULL && !set.isEmpty())
        m_current = bibTeXModel()->element(set.first().row());

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.removeOne(bibTeXModel()->element((*it).row()));
    }


    emit selectedElementsChanged();
}

void BibTeXEditor::itemActivated(const QModelIndex & index)
{
    emit elementExecuted(bibTeXModel()->element(sortFilterProxyModel()->mapToSource(index).row()));
}
