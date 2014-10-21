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

#include <KAction>
#include <KConfigGroup>
#include <KLocale>
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

    Private(const QString &n, BasicFileView *parent)
            : p(parent), storedColumnCount(BibTeXFields::self()->count()), name(n), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("ReadOnlyFileView")), configHeaderState(QLatin1String("HeaderState_%1")) {
        // nothing
    }


    void resetColumnsToDefault() {
        int widgetWidth = p->viewport()->size().width();
        if (widgetWidth < 8) return; ///< widget is too narrow or not yet initialized

        disconnect(p->header(), SIGNAL(sectionResized(int,int,int)), p, SLOT(columnResized(int,int,int)));
        disconnect(p->header(), SIGNAL(sectionMoved(int,int,int)), p, SLOT(columnMoved()));

        int sum = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            if (fd->defaultVisible)
                sum += fd->defaultWidth;
        }
        Q_ASSERT_X(sum > 0, "BasicFileView::Private::resetColumnsToDefault", "Sum of default widths over columns visible by default is zero.");

        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            foreach(QAction *action, p->header()->actions()) {
                bool ok = false;
                int ac = (int)action->data().toInt(&ok);
                if (ok && ac == col)
                    action->setChecked(fd->defaultVisible);
            }

            p->setColumnHidden(col, !fd->defaultVisible);
            p->setColumnWidth(col, fd->defaultWidth * widgetWidth / sum);
            ++col;
        }

        for (int i = 0; i < storedColumnCount; ++i) {
            int j = p->header()->visualIndex(i);
            if (i != j)
                p->header()->moveSection(j, i);
        }

        p->columnResized(0, 0, 0); ///< QTreeView seems to be lazy on updating itself, force updated

        QByteArray headerState = p->header()->saveState();
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configHeaderState.arg(name), headerState);
        config->sync();

        connect(p->header(), SIGNAL(sectionMoved(int,int,int)), p, SLOT(columnMoved()));
        connect(p->header(), SIGNAL(sectionResized(int,int,int)), p, SLOT(columnResized(int,int,int)));
    }

    void storeColumns() {
        QByteArray headerState = p->header()->saveState();
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configHeaderState.arg(name), headerState);
        config->sync();
    }

    void adjustColumns() {
        int widgetWidth = p->viewport()->size().width();
        if (widgetWidth < 8) return; ///< widget is too narrow or not yet initialized

        disconnect(p->header(), SIGNAL(sectionMoved(int,int,int)), p, SLOT(columnMoved()));
        disconnect(p->header(), SIGNAL(sectionResized(int,int,int)), p, SLOT(columnResized(int,int,int)));

        const int columnCount = p->header()->count();
        int *columnWidths = new int[columnCount];
        int sum = 0;
        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            columnWidths[col] = qMax(p->columnWidth(col), fd->defaultWidth);
            if (!p->isColumnHidden(col))
                sum += columnWidths[col];
            ++col;
        }
        Q_ASSERT_X(sum > 0, "BasicFileView::Private::adjustColumns", "Sum of column widths over visible columns is zero.");

        for (int col = storedColumnCount - 1; col >= 0; --col) {
            p->setColumnWidth(col, columnWidths[col] * widgetWidth / sum);
        }
        p->columnResized(0, 0, 0); ///< QTreeView seems to be lazy on updating itself, force updated

        QByteArray headerState = p->header()->saveState();
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configHeaderState.arg(name), headerState);
        config->sync();

        delete[] columnWidths;

        connect(p->header(), SIGNAL(sectionMoved(int,int,int)), p, SLOT(columnMoved()));
        connect(p->header(), SIGNAL(sectionResized(int,int,int)), p, SLOT(columnResized(int,int,int)));
    }

    void setColumnVisible(int column, bool isVisible) {
        disconnect(p->header(), SIGNAL(sectionMoved(int,int,int)), p, SLOT(columnMoved()));
        disconnect(p->header(), SIGNAL(sectionResized(int,int,int)), p, SLOT(columnResized(int,int,int)));

        if (isVisible) {
            static const int colMinWidth = p->fontMetrics().width(QChar('W')) * 5;
            int widgetWidth = p->viewport()->size().width();
            p->setColumnWidth(column, qMax(widgetWidth / 10, colMinWidth));
        }
        p->setColumnHidden(column, !isVisible);

        connect(p->header(), SIGNAL(sectionMoved(int,int,int)), p, SLOT(columnMoved()));
        connect(p->header(), SIGNAL(sectionResized(int,int,int)), p, SLOT(columnResized(int,int,int)));
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

    header()->setStretchLastSection(false);

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
        KAction *action = new KAction(fd->label, header());
        action->setData(col);
        action->setCheckable(true);
        action->setChecked(!isColumnHidden(col));
        connect(action, SIGNAL(triggered()), this, SLOT(headerActionToggled()));
        header()->addAction(action);
        ++col;
    }

    /// add separator to header's context menu
    KAction *action = new KAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    /// adjust column widths
    action = new KAction(i18n("Adjust Column Widths"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(headerAdjustColumnWidths()));
    header()->addAction(action);

    /// add action to reset to defaults (regarding column visibility) to header's context menu
    action = new KAction(i18n("Reset to defaults"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(headerResetToDefaults()));
    header()->addAction(action);

    /// add separator to header's context menu
    action = new KAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    /// add action to disable any sorting
    action = new KAction(i18n("No sorting"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(noSorting()));
    header()->addAction(action);

    /// restore header appearance
    KConfigGroup configGroup(d->config, d->configGroupName);
    QByteArray headerState = configGroup.readEntry(d->configHeaderState.arg(d->name), QByteArray());
    if (headerState.isEmpty())
        d->resetColumnsToDefault();
    else {
        header()->restoreState(headerState);
        d->storeColumns();
    }
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
        Q_ASSERT_X(d->sortFilterProxyModel != NULL, "ReadOnlyFileView::setModel(QAbstractItemModel *model)", "d->sortFilterProxyModel is NULL");
        d->fileModel = dynamic_cast<FileModel *>(d->sortFilterProxyModel->sourceModel());
    }

    /// sort according to session
    if (header()->isSortIndicatorShown())
        sort(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

    Q_ASSERT_X(d->fileModel != NULL, "ReadOnlyFileView::setModel(QAbstractItemModel *model)", "d->fileModel is NULL");
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

void BasicFileView::columnMoved()
{
    QTreeView::columnMoved();
    d->storeColumns();
}

void BasicFileView::columnResized(int column, int oldSize, int newSize)
{
    QTreeView::columnResized(column, oldSize, newSize);
    d->storeColumns();
}

void BasicFileView::headerActionToggled()
{
    KAction *action = static_cast<KAction *>(sender());
    bool ok = false;
    int col = (int)action->data().toInt(&ok);
    if (!ok) return;

    d->storeColumns();
    d->setColumnVisible(col, action->isChecked());
    d->adjustColumns();
}

void BasicFileView::headerAdjustColumnWidths()
{
    d->adjustColumns();
}

void BasicFileView::headerResetToDefaults()
{
    d->resetColumnsToDefault();
}

void BasicFileView::sort(int t, Qt::SortOrder s)
{
    SortFilterFileModel *sortedModel = qobject_cast<SortFilterFileModel *>(model());
    if (sortedModel != NULL) {
        sortedModel->sort(t, s);
        d->storeColumns();
    }
}

void BasicFileView::noSorting()
{
    SortFilterFileModel *sortedModel = qobject_cast<SortFilterFileModel *>(model());
    if (sortedModel != NULL) {
        sortedModel->sort(-1);
        header()->setSortIndicator(-1, Qt::AscendingOrder);
        d->storeColumns();
    }
}
