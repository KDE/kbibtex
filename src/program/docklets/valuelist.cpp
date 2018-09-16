/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "valuelist.h"

#include <typeinfo>

#include <QTreeView>
#include <QHeaderView>
#include <QGridLayout>
#include <QStringListModel>
#include <QScrollBar>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QAction>

#include <KLineEdit>
#include <KComboBox>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KToggleAction>
#include <KSharedConfig>

#include "bibtexfields.h"
#include "entry.h"
#include "fileview.h"
#include "valuelistmodel.h"
#include "models/filemodel.h"

class ValueList::ValueListPrivate
{
private:
    ValueList *p;
    ValueListDelegate *delegate;

public:
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configKeyFieldName, configKeyShowCountColumn, configKeySortByCountAction, configKeyHeaderState;

    FileView *fileView;
    QTreeView *treeviewFieldValues;
    ValueListModel *model;
    QSortFilterProxyModel *sortingModel;
    KComboBox *comboboxFieldNames;
    KLineEdit *lineeditFilter;
    const int countWidth;
    QAction *assignSelectionAction;
    QAction *removeSelectionAction;
    KToggleAction *showCountColumnAction;
    KToggleAction *sortByCountAction;

    ValueListPrivate(ValueList *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("Value List Docklet")),
          configKeyFieldName(QStringLiteral("FieldName")), configKeyShowCountColumn(QStringLiteral("ShowCountColumn")),
          configKeySortByCountAction(QStringLiteral("SortByCountAction")), configKeyHeaderState(QStringLiteral("HeaderState")),
          fileView(nullptr), model(nullptr), sortingModel(nullptr), countWidth(8 + parent->fontMetrics().width(i18n("Count"))) {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QBoxLayout *layout = new QVBoxLayout(p);
        layout->setMargin(0);

        comboboxFieldNames = new KComboBox(true, p);
        layout->addWidget(comboboxFieldNames);

        lineeditFilter = new KLineEdit(p);
        layout->addWidget(lineeditFilter);
        lineeditFilter->setClearButtonEnabled(true);
        lineeditFilter->setPlaceholderText(i18n("Filter value list"));

        treeviewFieldValues = new QTreeView(p);
        layout->addWidget(treeviewFieldValues);
        treeviewFieldValues->setEditTriggers(QAbstractItemView::EditKeyPressed);
        treeviewFieldValues->setSortingEnabled(true);
        treeviewFieldValues->sortByColumn(0, Qt::AscendingOrder);
        delegate = new ValueListDelegate(treeviewFieldValues);
        treeviewFieldValues->setItemDelegate(delegate);
        treeviewFieldValues->setRootIsDecorated(false);
        treeviewFieldValues->setSelectionMode(QTreeView::ExtendedSelection);
        treeviewFieldValues->setAlternatingRowColors(true);
        treeviewFieldValues->header()->setSectionResizeMode(QHeaderView::Fixed);

        treeviewFieldValues->setContextMenuPolicy(Qt::ActionsContextMenu);
        /// create context menu item to start renaming
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("Replace all occurrences"), p);
        connect(action, &QAction::triggered, p, &ValueList::startItemRenaming);
        treeviewFieldValues->addAction(action);
        /// create context menu item to delete value
        action = new QAction(QIcon::fromTheme(QStringLiteral("edit-table-delete-row")), i18n("Delete all occurrences"), p);
        connect(action, &QAction::triggered, p, &ValueList::deleteAllOccurrences);
        treeviewFieldValues->addAction(action);
        /// create context menu item to search for multiple selections
        action = new QAction(QIcon::fromTheme(QStringLiteral("edit-find")), i18n("Search for selected values"), p);
        connect(action, &QAction::triggered, p, &ValueList::searchSelection);
        treeviewFieldValues->addAction(action);
        /// create context menu item to assign value to selected bibliography elements
        assignSelectionAction = new QAction(QIcon::fromTheme(QStringLiteral("emblem-new")), i18n("Add value to selected entries"), p);
        connect(assignSelectionAction, &QAction::triggered, p, &ValueList::assignSelection);
        treeviewFieldValues->addAction(assignSelectionAction);
        /// create context menu item to remove value from selected bibliography elements
        removeSelectionAction = new QAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove value from selected entries"), p);
        connect(removeSelectionAction, &QAction::triggered, p, &ValueList::removeSelection);
        treeviewFieldValues->addAction(removeSelectionAction);

        p->setEnabled(false);

        connect(comboboxFieldNames, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), p, &ValueList::fieldNamesChanged);
        connect(comboboxFieldNames, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), lineeditFilter, &KLineEdit::clear);
        connect(treeviewFieldValues, &QTreeView::activated, p, &ValueList::listItemActivated);
        connect(delegate, &ValueListDelegate::closeEditor, treeviewFieldValues, &QTreeView::reset);

        /// add context menu to header
        treeviewFieldValues->header()->setContextMenuPolicy(Qt::ActionsContextMenu);
        showCountColumnAction = new KToggleAction(i18n("Show Count Column"), treeviewFieldValues);
        connect(showCountColumnAction, &QAction::triggered, p, &ValueList::showCountColumnToggled);
        treeviewFieldValues->header()->addAction(showCountColumnAction);

        sortByCountAction = new KToggleAction(i18n("Sort by Count"), treeviewFieldValues);
        connect(sortByCountAction, &QAction::triggered, p, &ValueList::sortByCountToggled);
        treeviewFieldValues->header()->addAction(sortByCountAction);
    }

    void setComboboxFieldNamesCurrentItem(const QString &text) {
        int index = comboboxFieldNames->findData(text, Qt::UserRole, Qt::MatchExactly);
        if (index < 0) index = comboboxFieldNames->findData(text, Qt::UserRole, Qt::MatchStartsWith);
        if (index < 0) index = comboboxFieldNames->findData(text, Qt::UserRole, Qt::MatchContains);
        if (index >= 0) comboboxFieldNames->setCurrentIndex(index);
    }

    void initialize() {
        const BibTeXFields *bibtexFields = BibTeXFields::self();

        lineeditFilter->clear();
        comboboxFieldNames->clear();
        for (const auto &fd : const_cast<const BibTeXFields &>(*bibtexFields)) {
            if (!fd.upperCamelCaseAlt.isEmpty()) continue; /// keep only "single" fields and not combined ones like "Author or Editor"
            if (fd.upperCamelCase.startsWith('^')) continue; /// skip "type" and "id"
            comboboxFieldNames->addItem(fd.label, fd.upperCamelCase);
        }
        /// Sort the combo box locale-aware. Thus we need a SortFilterProxyModel
        QSortFilterProxyModel *proxy = new QSortFilterProxyModel(comboboxFieldNames);
        proxy->setSortLocaleAware(true);
        proxy->setSourceModel(comboboxFieldNames->model());
        comboboxFieldNames->model()->setParent(proxy);
        comboboxFieldNames->setModel(proxy);
        comboboxFieldNames->model()->sort(0);

        KConfigGroup configGroup(config, configGroupName);
        QString fieldName = configGroup.readEntry(configKeyFieldName, QString(Entry::ftAuthor));
        setComboboxFieldNamesCurrentItem(fieldName);
        if (allowsMultipleValues(fieldName))
            assignSelectionAction->setText(i18n("Add value to selected entries"));
        else
            assignSelectionAction->setText(i18n("Replace value of selected entries"));
        showCountColumnAction->setChecked(configGroup.readEntry(configKeyShowCountColumn, true));
        sortByCountAction->setChecked(configGroup.readEntry(configKeySortByCountAction, false));
        sortByCountAction->setEnabled(!showCountColumnAction->isChecked());
        QByteArray headerState = configGroup.readEntry(configKeyHeaderState, QByteArray());
        treeviewFieldValues->header()->restoreState(headerState);

        connect(treeviewFieldValues->header(), &QHeaderView::sortIndicatorChanged, p, &ValueList::columnsChanged);
    }

    void update() {
        QString text = comboboxFieldNames->itemData(comboboxFieldNames->currentIndex()).toString();
        if (text.isEmpty()) text = comboboxFieldNames->currentText();

        delegate->setFieldName(text);
        model = fileView == nullptr ? nullptr : fileView->valueListModel(text);
        QAbstractItemModel *usedModel = model;
        if (usedModel != nullptr) {
            model->setShowCountColumn(showCountColumnAction->isChecked());
            model->setSortBy(sortByCountAction->isChecked() ? ValueListModel::SortByCount : ValueListModel::SortByText);

            if (sortingModel != nullptr) delete sortingModel;
            sortingModel = new QSortFilterProxyModel(p);
            sortingModel->setSourceModel(model);
            if (treeviewFieldValues->header()->isSortIndicatorShown())
                sortingModel->sort(treeviewFieldValues->header()->sortIndicatorSection(), treeviewFieldValues->header()->sortIndicatorOrder());
            else
                sortingModel->sort(1, Qt::DescendingOrder);
            sortingModel->setSortRole(ValueListModel::SortRole);
            sortingModel->setFilterKeyColumn(0);
            sortingModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
            sortingModel->setFilterRole(ValueListModel::SearchTextRole);
            connect(lineeditFilter, &KLineEdit::textEdited, sortingModel, &QSortFilterProxyModel::setFilterFixedString);
            sortingModel->setSortLocaleAware(true);
            usedModel = sortingModel;
        }
        treeviewFieldValues->setModel(usedModel);

        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyFieldName, text);
        config->sync();
    }

    bool allowsMultipleValues(const QString &field) const {
        return (field.compare(Entry::ftAuthor, Qt::CaseInsensitive) == 0
                || field.compare(Entry::ftEditor, Qt::CaseInsensitive) == 0
                || field.compare(Entry::ftUrl, Qt::CaseInsensitive) == 0
                || field.compare(Entry::ftLocalFile, Qt::CaseInsensitive) == 0
                || field.compare(Entry::ftDOI, Qt::CaseInsensitive) == 0
                || field.compare(Entry::ftKeywords, Qt::CaseInsensitive) == 0);
    }
};

ValueList::ValueList(QWidget *parent)
        : QWidget(parent), d(new ValueListPrivate(this))
{
    QTimer::singleShot(500, this, &ValueList::delayedResize);
}

ValueList::~ValueList()
{
    delete d;
}

void ValueList::setFileView(FileView *fileView)
{
    if (d->fileView != nullptr)
        disconnect(d->fileView, &FileView::selectedElementsChanged, this, &ValueList::editorSelectionChanged);
    d->fileView = fileView;
    if (d->fileView != nullptr) {
        connect(d->fileView, &FileView::selectedElementsChanged, this, &ValueList::editorSelectionChanged);
        connect(d->fileView, &FileView::destroyed, this, &ValueList::editorDestroyed);
    }
    editorSelectionChanged();
    update();
    resizeEvent(nullptr);
}

void ValueList::update()
{
    d->update();
    setEnabled(d->fileView != nullptr);
}

void ValueList::resizeEvent(QResizeEvent *)
{
    int widgetWidth = d->treeviewFieldValues->size().width() - d->treeviewFieldValues->verticalScrollBar()->size().width() - 8;
    d->treeviewFieldValues->setColumnWidth(0, widgetWidth - d->countWidth);
    d->treeviewFieldValues->setColumnWidth(1, d->countWidth);
}

void ValueList::listItemActivated(const QModelIndex &index)
{
    setEnabled(false);
    QString itemText = d->sortingModel->mapToSource(index).data(ValueListModel::SearchTextRole).toString();
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterFileModel::FilterQuery fq;
    fq.terms << itemText;
    fq.combination = SortFilterFileModel::EveryTerm;
    fq.field = fieldText;
    fq.searchPDFfiles = false;

    d->fileView->setFilterBarFilter(fq);
    setEnabled(true);
}

void ValueList::searchSelection()
{
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterFileModel::FilterQuery fq;
    fq.combination = SortFilterFileModel::EveryTerm;
    fq.field = fieldText;
    const auto selectedIndexes = d->treeviewFieldValues->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : selectedIndexes) {
        if (index.column() == 0) {
            QString itemText = index.data(ValueListModel::SearchTextRole).toString();
            fq.terms << itemText;
        }
    }
    fq.searchPDFfiles = false;

    if (!fq.terms.isEmpty())
        d->fileView->setFilterBarFilter(fq);
}

void ValueList::assignSelection() {
    QString field = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex()).toString();
    if (field.isEmpty()) field = d->comboboxFieldNames->currentText();
    if (field.isEmpty()) return; ///< empty field is invalid; quit

    const Value toBeAssignedValue = d->sortingModel->mapToSource(d->treeviewFieldValues->currentIndex()).data(Qt::EditRole).value<Value>();
    if (toBeAssignedValue.isEmpty()) return; ///< empty value is invalid; quit
    const QString toBeAssignedValueText = PlainTextValue::text(toBeAssignedValue);

    /// Keep track if any modifications were made to the bibliography file
    bool madeModification = false;

    /// Go through all selected elements in current editor
    const QList<QSharedPointer<Element> > &selection = d->fileView->selectedElements();
    for (const auto &element : selection) {
        /// Only entries (not macros or comments) are of interest
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (!entry.isNull()) {
            /// Fields are separated into two categories:
            /// 1. Where more values can be appended, like authors or URLs
            /// 2. Where values should be replaced, like title, year, or journal
            if (d->allowsMultipleValues(field)) {
                /// Fields for which multiple values are valid
                bool valueItemAlreadyContained = false; ///< add only if to-be-assigned value is not yet contained
                Value entrysValueForField = entry->value(field);
                for (const auto &containedValueItem : const_cast<const Value &>(entrysValueForField)) {
                    valueItemAlreadyContained |= PlainTextValue::text(containedValueItem) == toBeAssignedValueText;
                    if (valueItemAlreadyContained) break;
                }

                if (!valueItemAlreadyContained) {
                    /// Add each ValueItem from the to-be-assigned value to the entry's value for this field
                    entrysValueForField.reserve(toBeAssignedValue.size());
                    for (const auto &newValueItem : toBeAssignedValue) {
                        entrysValueForField.append(newValueItem);
                    }
                    /// "Write back" value to field in entry
                    entry->remove(field);
                    entry->insert(field, entrysValueForField);
                    /// Keep track that bibliography file has been modified
                    madeModification = true;
                }
            } else {
                /// Fields for which only value is valid, thus the old value will be replaced
                entry->remove(field);
                entry->insert(field, toBeAssignedValue);
                /// Keep track that bibliography file has been modified
                madeModification = true;
            }

        }
    }

    if (madeModification) {
        /// Notify main editor about change it its data
        d->fileView->externalModification();
    }
}

void ValueList::removeSelection() {
    QString field = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex()).toString();
    if (field.isEmpty()) field = d->comboboxFieldNames->currentText();
    if (field.isEmpty()) return; ///< empty field is invalid; quit

    const Value toBeRemovedValue = d->sortingModel->mapToSource(d->treeviewFieldValues->currentIndex()).data(Qt::EditRole).value<Value>();
    if (toBeRemovedValue.isEmpty()) return; ///< empty value is invalid; quit
    const QString toBeRemovedValueText = PlainTextValue::text(toBeRemovedValue);

    /// Keep track if any modifications were made to the bibliography file
    bool madeModification = false;

    /// Go through all selected elements in current editor
    const QList<QSharedPointer<Element> > &selection = d->fileView->selectedElements();
    for (const auto &element : selection) {
        /// Only entries (not macros or comments) are of interest
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (!entry.isNull()) {
            Value entrysValueForField = entry->value(field);
            bool valueModified = false;
            for (int i = 0; i < entrysValueForField.count(); ++i) {
                const QString valueItemText = PlainTextValue::text(entrysValueForField[i]);
                if (valueItemText == toBeRemovedValueText) {
                    valueModified = true;
                    entrysValueForField.remove(i);
                    break;
                }
            }
            if (valueModified) {
                entry->remove(field);
                entry->insert(field, entrysValueForField);
                madeModification = true;
            }
        }
    }

    if (madeModification) {
        update();
        /// Notify main editor about change it its data
        d->fileView->externalModification();
    }
}

void ValueList::startItemRenaming()
{
    /// Get current index from sorted model
    QModelIndex sortedIndex = d->treeviewFieldValues->currentIndex();
    /// Make the tree view start and editing delegate on the index
    d->treeviewFieldValues->edit(sortedIndex);
}

void ValueList::deleteAllOccurrences()
{
    /// Get current index from sorted model
    QModelIndex sortedIndex = d->treeviewFieldValues->currentIndex();
    /// Get "real" index from original model, but resort to sibling in first column
    QModelIndex realIndex = d->sortingModel->mapToSource(sortedIndex);
    realIndex = realIndex.sibling(realIndex.row(), 0);

    /// Remove current index from data model
    d->model->removeValue(realIndex);
    /// Notify main editor about change it its data
    d->fileView->externalModification();
}

void ValueList::showCountColumnToggled()
{
    if (d->model != nullptr)
        d->model->setShowCountColumn(d->showCountColumnAction->isChecked());
    if (d->showCountColumnAction->isChecked())
        resizeEvent(nullptr);

    d->sortByCountAction->setEnabled(!d->showCountColumnAction->isChecked());

    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeyShowCountColumn, d->showCountColumnAction->isChecked());
    d->config->sync();
}

void ValueList::sortByCountToggled()
{
    if (d->model != nullptr)
        d->model->setSortBy(d->sortByCountAction->isChecked() ? ValueListModel::SortByCount : ValueListModel::SortByText);

    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeySortByCountAction, d->sortByCountAction->isChecked());
    d->config->sync();
}

void ValueList::delayedResize()
{
    resizeEvent(nullptr);
}

void ValueList::columnsChanged()
{
    QByteArray headerState = d->treeviewFieldValues->header()->saveState();
    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeyHeaderState, headerState);
    d->config->sync();

    resizeEvent(nullptr);
}

void ValueList::editorSelectionChanged() {
    const bool selectedElements = d->fileView == nullptr ? false : d->fileView->selectedElements().count() > 0;
    d->assignSelectionAction->setEnabled(selectedElements);
    d->removeSelectionAction->setEnabled(selectedElements);
}

void ValueList::editorDestroyed() {
    /// Reset internal variable to NULL to avoid
    /// accessing invalid pointer/data later
    d->fileView = nullptr;
    editorSelectionChanged();
}

void ValueList::fieldNamesChanged(int i) {
    const QString field = d->comboboxFieldNames->itemData(i).toString();
    if (d->allowsMultipleValues(field))
        d->assignSelectionAction->setText(i18n("Add value to selected entries"));
    else
        d->assignSelectionAction->setText(i18n("Replace value of selected entries"));
    update();
}
