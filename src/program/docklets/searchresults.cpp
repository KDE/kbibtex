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
    File *file;
    KPushButton *buttonImport;
    BibTeXEditor *resultList, *mainEditor;
    KAction *actionViewCurrent, *actionImportSelected, *actionCopySelected;

    SearchResultsPrivate(MDIWidget *mdiWidget, SearchResults *parent)
            : p(parent), m(mdiWidget), file(new File()) {
        QGridLayout *layout = new QGridLayout(parent);
        layout->setMargin(0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        resultList = new BibTeXEditor(QLatin1String("SearchResults"), parent);
        resultList->setReadOnly(true);
        resultList->setFrameShadow(QFrame::Sunken);
        resultList->setFrameShape(QFrame::StyledPanel);
        resultList->setContextMenuPolicy(Qt::ActionsContextMenu);
        layout->addWidget(resultList, 0, 0, 1, 2);

        clipboard = new Clipboard(resultList);

        buttonImport = new KPushButton(KIcon("svn-update"), i18n("Import"), parent);
        layout->addWidget(buttonImport, 1, 1, 1, 1);
        buttonImport->setEnabled(false);

        SortFilterBibTeXFileModel *model = new SortFilterBibTeXFileModel(parent);
        BibTeXFileModel *bibTeXModel = new BibTeXFileModel(parent);
        bibTeXModel->setBibTeXFile(file);
        model->setSourceModel(bibTeXModel);
        resultList->setModel(model);

        actionViewCurrent = new KAction(KIcon("document-preview"), i18n("View Element"), parent);
        resultList->addAction(actionViewCurrent);
        actionViewCurrent->setEnabled(false);
        connect(actionViewCurrent, SIGNAL(triggered()), resultList, SLOT(viewCurrentElement()));

        actionImportSelected = new KAction(KIcon("svn-update"), i18n("Import"), parent);
        resultList->addAction(actionImportSelected);
        actionImportSelected->setEnabled(false);
        connect(actionImportSelected, SIGNAL(triggered()), parent, SLOT(importSelected()));

        actionCopySelected = new KAction(KIcon("edit-copy"), i18n("Copy"), parent);
        resultList->addAction(actionCopySelected);
        actionCopySelected->setEnabled(false);
        connect(actionCopySelected, SIGNAL(triggered()), clipboard, SLOT(copy()));

        connect(resultList, SIGNAL(doubleClicked(QModelIndex)), resultList, SLOT(viewCurrentElement()));
        connect(resultList, SIGNAL(selectedElementsChanged()), parent, SLOT(updateGUI()));
        connect(buttonImport, SIGNAL(clicked()), parent, SLOT(importSelected()));
    }

    ~SearchResultsPrivate() {
        delete file;
    }

    void clear() {
        file->clear();
        resultList->reset();
    }

    bool insertElement(Element *element) {
        BibTeXFileModel *model = resultList->bibTeXModel();
        return model->insertRow(element,  model->rowCount());
    }

};

SearchResults::SearchResults(MDIWidget *mdiWidget, QWidget *parent)
        : QWidget(parent), d(new SearchResultsPrivate(mdiWidget, this))
{
    // nothing
}

SearchResults::~SearchResults()
{
    delete d;
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
    d->mainEditor = newEditor;
    updateGUI();
}

void SearchResults::updateGUI()
{
    d->buttonImport->setEnabled(d->mainEditor != NULL && !d->resultList->selectedElements().isEmpty());
    d->actionImportSelected->setEnabled(d->buttonImport->isEnabled());
    d->actionCopySelected->setEnabled(!d->resultList->selectedElements().isEmpty());
    d->actionViewCurrent->setEnabled(d->resultList->currentElement() != NULL);
}

void SearchResults::importSelected()
{
    Q_ASSERT(d->mainEditor != NULL);

    BibTeXFileModel *targetModel = d->mainEditor->bibTeXModel();
    BibTeXFileModel *sourceModel = d->resultList->bibTeXModel();
    QList<QModelIndex> selList = d->resultList->selectionModel()->selectedRows();
    for (QList<QModelIndex>::ConstIterator it = selList.constBegin(); it != selList.constEnd(); ++it) {
        int row = d->resultList->sortFilterProxyModel()->mapToSource(*it).row();
        Element *element = sourceModel->element(row);
        targetModel->insertRow(element, targetModel->rowCount());
    }

    if (!selList.isEmpty())
        d->mainEditor->externalModification();
}
