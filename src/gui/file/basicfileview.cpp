/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <KLocalizedString>

#include <BibTeXFields>
#include <models/FileModel>
#include "file/sortfilterfilemodel.h"
#include "logging_gui.h"

class BasicFileView::Private
{
private:
    BasicFileView *p;
    const QString name;

public:

    FileModel *fileModel;
    QSortFilterProxyModel *sortFilterProxyModel;

    Private(const QString &n, BasicFileView *parent)
            : p(parent), name(n), fileModel(nullptr), sortFilterProxyModel(nullptr) {
        /// nothing
    }

    ~Private() {
        saveColumnProperties();
    }

    void balanceColumns() {
        if (p->header()->count() != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }
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

    void resetColumnProperties() {
        if (p->header()->count() != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }
        int col = 0;
        for (BibTeXFields::Iterator it = BibTeXFields::instance().begin(), endIt = BibTeXFields::instance().end(); it != endIt; ++it) {
            auto &fd = *it;
            fd.visible.remove(name);
            const bool visibility = fd.defaultVisible;
            p->header()->setSectionHidden(col, !visibility);
            ++col;
        }
        BibTeXFields::instance().save();
        balanceColumns();
    }

    void loadColumnProperties() {
        if (p->header()->count() != BibTeXFields::instance().count()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Number of columns in file view does not match number of bibliography fields:" << p->header()->count() << "!=" << BibTeXFields::instance().count();
            return;
        }
        int col = 0;
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
            const bool visibility = fd.visible.contains(name) ? fd.visible[name] : fd.defaultVisible;
            p->header()->setSectionHidden(col, !visibility);
            ++col;
        }
        balanceColumns();
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
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);

    /// header appearance and behaviour
    header()->setSectionsClickable(true);
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(-1, Qt::AscendingOrder);
    header()->setSectionsMovable(false);
    header()->setSectionResizeMode(QHeaderView::Fixed);
    connect(header(), &QHeaderView::sortIndicatorChanged, this, &BasicFileView::sort);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), &QHeaderView::customContextMenuRequested, this, &BasicFileView::showHeaderContextMenu);
}

BasicFileView::~BasicFileView()
{
    delete d;
}

void BasicFileView::setModel(QAbstractItemModel *model)
{
    if (d->fileModel != nullptr)
        disconnect(d->fileModel, &FileModel::headerDataChanged, this, &BasicFileView::headerResetToDefaults);

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
    else
        /// On header data change, e.g. triggered by change of bibliographic system, reset visibile columns
        connect(d->fileModel, &FileModel::headerDataChanged, this, &BasicFileView::headerResetToDefaults);

    QTreeView::setModel(model);

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
    Q_UNUSED(event)
    const int w = qMax(width() - 20, 0);
    header()->setMinimumWidth(w);
    header()->setMaximumWidth(w);
    d->balanceColumns();
}

void BasicFileView::headerActionToggled()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == nullptr) return;
    bool ok = false;
    const int col = action->data().toInt(&ok);
    if (!ok) return;

    header()->setSectionHidden(col, !header()->isSectionHidden(col));
    d->balanceColumns();
}

void BasicFileView::headerResetToDefaults()
{
    d->resetColumnProperties();
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
    QMenu menu;

    int col = 0;
    for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
        QAction *action = new QAction(fd.label, &menu);
        action->setData(col);
        action->setCheckable(true);
        action->setChecked(!header()->isSectionHidden(col));
        connect(action, &QAction::triggered, this, &BasicFileView::headerActionToggled);
        menu.addAction(action);
        ++col;
    }

    /// Add separator to header's context menu
    QAction *action = new QAction(&menu);
    action->setSeparator(true);
    menu.addAction(action);

    /// Add action to reset to defaults (regarding column visibility) to header's context menu
    action = new QAction(i18n("Reset to defaults"), &menu);
    connect(action, &QAction::triggered, this, &BasicFileView::headerResetToDefaults);
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
