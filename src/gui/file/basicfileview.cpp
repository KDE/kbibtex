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

#include "basicfileview.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QKeyEvent>
#include <QAction>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "bibtexfields.h"
#include "filemodel.h"

class BasicFileView::Private
{
private:
    BasicFileView *p;
    const int storedColumnCount;

public:
    QString name;
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configHeaderState;

    FileModel *fileModel;
    QSortFilterProxyModel *sortFilterProxyModel;

    struct ColumnProperty {
        int width; /// the width of a column
        int visualIndex; /// if moved, in which column does it appear?
        bool isHidden; /// flag if column is hidden
    };
    struct HeaderProperty {
        int sumWidths; /// sum of all columns' widths
        int columnCount; /// number of columns
        ColumnProperty *columns; /// array of column properties
        int sortedColumn; /// the one column that is sorted
        Qt::SortOrder sortOrder; /// the sorted column's sort order
    } *headerProperty;

    Private(const QString &n, BasicFileView *parent)
            : p(parent), storedColumnCount(BibTeXFields::self()->count()), name(n),
          config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
          configGroupName(QLatin1String("BibliographyView")),
          configHeaderState(QLatin1String("HeaderState_%1")),
          fileModel(NULL), sortFilterProxyModel(NULL) {
        /// Allocate memory for headerProperty structure
        headerProperty = (struct HeaderProperty *)calloc(1, sizeof(struct HeaderProperty));
        headerProperty->columnCount = BibTeXFields::self()->count();
        headerProperty->columns = (struct ColumnProperty *)calloc(headerProperty->columnCount, sizeof(struct ColumnProperty));
        headerProperty->sortedColumn = -1;
        headerProperty->sortOrder = Qt::AscendingOrder;
    }

    ~Private() {
        updateHeaderProperties();
        saveHeaderProperties();
        /// Deallocate memory for headerProperty structure
        free(headerProperty->columns);
        free(headerProperty);
    }

    /**
     * Load values for headerProperty data structure from
     * fields' configuration
     */
    void resetHeaderProperties() {
        headerProperty->sumWidths = 0;
        headerProperty->sortedColumn = -1;
        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            headerProperty->columns[col].isHidden = !fd->defaultVisible;
            headerProperty->columns[col].width = fd->defaultWidth;
            headerProperty->columns[col].visualIndex = col;
            if (!headerProperty->columns[col].isHidden)
                headerProperty->sumWidths += fd->defaultWidth;
            ++col;
        }
        Q_ASSERT(col == headerProperty->columnCount);
        Q_ASSERT(headerProperty->sumWidths > 0);
    }

    void applyHeaderProperties() {
        Q_ASSERT(headerProperty->sumWidths > 0);
        const int widgetWidth = p->viewport()->size().width();
        if (widgetWidth < 8) return; ///< widget is too narrow or not yet initialized

        p->header()->blockSignals(true);

        headerProperty->sumWidths = 0;
        for (int col = 0; col < headerProperty->columnCount; ++col)
            headerProperty->sumWidths += headerProperty->columns[col].isHidden ? 0 : headerProperty->columns[col].width;

        for (int col = 0; col < headerProperty->columnCount; ++col) {
            p->setColumnHidden(col, headerProperty->columns[col].isHidden);
            p->setColumnWidth(col, headerProperty->columns[col].width * widgetWidth / headerProperty->sumWidths);
            const int fromVI = p->header()->visualIndex(col);
            /// Only initialized columns have a visual index (?)
            if (fromVI >= 0) {
                const int toVI = headerProperty->columns[col].visualIndex;
                if (fromVI != toVI)
                    p->header()->moveSection(fromVI, toVI);
            }

            foreach(QAction *action, p->header()->actions()) {
                bool ok = false;
                int ac = (int)action->data().toInt(&ok);
                if (ok && ac == col) {
                    action->setChecked(!headerProperty->columns[col].isHidden);
                    break;
                }
            }
        }

        p->header()->setSortIndicator(headerProperty->sortedColumn, headerProperty->sortOrder);

        p->header()->blockSignals(false);
    }

    void updateHeaderProperties() {
        headerProperty->sumWidths = 0;
        int countVisible = 0;
        for (int col = 0; col < headerProperty->columnCount; ++col) {
            headerProperty->columns[col].isHidden = p->isColumnHidden(col);
            headerProperty->columns[col].width = p->columnWidth(col);
            headerProperty->columns[col].visualIndex = p->header()->visualIndex(col);
            if (!headerProperty->columns[col].isHidden) {
                ++countVisible;
                headerProperty->sumWidths += headerProperty->columns[col].width;
            }
        }

        headerProperty->sortedColumn = p->header()->sortIndicatorSection();
        headerProperty->sortOrder = p->header()->sortIndicatorOrder();

        Q_ASSERT(headerProperty->sumWidths > 0);
        Q_ASSERT(countVisible > 0);
        const int hiddenColumnWidth = headerProperty->sumWidths / countVisible;
        for (int col = 0; col < headerProperty->columnCount; ++col)
            if (headerProperty->columns[col].isHidden)
                headerProperty->columns[col].width = hiddenColumnWidth;
    }

    void loadHeaderProperties() {
        KConfigGroup configGroup(config, configGroupName);
        headerProperty->sumWidths = 0;
        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            headerProperty->columns[col].isHidden = configGroup.readEntry(configHeaderState.arg(name).append(QString::number(col)).append(QLatin1String("IsHidden")), !fd->defaultVisible);
            headerProperty->columns[col].width = configGroup.readEntry(configHeaderState.arg(name).append(QString::number(col)).append(QLatin1String("Width")), fd->defaultWidth);
            headerProperty->columns[col].visualIndex = configGroup.readEntry(configHeaderState.arg(name).append(QString::number(col)).append(QLatin1String("VisualIndex")), col);
            if (!headerProperty->columns[col].isHidden)
                headerProperty->sumWidths += headerProperty->columns[col].width;
            ++col;
        }
        Q_ASSERT_X(headerProperty->sumWidths > 0, "BasicFileView::Private::loadHeaderProperties", "Sum of column widths over visible columns is zero.");

        headerProperty->sortedColumn = configGroup.readEntry(configHeaderState.arg(name).append(QLatin1String("SortedColumn")), -1);
        headerProperty->sortOrder = (Qt::SortOrder)configGroup.readEntry(configHeaderState.arg(name).append(QLatin1String("SortOrder")), (int)Qt::AscendingOrder);


        Q_ASSERT(headerProperty->sumWidths > 0);
    }

    void saveHeaderProperties() {
        KConfigGroup configGroup(config, configGroupName);
        for (int col = 0; col < headerProperty->columnCount; ++col) {
            configGroup.writeEntry(configHeaderState.arg(name).append(QString::number(col)).append(QLatin1String("IsHidden")), headerProperty->columns[col].isHidden);
            configGroup.writeEntry(configHeaderState.arg(name).append(QString::number(col)).append(QLatin1String("Width")), headerProperty->columns[col].width);
            configGroup.writeEntry(configHeaderState.arg(name).append(QString::number(col)).append(QLatin1String("VisualIndex")), headerProperty->columns[col].visualIndex);
        }

        configGroup.writeEntry(configHeaderState.arg(name).append(QLatin1String("SortedColumn")), headerProperty->sortedColumn);
        configGroup.writeEntry(configHeaderState.arg(name).append(QLatin1String("SortOrder")), (int)headerProperty->sortOrder);

        configGroup.sync();
    }

    void setColumnVisible(int column, bool isVisible) {
        if (headerProperty->columns[column].isHidden != isVisible)
            return; ///< nothing to do

        headerProperty->columns[column].isHidden = !isVisible;

        if (isVisible) {
            int countVisible = 0;
            headerProperty->sumWidths = 0;
            for (int col = 0; col < headerProperty->columnCount; ++col) {
                if (!headerProperty->columns[col].isHidden) {
                    ++countVisible;
                    headerProperty->sumWidths += headerProperty->columns[col].width;
                }
            }

            Q_ASSERT(headerProperty->sumWidths > 0);
            Q_ASSERT(countVisible > 0);
            const int hiddenColumnWidth = headerProperty->sumWidths / countVisible;
            headerProperty->columns[column].width = hiddenColumnWidth;
            headerProperty->sumWidths += hiddenColumnWidth;
        } else {
            /// Column becomes invisible
            headerProperty->sumWidths -= headerProperty->columns[column].width;
        }
        applyHeaderProperties();
    }
};

BasicFileView::BasicFileView(const QString &name, QWidget *parent)
        : QTreeView(parent), d(new Private(name, this))
{
    /// general visual appearance and behaviour
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);

    /// header appearance and behaviour
    header()->setClickable(true);
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(-1, Qt::AscendingOrder);
    connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sort(int,Qt::SortOrder)));
    header()->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(header(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(columnMoved()));
    connect(header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(columnResized(int,int,int)));

    /// build context menu for header to show/hide single columns
    int col = 0;
    foreach(const FieldDescription *fd, *BibTeXFields::self()) {
        QAction *action = new QAction(fd->label, header());
        action->setData(col);
        action->setCheckable(true);
        action->setChecked(!isColumnHidden(col));
        connect(action, SIGNAL(triggered()), this, SLOT(headerActionToggled()));
        header()->addAction(action);
        ++col;
    }

    /// add separator to header's context menu
    QAction *action = new QAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    /// add action to reset to defaults (regarding column visibility) to header's context menu
    action = new QAction(i18n("Reset to defaults"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(headerResetToDefaults()));
    header()->addAction(action);

    /// add separator to header's context menu
    action = new QAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    /// add action to disable any sorting
    action = new QAction(i18n("No sorting"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(noSorting()));
    header()->addAction(action);

    /// restore header appearance
    KConfigGroup configGroup(d->config, d->configGroupName);
    if (configGroup.hasKey(d->configHeaderState.arg(name).append(QLatin1String("1VisualIndex")))) {
        d->loadHeaderProperties();
    } else {
        d->resetHeaderProperties();
    }
    d->applyHeaderProperties();
}

BasicFileView::~BasicFileView()
{
    delete d;
}

void BasicFileView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    d->sortFilterProxyModel = NULL;
    d->fileModel = dynamic_cast<FileModel *>(model);
    if (d->fileModel == NULL) {
        d->sortFilterProxyModel = qobject_cast<QSortFilterProxyModel *>(model);
        Q_ASSERT_X(d->sortFilterProxyModel != NULL, "BasicFileView::setModel(QAbstractItemModel *model)", "d->sortFilterProxyModel is NULL");
        d->fileModel = dynamic_cast<FileModel *>(d->sortFilterProxyModel->sourceModel());
    }

    /// sort according to session
    if (header()->isSortIndicatorShown())
        sort(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

    Q_ASSERT_X(d->fileModel != NULL, "BasicFileView::setModel(QAbstractItemModel *model)", "d->fileModel is NULL");
}

FileModel *BasicFileView::fileModel()
{
    return d->fileModel;
}

QSortFilterProxyModel *BasicFileView::sortFilterProxyModel()
{
    return d->sortFilterProxyModel;
}

void BasicFileView::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::NoModifier) {
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && currentIndex() != QModelIndex()) {
            emit doubleClicked(currentIndex());
            event->accept();
        } else if (!event->text().isEmpty() && event->text().at(0).isLetterOrNumber()) {
            emit searchFor(event->text());
        }
    }
    QTreeView::keyPressEvent(event);
}

void BasicFileView::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    d->applyHeaderProperties();
}

void BasicFileView::columnMoved()
{
    d->updateHeaderProperties();
}

void BasicFileView::columnResized(int /*column*/, int /*oldSize*/, int /*newSize*/)
{
    d->updateHeaderProperties();
}

void BasicFileView::headerActionToggled()
{
    QAction *action = static_cast<QAction *>(sender());
    bool ok = false;
    const int col = (int)action->data().toInt(&ok);
    if (!ok) return;

    d->setColumnVisible(col, action->isChecked());
    d->applyHeaderProperties();
}

void BasicFileView::headerResetToDefaults()
{
    d->resetHeaderProperties();
    d->applyHeaderProperties();
}

void BasicFileView::sort(int t, Qt::SortOrder s)
{
    SortFilterFileModel *sortedModel = qobject_cast<SortFilterFileModel *>(model());
    if (sortedModel != NULL) {
        sortedModel->sort(t, s);
        /// Store sorting column and order in configuration data struct
        d->headerProperty->sortedColumn = header()->sortIndicatorSection();
        d->headerProperty->sortOrder = header()->sortIndicatorOrder();
    }
}

void BasicFileView::noSorting()
{
    SortFilterFileModel *sortedModel = qobject_cast<SortFilterFileModel *>(model());
    if (sortedModel != NULL) {
        sortedModel->sort(-1);
        header()->setSortIndicator(-1, Qt::AscendingOrder);
        /// Store sorting column and order in configuration data struct
        d->headerProperty->sortedColumn = header()->sortIndicatorSection();
        d->headerProperty->sortOrder = header()->sortIndicatorOrder();
    }
}
