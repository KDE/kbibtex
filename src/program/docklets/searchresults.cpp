/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "searchresults.h"

#include <QGridLayout>

#include <KLocale>
#include <KPushButton>
#include <KDebug>
#include <KAction>

#include "file.h"
#include "clipboard.h"
#include "fileview.h"
#include "filemodel.h"
#include "idsuggestions.h"

class SearchResults::SearchResultsPrivate
{
private:
    // UNUSED SearchResults *p;
    Clipboard *clipboard;

public:
    MDIWidget *m;
    File *file;
    KPushButton *buttonImport;
    FileView *resultList, *mainEditor;
    KAction *actionViewCurrent, *actionImportSelected, *actionCopySelected;

    SearchResultsPrivate(MDIWidget *mdiWidget, SearchResults *parent)
        : /* UNUSED p(parent),*/ m(mdiWidget), file(new File()), mainEditor(NULL) {
        QGridLayout *layout = new QGridLayout(parent);
        layout->setMargin(0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        resultList = new FileView(QLatin1String("SearchResults"), parent);
        resultList->setItemDelegate(new FileDelegate(resultList));
        resultList->setReadOnly(true);
        resultList->setFrameShadow(QFrame::Sunken);
        resultList->setFrameShape(QFrame::StyledPanel);
        resultList->setContextMenuPolicy(Qt::ActionsContextMenu);
        layout->addWidget(resultList, 0, 0, 1, 2);

        clipboard = new Clipboard(resultList);

        buttonImport = new KPushButton(KIcon("svn-update"), i18n("Import"), parent);
        layout->addWidget(buttonImport, 1, 1, 1, 1);
        buttonImport->setEnabled(false);

        SortFilterFileModel *model = new SortFilterFileModel(parent);
        FileModel *fileModel = new FileModel(parent);
        fileModel->setBibliographyFile(file);
        model->setSourceModel(fileModel);
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
        resultList->fileModel()->reset();
    }

    bool insertElement(QSharedPointer<Element> element) {
        static IdSuggestions idSuggestions;
        FileModel *model = resultList->fileModel();

        /// If the user had configured a default formatting string
        /// for entry ids, apply this formatting strings here
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (!entry.isNull())
            idSuggestions.applyDefaultFormatId(*entry.data());

        bool result = model->insertRow(element, model->rowCount());
        if (result)
            resultList->sortFilterProxyModel()->invalidate();

        return result;
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

bool SearchResults::insertElement(QSharedPointer<Element> element)
{
    return d->insertElement(element);
}

void SearchResults::documentSwitched(FileView *oldEditor, FileView *newEditor)
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
    Q_ASSERT_X(d->mainEditor != NULL, "SearchResults::importSelected", "d->mainEditor is NULL");

    FileModel *targetModel = d->mainEditor->fileModel();
    FileModel *sourceModel = d->resultList->fileModel();
    QList<QModelIndex> selList = d->resultList->selectionModel()->selectedRows();
    for (QList<QModelIndex>::ConstIterator it = selList.constBegin(); it != selList.constEnd(); ++it) {
        /// Map from visible row to 'real' row
        /// that may be hidden through sorting
        int row = d->resultList->sortFilterProxyModel()->mapToSource(*it).row();
        /// Should only be an Entry,
        /// everthing else is unexpected
        QSharedPointer<Entry> entry = sourceModel->element(row).dynamicCast<Entry>();
        if (!entry.isNull()) {
            /// Important: make clone of entry before inserting
            /// in main list, otherwise data would be shared
            QSharedPointer<Entry> clone(new Entry(*entry));
            targetModel->insertRow(clone, targetModel->rowCount());
        } else
            kWarning() << "Trying to import something that isn't an Entry";
    }

    if (!selList.isEmpty())
        d->mainEditor->externalModification();
}
