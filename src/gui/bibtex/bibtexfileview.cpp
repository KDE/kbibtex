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

#include <QHeaderView>
#include <QSignalMapper>
#include <QScrollBar>

#include <KAction>
#include <KLocale>
#include <KDebug>

#include <bibtexfields.h>
#include "bibtexfilemodel.h"
#include "bibtexfileview.h"

BibTeXFileView::BibTeXFileView(const QString &name, QWidget * parent)
        : QTreeView(parent), m_name(name), m_signalMapperBibTeXFields(new QSignalMapper(this))
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
    header()->setMovable(false); ///< FIXME don't know how to restore moved columns
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(-1, Qt::AscendingOrder);
    connect(header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(sort(int, Qt::SortOrder)));
    header()->setContextMenuPolicy(Qt::ActionsContextMenu);

    /// build context menu for header to show/hide single columns
    int col = 0;
    foreach(const FieldDescription &fd,  *BibTeXFields::self()) {
        KAction *action = new KAction(fd.label, header());
        action->setData(col);
        action->setCheckable(true);
        action->setChecked(fd.visible[m_name]);
        connect(action, SIGNAL(triggered()), m_signalMapperBibTeXFields, SLOT(map()));
        m_signalMapperBibTeXFields->setMapping(action, action);
        header()->addAction(action);
        ++col;
    }
    connect(m_signalMapperBibTeXFields, SIGNAL(mapped(QObject*)), this, SLOT(headerActionToggled(QObject*)));

    /// add separator to header's context menu
    KAction *action = new KAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    /// add action to reset to defaults (regarding column visibility) to header's context menu
    action = new KAction(i18n("Reset to defaults"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(headerResetToDefaults()));
    header()->addAction(action);
}

void BibTeXFileView::setModel(QAbstractItemModel * model)
{
    QTreeView::setModel(model);

    m_sortFilterProxyModel = NULL;
    m_bibTeXFileModel = dynamic_cast<BibTeXFileModel*>(model);
    if (m_bibTeXFileModel == NULL) {
        m_sortFilterProxyModel = dynamic_cast<QSortFilterProxyModel*>(model);
        Q_ASSERT(m_sortFilterProxyModel != NULL);
        m_bibTeXFileModel = dynamic_cast<BibTeXFileModel*>(m_sortFilterProxyModel->sourceModel());
    }
    Q_ASSERT(m_bibTeXFileModel != NULL);
}

BibTeXFileModel *BibTeXFileView::bibTeXModel()
{
    return m_bibTeXFileModel;
}

QSortFilterProxyModel *BibTeXFileView::sortFilterProxyModel()
{
    return m_sortFilterProxyModel;
}

void BibTeXFileView::resizeEvent(QResizeEvent *)
{
    int sum = 0;
    int widgetWidth = size().width() - verticalScrollBar()->size().width() - 8;

    foreach(const FieldDescription &fd, *BibTeXFields::self()) {
        if (fd.visible[m_name])
            sum += fd.width[m_name];
    }
    Q_ASSERT(sum > 0);

    int col = 0;
    foreach(const FieldDescription &fd, *BibTeXFields::self()) {
        setColumnWidth(col, fd.width[m_name] * widgetWidth / sum);
        setColumnHidden(col, !fd.visible[m_name]);
        ++col;
    }
}

void BibTeXFileView::columnResized(int column, int oldSize, int newSize)
{
    syncBibTeXFields();
    QTreeView::columnResized(column, oldSize, newSize);
}

void BibTeXFileView::syncBibTeXFields()
{
    int i = 0;
    BibTeXFields *bibtexFields = BibTeXFields::self();
    foreach(const FieldDescription &origFd, *bibtexFields) {
        FieldDescription newFd(origFd);
        newFd.width[m_name] = newFd.visible[m_name] ? columnWidth(i) : 0;
        bibtexFields->replace(i, newFd);
        ++i;
    }
    bibtexFields->save();
}

void BibTeXFileView::headerActionToggled(QObject *obj)
{
    KAction *action = dynamic_cast<KAction*>(obj);
    if (action == NULL) return;
    bool ok = false;
    int col = (int)action->data().toInt(&ok);
    if (!ok) return;

    BibTeXFields *bibtexFields = BibTeXFields::self();
    FieldDescription fd(bibtexFields->at(col));
    fd.visible[m_name] = action->isChecked();
    bibtexFields->replace(col, fd); ///< replace already here to make sum calculation below work

    /// accumulate column widths (needed below)
    int sum = 0;
    foreach(const FieldDescription &fd, *BibTeXFields::self()) {
        if (fd.visible[m_name])
            sum += fd.width[m_name];
    }
    if (sum == 0) {
        /// no more columns left visible, therefore re-visibiling this column
        action->setChecked(fd.visible[m_name] = true);
        sum = 10;
    }
    if (fd.visible[m_name]) {
        /// column just got visible, reset width
        fd.width[m_name] = sum / 10;
    }

    bibtexFields->replace(col, fd);

    resizeEvent(NULL);
    syncBibTeXFields();
}

void BibTeXFileView::headerResetToDefaults()
{
    BibTeXFields *bibtexFields = BibTeXFields::self();
    bibtexFields->resetToDefaults(m_name);
    foreach(QAction *action, header()->actions()) {
        bool ok = false;
        int col = (int)action->data().toInt(&ok);
        if (ok) {
            FieldDescription fd = bibtexFields->at(col);
            action->setChecked(fd.visible[m_name]);
        }
    }

    resizeEvent(NULL);
}

void BibTeXFileView::sort(int t, Qt::SortOrder s)
{
    SortFilterBibTeXFileModel *sortedModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    if (sortedModel != NULL)
        sortedModel->sort(t, s);
}
