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

#include <typeinfo>

#include <QTreeView>
#include <QHeaderView>
#include <QGridLayout>
#include <QStringListModel>
#include <QScrollBar>
#include <QTimer>
#include <QSortFilterProxyModel>

#include <KComboBox>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KLocale>
#include <KAction>

#include <bibtexfields.h>
#include <entry.h>
#include <bibtexeditor.h>
#include <valuelistmodel.h>
#include "valuelist.h"

class ValueList::ValueListPrivate
{
private:
    ValueList *p;
    ValueListDelegate *delegate;

public:
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configKeyName;

    BibTeXEditor *editor;
    QTreeView *treeviewFieldValues;
    QSortFilterProxyModel *sortingModel;
    KComboBox *comboboxFieldNames;
    const int countWidth;

    ValueListPrivate(ValueList *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("Value List Docklet")),
            configKeyName(QLatin1String("FieldName")), sortingModel(NULL), countWidth(parent->fontMetrics().width(i18n("Count"))) {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        comboboxFieldNames = new KComboBox(true, p);
        layout->addWidget(comboboxFieldNames, 0, 0, 1, 1);

        treeviewFieldValues = new QTreeView(p);
        layout->addWidget(treeviewFieldValues, 1, 0, 1, 1);
        treeviewFieldValues->setEditTriggers(QAbstractItemView::EditKeyPressed);
        treeviewFieldValues->setSortingEnabled(true);
        delegate = new ValueListDelegate(treeviewFieldValues);
        treeviewFieldValues->setItemDelegate(delegate);
        treeviewFieldValues->setRootIsDecorated(false);

        /// create context menu to start renaming
        treeviewFieldValues->setContextMenuPolicy(Qt::ActionsContextMenu);
        KAction *action = new KAction(KIcon("edit-rename"), i18n("Replace all occurrences"), p);
        connect(action, SIGNAL(triggered()), p, SLOT(startItemRenaming()));
        treeviewFieldValues->addAction(action);

        p->setEnabled(false);

        connect(comboboxFieldNames, SIGNAL(activated(int)), p, SLOT(update()));
        connect(treeviewFieldValues, SIGNAL(activated(const QModelIndex &)), p, SLOT(listItemActivated(const QModelIndex &)));
    }

    void initialize() {
        const BibTeXFields *bibtexFields = BibTeXFields::self();

        comboboxFieldNames->clear();
        foreach(const FieldDescription &fd, *bibtexFields) {
            if (!fd.upperCamelCaseAlt.isEmpty()) continue; /// keep only "single" fields and not combined ones like "Author or Editor"
            if (fd.upperCamelCase.startsWith('^')) continue; /// skip "type" and "id"
            comboboxFieldNames->addItem(fd.label, fd.upperCamelCase);
        }

        KConfigGroup configGroup(config, configGroupName);
        QString fieldName = configGroup.readEntry(configKeyName, QString());
        comboboxFieldNames->setCurrentItem(fieldName);
    }

    void update() {
        QVariant var = comboboxFieldNames->itemData(comboboxFieldNames->currentIndex());
        QString text = var.toString();
        if (text.isEmpty()) text = comboboxFieldNames->currentText();

        delegate->setFieldName(text);
        QAbstractItemModel *model = editor == NULL ? NULL : editor->valueListModel(text);
        if (model != NULL) {
            if (sortingModel != NULL) delete sortingModel;
            sortingModel = new QSortFilterProxyModel(p);
            sortingModel->setSourceModel(model);
            sortingModel->sort(1, Qt::DescendingOrder);
            sortingModel->setSortRole(SortRole);
            model = sortingModel;
        }
        treeviewFieldValues->setModel(model);
        treeviewFieldValues->header()->setResizeMode(QHeaderView::Fixed);

        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyName, text);
        config->sync();
    }
};

ValueList::ValueList(QWidget *parent)
        : QWidget(parent), d(new ValueListPrivate(this))
{
}

void ValueList::setEditor(BibTeXEditor *editor)
{
    d->editor = editor;
    update();
}

void ValueList::update()
{
    d->update();
    setEnabled(d->editor != NULL);
    QTimer::singleShot(100, this, SLOT(resizeEvent()));
}

void ValueList::resizeEvent(QResizeEvent *)
{
    int widgetWidth = d->treeviewFieldValues->size().width() - d->treeviewFieldValues->verticalScrollBar()->size().width() - 8;
    d->treeviewFieldValues->setColumnWidth(0, widgetWidth - d->countWidth);
    d->treeviewFieldValues->setColumnWidth(1, d->countWidth);
}

void ValueList::listItemActivated(const QModelIndex &index)
{
    QString itemText = d->sortingModel->mapToSource(index).data(SearchTextRole).toString();
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterBibTeXFileModel::FilterQuery fq;
    fq.terms << itemText;
    fq.combination = SortFilterBibTeXFileModel::EveryTerm;
    fq.field = fieldText;

    d->editor->setFilterBarFilter(fq);
}

void ValueList::startItemRenaming()
{
    int row = d->treeviewFieldValues->selectionModel()->selectedIndexes().first().row();
    QModelIndex index = d->treeviewFieldValues->model()->index(row, 0);
    d->treeviewFieldValues->edit(index);
}
