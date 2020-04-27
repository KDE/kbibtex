/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "basicfileview.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QKeyEvent>
#include <QAction>
#include <QMenu>
#include <QTimer>

#include <KLocalizedString>

#include <BibTeXFields>
#include <models/FileModel>
#include "file/sortfilterfilemodel.h"
#include "logging_gui.h"

class BasicFileView::Private
{
private:
    BasicFileView *p;

public:
    const QString name;
    bool automaticBalancing;
    FileModel *fileModel;
    QSortFilterProxyModel *sortFilterProxyModel;

    Private(const QString &n, BasicFileView *parent)
            : p(parent), name(n), automaticBalancing(true), fileModel(nullptr), sortFilterProxyModel(nullptr) {
        /// nothing
    }

    ~Private() {
        saveColumnProperties();
    }

    void balanceColumns() {
        QSignalBlocker headerViewSignalBlocker(p->header());

        if (p->header()->count() != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        } else if (!automaticBalancing) {
            qCWarning(LOG_KBIBTEX_GUI) << "Will not automaticlly balance columns if automatic balancing is disabled";
            return;
        }

        /// Automatic balancing of columns is enabled
        int defaultWidthSumVisible = 0;
        int col = 0;
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
            if (!p->header()->isSectionHidden(col))
                defaultWidthSumVisible += fd.defaultWidth;
            ++col;
        }

        if (defaultWidthSumVisible == 0) return;

        col = 0;
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
            if (!p->header()->isSectionHidden(col))
                p->header()->resizeSection(col, p->header()->width() * fd.defaultWidth / defaultWidthSumVisible);
            ++col;
        }
    }

    void enableAutomaticBalancing()
    {
        automaticBalancing = true;
        p->header()->setSectionsMovable(false);
        p->header()->setSectionResizeMode(QHeaderView::Fixed);
        int col = 0;
        for (BibTeXFields::Iterator it = BibTeXFields::instance().begin(), endIt = BibTeXFields::instance().end(); it != endIt; ++it) {
            auto &fd = *it;
            fd.visible.remove(name);
            const bool visibility = fd.defaultVisible;
            p->header()->setSectionHidden(col, !visibility);

            /// Later, when loading config, Width of 0 for all fields means 'enable auto-balancing'
            fd.width[name] = 0;

            /// Move columns in their original order, i.e. visual index == logical index
            const int vi = p->header()->visualIndex(col);
            fd.visualIndex[name] = col;
            if (vi != col) p->header()->moveSection(vi, col);

            ++col;
        }

        balanceColumns();
    }

    enum class ColumnSizingOrigin { FromCurrentLayout, FromStoredSettings };

    void enableManualColumnSizing(const ColumnSizingOrigin &columnSizingOrigin)
    {
        const int columnCount = p->header()->count();
        if (columnCount != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }

        automaticBalancing = false;

        p->header()->setSectionsMovable(true);
        p->header()->setSectionResizeMode(QHeaderView::Interactive);

        if (columnSizingOrigin == ColumnSizingOrigin::FromCurrentLayout) {
            /// Memorize each columns current width (for future reference)
            for (int logicalIndex = 0; logicalIndex < columnCount; ++logicalIndex)
                BibTeXFields::instance()[logicalIndex].width[name] = p->header()->sectionSize(logicalIndex);
        } else if (columnSizingOrigin == ColumnSizingOrigin::FromStoredSettings) {
            QSignalBlocker headerViewSignalBlocker(p->header());

            /// Manual columns widths are to be restored
            int col = 0;
            for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
                /// Take column width from stored settings, but as a fall-back/default value
                /// take the default width (usually <10) and multiply it by the average character
                /// width and 4 (magic constant)
                p->header()->resizeSection(col, fd.width.value(name, fd.defaultWidth * 4 * p->fontMetrics().averageCharWidth()));
                ++col;
            }
        }
    }

    void resetColumnProperties() {
        enableAutomaticBalancing();
        BibTeXFields::instance().save();
    }

    void loadColumnProperties() {
        const int columnCount = p->header()->count();
        if (columnCount != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }

        QSignalBlocker headerViewSignalBlocker(p->header());

        if (p->header()->count() != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }
        int col = 0;
        automaticBalancing = true;
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
            const bool visibility = fd.visible.value(name, fd.defaultVisible);
            /// Width of 0 for all fields means 'enable auto-balancing'
            automaticBalancing &= fd.width.value(name, 0) == 0;
            p->header()->setSectionHidden(col, !visibility);
            ++col;
        }
        if (automaticBalancing)
            enableAutomaticBalancing();
        else {
            enableManualColumnSizing(ColumnSizingOrigin::FromStoredSettings);

            /// In case no automatic balancing was set, move columns to their positions from previous session
            /// (columns' widths got already restored in 'enableManualColumnSizingPositioning')
            col = 0;
            QHash<int, int> moveTarget;
            for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
                moveTarget[fd.visualIndex[name]] = col;
                ++col;
            }
            for (int visualIndex = 0; visualIndex < columnCount; ++visualIndex)
                if (moveTarget[visualIndex] != visualIndex) p->header()->moveSection(moveTarget[visualIndex], visualIndex);
        }
    }

    void saveColumnProperties() {
        if (p->header()->count() != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }
        int col = 0;
        for (BibTeXFields::Iterator it = BibTeXFields::instance().begin(), endIt = BibTeXFields::instance().end(); it != endIt; ++it) {
            auto &fd = *it;
            fd.visible[name] = !p->header()->isSectionHidden(col);
            /// Width of 0 for all fields means 'enable auto-balancing'
            fd.width[name] = automaticBalancing ? 0 : p->header()->sectionSize(col);
            fd.visualIndex[name] = automaticBalancing ? col : p->header()->visualIndex(col);
            ++col;
        }
        BibTeXFields::instance().save();
    }
};

BasicFileView::BasicFileView(const QString &name, QWidget *parent)
        : QTreeView(parent), d(new Private(name, this))
{
    /// general visual appearance and behaviour
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);

    /// header appearance and behaviour
    header()->setSectionsClickable(true);
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(-1, Qt::AscendingOrder);
    connect(header(), &QHeaderView::sortIndicatorChanged, this, &BasicFileView::sort);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), &QHeaderView::customContextMenuRequested, this, &BasicFileView::showHeaderContextMenu);
    QTimer::singleShot(250, this, [this]() {
        /// Delayed initialization to prevent early triggering of the following slot
        connect(header(), &QHeaderView::sectionResized, this, [this](int logicalIndex, int oldSize, int newSize) {
            Q_UNUSED(oldSize)
            if (!d->automaticBalancing && !d->name.isEmpty() && newSize > 0 && logicalIndex >= 0 && logicalIndex < BibTeXFields::instance().count()) {
                BibTeXFields::instance()[logicalIndex].width[d->name] = newSize;
            }
        });
    });
}

BasicFileView::~BasicFileView()
{
    delete d;
}

void BasicFileView::setModel(QAbstractItemModel *model)
{
    d->sortFilterProxyModel = nullptr;
    d->fileModel = dynamic_cast<FileModel *>(model);
    if (d->fileModel == nullptr) {
        d->sortFilterProxyModel = qobject_cast<QSortFilterProxyModel *>(model);
        if (d->sortFilterProxyModel == nullptr)
            qCWarning(LOG_KBIBTEX_GUI) << "Failed to dynamically cast model to QSortFilterProxyModel*";
        else
            d->fileModel = dynamic_cast<FileModel *>(d->sortFilterProxyModel->sourceModel());
    }
    if (d->fileModel == nullptr)
        qCWarning(LOG_KBIBTEX_GUI) << "Failed to dynamically cast model to FileModel*";

    QTreeView::setModel(model);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &selected, const QItemSelection &) {
        const bool hasSelection = !selected.empty();
        emit this->hasSelectionChanged(hasSelection);
    });

    /// sort according to session
    if (header()->isSortIndicatorShown())
        sort(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

    d->loadColumnProperties();
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
            event->accept();
        }
    }
    QTreeView::keyPressEvent(event);
}

void BasicFileView::resizeEvent(QResizeEvent *event) {
    const int w = qMax(event->size().width() - verticalScrollBar()->width(), 0);
    header()->setMinimumWidth(w);
    if (d->automaticBalancing) {
        header()->setMaximumWidth(w);
        d->balanceColumns();
    } else
        header()->setMaximumWidth(w * 3);
}

void BasicFileView::headerColumnVisibilityToggled()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == nullptr) return;
    bool ok = false;
    const int col = action->data().toInt(&ok);
    if (!ok) return;

    if (header()->hiddenSectionCount() + 1 >= header()->count() && !header()->isSectionHidden(col)) {
        /// If only one last column is visible and the current action likes to hide
        /// this column, abort so that the column cannot be hidden by the user
        qWarning() << "Already too many columns hidden, won't hide more";
        return;
    }

    header()->setSectionHidden(col, !header()->isSectionHidden(col));
    if (d->automaticBalancing)
        d->balanceColumns();
}

void BasicFileView::sort(int t, Qt::SortOrder s)
{
    SortFilterFileModel *sortedModel = qobject_cast<SortFilterFileModel *>(model());
    if (sortedModel != nullptr)
        sortedModel->sort(t, s);
}

void BasicFileView::noSorting()
{
    SortFilterFileModel *sortedModel = qobject_cast<SortFilterFileModel *>(model());
    if (sortedModel != nullptr) {
        sortedModel->sort(-1);
        header()->setSortIndicator(-1, Qt::AscendingOrder);
    }
}

void BasicFileView::showHeaderContextMenu(const QPoint &pos)
{
    const QPoint globalPos = viewport()->mapToGlobal(pos);
    QMenu menu(this);

    int col = 0;
    const bool onlyOneLastColumnVisible = header()->hiddenSectionCount() + 1 >= header()->count();
    for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
        QAction *action = new QAction(fd.label, &menu);
        action->setData(col);
        action->setCheckable(true);
        action->setChecked(!header()->isSectionHidden(col));
        if (onlyOneLastColumnVisible && action->isChecked()) {
            /// If only one last column is visible and the current field is this column,
            /// disable action so that the column cannot be hidden by the user
            action->setEnabled(false);
        }
        connect(action, &QAction::triggered, this, &BasicFileView::headerColumnVisibilityToggled);
        menu.addAction(action);
        ++col;
    }

    /// Add separator to header's context menu
    QAction *action = new QAction(&menu);
    action->setSeparator(true);
    menu.addAction(action);

    /// Add action to reset to defaults (regarding column visibility) to header's context menu
    action = new QAction(i18n("Reset to defaults"), &menu);
    connect(action, &QAction::triggered, this, [this]() {
        d->resetColumnProperties();
    });
    menu.addAction(action);

    /// Add action to allow manual resizing of columns
    action = new QAction(i18n("Allow manual column resizing/positioning"), &menu);
    action->setCheckable(true);
    action->setChecked(!d->automaticBalancing);
    connect(action, &QAction::triggered, this, [this, action]() {
        if (action->isChecked())
            d->enableManualColumnSizing(BasicFileView::Private::ColumnSizingOrigin::FromCurrentLayout);
        else
            d->enableAutomaticBalancing();
    });
    menu.addAction(action);

    /// Add separator to header's context menu
    action = new QAction(&menu);
    action->setSeparator(true);
    menu.addAction(action);

    /// Add action to disable any sorting
    action = new QAction(i18n("No sorting"), &menu);
    connect(action, &QAction::triggered, this, &BasicFileView::noSorting);
    menu.addAction(action);

    menu.exec(globalPos);
}
