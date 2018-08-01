/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "searchresults.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QAction>

#include <KLocalizedString>
#include <KIconLoader>

#include "file.h"
#include "clipboard.h"
#include "fileview.h"
#include "filedelegate.h"
#include "models/filemodel.h"
#include "idsuggestions.h"
#include "logging_program.h"

class SearchResults::SearchResultsPrivate
{
private:
    // UNUSED SearchResults *p;
    Clipboard *clipboard;

public:
    MDIWidget *m;
    File *file;
    QWidget *widgetCannotImport;
    QLabel *labelCannotImportMsg;
    QPushButton *buttonImport;
    FileView *resultList, *mainEditor;
    QAction *actionViewCurrent, *actionImportSelected, *actionCopySelected;

    SearchResultsPrivate(MDIWidget *mdiWidget, SearchResults *parent)
        : /* UNUSED p(parent),*/ m(mdiWidget), file(new File()), mainEditor(nullptr) {
        QGridLayout *layout = new QGridLayout(parent);
        layout->setMargin(0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        resultList = new FileView(QStringLiteral("SearchResults"), parent);
        resultList->setItemDelegate(new FileDelegate(resultList));
        resultList->setReadOnly(true);
        resultList->setFrameShadow(QFrame::Sunken);
        resultList->setFrameShape(QFrame::StyledPanel);
        resultList->setContextMenuPolicy(Qt::ActionsContextMenu);
        layout->addWidget(resultList, 0, 0, 1, 2);

        clipboard = new Clipboard(resultList);

        /// Create a special widget that shows a small icon and a text
        /// stating that there are no search results. It will only be
        /// shown until the first search results arrive.
        // TODO nearly identical code as in ElementFormPrivate constructor, create common class
        widgetCannotImport = new QWidget(parent);
        layout->addWidget(widgetCannotImport, 1, 0, 1, 1);
        QBoxLayout *layoutCannotImport = new QHBoxLayout(widgetCannotImport);
        layoutCannotImport->addStretch(10);
        QLabel *label = new QLabel(widgetCannotImport);
        label->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        label->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("dialog-warning"), KIconLoader::Dialog, KIconLoader::SizeSmall));
        layoutCannotImport->addWidget(label);
        labelCannotImportMsg = new QLabel(widgetCannotImport);
        labelCannotImportMsg->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layoutCannotImport->addWidget(labelCannotImportMsg);
        /// see also updateCannotImportMessage()

        buttonImport = new QPushButton(QIcon::fromTheme(QStringLiteral("svn-update")), i18n("Import"), parent);
        layout->addWidget(buttonImport, 1, 1, 1, 1);
        buttonImport->setEnabled(false);

        SortFilterFileModel *model = new SortFilterFileModel(parent);
        FileModel *fileModel = new FileModel(parent);
        fileModel->setBibliographyFile(file);
        model->setSourceModel(fileModel);
        resultList->setModel(model);

        actionViewCurrent = new QAction(QIcon::fromTheme(QStringLiteral("document-preview")), i18n("View Element"), parent);
        resultList->addAction(actionViewCurrent);
        actionViewCurrent->setEnabled(false);
        connect(actionViewCurrent, &QAction::triggered, resultList, &FileView::viewCurrentElement);

        actionImportSelected = new QAction(QIcon::fromTheme(QStringLiteral("svn-update")), i18n("Import"), parent);
        resultList->addAction(actionImportSelected);
        actionImportSelected->setEnabled(false);
        connect(actionImportSelected, &QAction::triggered, parent, &SearchResults::importSelected);

        actionCopySelected = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy"), parent);
        resultList->addAction(actionCopySelected);
        actionCopySelected->setEnabled(false);
        connect(actionCopySelected, &QAction::triggered, clipboard, &Clipboard::copy);

        connect(resultList, &FileView::doubleClicked, resultList, &FileView::viewCurrentElement);
        connect(resultList, &FileView::selectedElementsChanged, parent, &SearchResults::updateGUI);
        connect(buttonImport, &QPushButton::clicked, parent, &SearchResults::importSelected);

        updateCannotImportMessage();
    }

    ~SearchResultsPrivate() {
        delete file;
    }

    void clear() {
        resultList->fileModel()->clear();
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

    void updateCannotImportMessage() {
        const bool isEmpty = resultList->model()->rowCount() == 0;
        if (mainEditor == nullptr && isEmpty)
            labelCannotImportMsg->setText(i18n("No results to import and no bibliography open to import to."));
        else if (mainEditor == nullptr)
            labelCannotImportMsg->setText(i18n("No bibliography open to import to."));
        else if (isEmpty)
            labelCannotImportMsg->setText(i18n("No results to import."));
        widgetCannotImport->setVisible(mainEditor == nullptr || isEmpty);
    }
};

SearchResults::SearchResults(MDIWidget *mdiWidget, QWidget *parent)
        : QWidget(parent), d(new SearchResultsPrivate(mdiWidget, this))
{
    /// nothing
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
    const bool success = d->insertElement(element);
    if (success)
        d->updateCannotImportMessage();
    return success;
}

void SearchResults::documentSwitched(FileView *oldEditor, FileView *newEditor)
{
    Q_UNUSED(oldEditor);
    d->mainEditor = newEditor;
    updateGUI();
}

void SearchResults::updateGUI()
{
    d->updateCannotImportMessage();
    d->buttonImport->setEnabled(d->mainEditor != nullptr && !d->resultList->selectedElements().isEmpty());
    d->actionImportSelected->setEnabled(d->buttonImport->isEnabled());
    d->actionCopySelected->setEnabled(!d->resultList->selectedElements().isEmpty());
    d->actionViewCurrent->setEnabled(d->resultList->currentElement() != nullptr);
}

void SearchResults::importSelected()
{
    Q_ASSERT_X(d->mainEditor != nullptr, "SearchResults::importSelected", "d->mainEditor is NULL");

    FileModel *targetModel = d->mainEditor->fileModel();
    FileModel *sourceModel = d->resultList->fileModel();
    const QModelIndexList selList = d->resultList->selectionModel()->selectedRows();
    for (const QModelIndex &modelIndex : selList) {
        /// Map from visible row to 'real' row
        /// that may be hidden through sorting
        int row = d->resultList->sortFilterProxyModel()->mapToSource(modelIndex).row();
        /// Should only be an Entry,
        /// everthing else is unexpected
        QSharedPointer<Entry> entry = sourceModel->element(row).dynamicCast<Entry>();
        if (!entry.isNull()) {
            /// Important: make clone of entry before inserting
            /// in main list, otherwise data would be shared
            QSharedPointer<Entry> clone(new Entry(*entry));
            targetModel->insertRow(clone, targetModel->rowCount());
        } else
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Trying to import something that isn't an Entry";
    }

    if (!selList.isEmpty())
        d->mainEditor->externalModification();
}
