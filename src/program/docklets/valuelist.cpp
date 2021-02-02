/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QLineEdit>
#include <QComboBox>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QAction>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KToggleAction>
#include <KSharedConfig>

#include <BibTeXFields>
#include <Entry>
#include <file/FileView>
#include <ValueListModel>
#include <models/FileModel>

class ValueList::Private
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
    QComboBox *comboboxFieldNames;
    QLineEdit *lineeditFilter;
    const int countWidth;
    KToggleAction *showCountColumnAction;
    KToggleAction *sortByCountAction;

    Private(ValueList *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("Value List Docklet")),
          configKeyFieldName(QStringLiteral("FieldName")), configKeyShowCountColumn(QStringLiteral("ShowCountColumn")),
          configKeySortByCountAction(QStringLiteral("SortByCountAction")), configKeyHeaderState(QStringLiteral("HeaderState")),
          fileView(nullptr), model(nullptr), sortingModel(nullptr),
#if QT_VERSION >= 0x050b00
          countWidth(8 + parent->fontMetrics().horizontalAdvance(i18n("Count")))
#else // QT_VERSION >= 0x050b00
          countWidth(8 + parent->fontMetrics().width(i18n("Count")))
#endif // QT_VERSION >= 0x050b00
    {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QBoxLayout *layout = new QVBoxLayout(p);
        layout->setMargin(0);

        comboboxFieldNames = new QComboBox(p);
        comboboxFieldNames->setEditable(true);
        layout->addWidget(comboboxFieldNames);

        lineeditFilter = new QLineEdit(p);
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
        treeviewFieldValues->setSelectionMode(QTreeView::SingleSelection);
        treeviewFieldValues->setAlternatingRowColors(true);
        treeviewFieldValues->header()->setSectionResizeMode(QHeaderView::Fixed);

        treeviewFieldValues->setContextMenuPolicy(Qt::ActionsContextMenu);
        // Create context menu item to start renaming the current value in all bibliography entries
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("Edit to replace all occurrences"), p);
        connect(action, &QAction::triggered, p, &ValueList::listItemStartRenaming);
        treeviewFieldValues->addAction(action);
        // Create context menu item to remove current value from all bibliography entries
        action = new QAction(QIcon::fromTheme(QStringLiteral("edit-table-delete-row")), i18n("Remove all occurrences"), p);
        connect(action, &QAction::triggered, p, &ValueList::deleteAllOccurrences);
        treeviewFieldValues->addAction(action);

        p->setEnabled(false);

        connect(comboboxFieldNames, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), p, &ValueList::update);
        connect(comboboxFieldNames, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), lineeditFilter, &QLineEdit::clear);
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
        lineeditFilter->clear();
        comboboxFieldNames->clear();
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
            if (!fd.upperCamelCaseAlt.isEmpty()) continue; /// keep only "single" fields and not combined ones like "Author or Editor"
            if (fd.upperCamelCase.startsWith(QLatin1Char('^'))) continue; /// skip "type" and "id" (those are marked with '^')
            comboboxFieldNames->addItem(fd.label, fd.upperCamelCase);
        }
        // Sort the combo box locale-aware. Thus we need a SortFilterProxyModel
        QSortFilterProxyModel *proxy = new QSortFilterProxyModel(comboboxFieldNames);
        proxy->setSortLocaleAware(true);
        proxy->setSourceModel(comboboxFieldNames->model());
        comboboxFieldNames->model()->setParent(proxy);
        comboboxFieldNames->setModel(proxy);
        comboboxFieldNames->model()->sort(0);

        KConfigGroup configGroup(config, configGroupName);
        const QString fieldName = configGroup.readEntry(configKeyFieldName, QString(Entry::ftAuthor));
        setComboboxFieldNamesCurrentItem(fieldName);
        showCountColumnAction->setChecked(configGroup.readEntry(configKeyShowCountColumn, true));
        sortByCountAction->setChecked(configGroup.readEntry(configKeySortByCountAction, false));
        sortByCountAction->setEnabled(!showCountColumnAction->isChecked());
        QByteArray headerState = configGroup.readEntry(configKeyHeaderState, QByteArray());
        treeviewFieldValues->header()->restoreState(headerState);

        connect(treeviewFieldValues->header(), &QHeaderView::sortIndicatorChanged, p, &ValueList::columnsChanged);
    }

    void update() {
        const QString fieldName = currentField();

        delegate->setFieldName(fieldName);
        model = fileView == nullptr ? nullptr : fileView->valueListModel(fieldName);
        QAbstractItemModel *usedModel = model;
        if (usedModel != nullptr) {
            model->setShowCountColumn(showCountColumnAction->isChecked());
            model->setSortBy(sortByCountAction->isChecked() ? ValueListModel::SortBy::Count : ValueListModel::SortBy::Text);

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
            connect(lineeditFilter, &QLineEdit::textEdited, sortingModel, &QSortFilterProxyModel::setFilterFixedString);
            sortingModel->setSortLocaleAware(true);
            usedModel = sortingModel;
        }
        treeviewFieldValues->setModel(usedModel);

        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyFieldName, fieldName);
        config->sync();
    }

    QString currentField() const {
        QString field = comboboxFieldNames->itemData(comboboxFieldNames->currentIndex()).toString();
        if (field.isEmpty()) field = comboboxFieldNames->currentText();
        return field;
    }
};

ValueList::ValueList(QWidget *parent)
        : QWidget(parent), d(new Private(this))
{
    QTimer::singleShot(500, this, [this]() {
        resizeEvent(nullptr);
    });
}

ValueList::~ValueList()
{
    delete d;
}

void ValueList::setFileView(FileView *fileView)
{
    if (d->fileView != nullptr)
        disconnect(d->fileView, &FileView::destroyed, this, &ValueList::editorDestroyed);
    d->fileView = fileView;
    if (d->fileView != nullptr)
        connect(d->fileView, &FileView::destroyed, this, &ValueList::editorDestroyed);
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
    const QString itemText = d->sortingModel->mapToSource(index).data(ValueListModel::SearchTextRole).toString();
    if (itemText.isEmpty())
        return;

    SortFilterFileModel::FilterQuery fq;
    fq.terms = QStringList{itemText};
    fq.combination = SortFilterFileModel::FilterCombination::EveryTerm;
    fq.field = d->currentField();
    if (fq.field.isEmpty())
        return;
    fq.searchPDFfiles = false;

    setEnabled(false); // FIXME necesary?
    d->fileView->setFilterBarFilter(fq);
    setEnabled(true); // FIXME necesary?
}

void ValueList::listItemStartRenaming()
{
    // Get current index from sorted model
    const QModelIndex sortedIndex = d->treeviewFieldValues->currentIndex();
    // Make the tree view start and editing delegate on the index
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
        d->model->setSortBy(d->sortByCountAction->isChecked() ? ValueListModel::SortBy::Count : ValueListModel::SortBy::Text);

    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeySortByCountAction, d->sortByCountAction->isChecked());
    d->config->sync();
}

void ValueList::columnsChanged()
{
    QByteArray headerState = d->treeviewFieldValues->header()->saveState();
    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeyHeaderState, headerState);
    d->config->sync();

    resizeEvent(nullptr);
}

void ValueList::editorDestroyed() {
    /// Reset internal variable to NULL to avoid
    /// accessing invalid pointer/data later
    d->fileView = nullptr;
}
