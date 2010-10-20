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

#include <QGridLayout>

#include <KLocale>
#include <KPushButton>
#include <KDebug>
#include <KAction>

#include <file.h>
#include <clipboard.h>
#include <bibtexeditor.h>
#include <bibtexfilemodel.h>
#include "searchresults.h"

class SearchResults::SearchResultsPrivate
{
private:
    SearchResults *p;
    Clipboard *clipboard;

public:
    MDIWidget *m;
    BibTeXEditor *currentFile;
    KPushButton *buttonImport;
    BibTeXEditor *editor;
    KAction *actionViewCurrent, *actionImportSelected, *actionCopySelected;

    SearchResultsPrivate(MDIWidget *mdiWidget, SearchResults *parent)
            : p(parent), m(mdiWidget), currentFile(NULL) {
        QGridLayout *layout = new QGridLayout(parent);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        editor = new BibTeXEditor(parent);
        editor->setReadOnly(true);
        editor->setFrameStyle(QFrame::Panel);
        editor->setContextMenuPolicy(Qt::ActionsContextMenu);
        layout->addWidget(editor, 0, 0, 1, 2);

        clipboard = new Clipboard(editor);

        buttonImport = new KPushButton(KIcon("svn-update"), i18n("Import"), parent);
        layout->addWidget(buttonImport, 1, 1, 1, 1);
        buttonImport->setEnabled(false);

        SortFilterBibTeXFileModel *model = new SortFilterBibTeXFileModel(parent);
        model->setSourceModel(new BibTeXFileModel(parent));
        editor->setModel(model);

        actionViewCurrent = new KAction(KIcon("document-preview"), i18n("View Element"), parent);
        editor->addAction(actionViewCurrent);
        actionViewCurrent->setEnabled(false);
        connect(actionViewCurrent, SIGNAL(triggered()), editor, SLOT(viewCurrentElement()));

        actionImportSelected = new KAction(KIcon("svn-update"), i18n("Import"), parent);
        editor->addAction(actionImportSelected);
        actionImportSelected->setEnabled(false);
        connect(actionImportSelected, SIGNAL(triggered()), parent, SLOT(importSelected()));

        actionCopySelected = new KAction(KIcon("edit-copy"), i18n("Copy"), parent);
        editor->addAction(actionCopySelected);
        actionCopySelected->setEnabled(false);
        connect(actionCopySelected, SIGNAL(triggered()), clipboard, SLOT(copy()));

        connect(editor, SIGNAL(doubleClicked(QModelIndex)), editor, SLOT(viewCurrentElement()));
        connect(editor, SIGNAL(selectedElementsChanged()), parent, SLOT(updateGUI()));
        connect(buttonImport, SIGNAL(clicked()), parent, SLOT(importSelected()));
    }

    void clear() {
        File *file = editor->bibTeXModel()->bibTeXFile();
        delete file;
        editor->bibTeXModel()->setBibTeXFile(new File());
    }

    bool insertElement(Element *element) {
        BibTeXFileModel *model = editor->bibTeXModel();
        return model->insertRow(element,  model->rowCount());
    }

};

SearchResults::SearchResults(MDIWidget *mdiWidget, QWidget *parent)
        : QWidget(parent), d(new SearchResultsPrivate(mdiWidget, this))
{
    // nothing
}

void SearchResults::clear()
{
    d->clear();
}

bool SearchResults::insertElement(Element *element)
{
    return d->insertElement(element);
}

void SearchResults::documentSwitched(BibTeXEditor *oldEditor, BibTeXEditor *newEditor)
{
    Q_UNUSED(oldEditor);
    d->currentFile = newEditor;
    updateGUI();
}

void SearchResults::updateGUI()
{
    d->buttonImport->setEnabled(d->currentFile != NULL && !d->editor->selectedElements().isEmpty());
    d->actionImportSelected->setEnabled(d->buttonImport->isEnabled());
    d->actionCopySelected->setEnabled(!d->editor->selectedElements().isEmpty());
    d->actionViewCurrent->setEnabled(d->editor->currentElement() != NULL);
}

void SearchResults::importSelected()
{
    Q_ASSERT(d->currentFile != NULL);

    BibTeXFileModel *targetModel = d->currentFile->bibTeXModel();
    BibTeXFileModel *sourceModel = d->editor->bibTeXModel();
    QList<QModelIndex> selList = d->editor->selectionModel()->selectedRows();
    for (QList<QModelIndex>::ConstIterator it = selList.constBegin(); it != selList.constEnd(); ++it) {
        int row = d->editor->sortFilterProxyModel()->mapToSource(*it).row();
        Element *element = sourceModel->element(row);
        targetModel->insertRow(element, targetModel->rowCount());
    }
}
