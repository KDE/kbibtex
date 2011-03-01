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

BibTeXFileView::BibTeXFileView(QWidget * parent)
        : QTreeView(parent), m_signalMapperBibTeXFields(new QSignalMapper(this))
{
    /// general visual appearance and behaviour
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);

    /// header appearance and behaviour
    header()->setClickable(true);
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(-1, Qt::AscendingOrder);
    connect(header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(sort(int, Qt::SortOrder)));
    header()->setContextMenuPolicy(Qt::ActionsContextMenu);

    /// build context menu for header to show/hide single columns
    BibTeXFields *bibtexFields = BibTeXFields::self();
    int col = 0;
    for (BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it, ++col) {
        QString label = (*it).label;
        KAction *action = new KAction(label, header());
        action->setData(col);
        action->setCheckable(true);
        action->setChecked((*it).visible);
        connect(action, SIGNAL(triggered()), m_signalMapperBibTeXFields, SLOT(map()));
        m_signalMapperBibTeXFields->setMapping(action, action);
        header()->addAction(action);
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

BibTeXFileView::~BibTeXFileView()
{
    BibTeXFields *bibtexFields = BibTeXFields::self();

    for (int i = header()->count() - 1; i >= 0; --i) {
        FieldDescription fd = bibtexFields->at(i);
        fd.width = columnWidth(i);
        bibtexFields->replace(i, fd);
    }
    bibtexFields->save();
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

void BibTeXFileView::resizeEvent(QResizeEvent */*event*/)
{
    BibTeXFields *bibtexFields = BibTeXFields::self();
    int sum = 0;
    int widgetWidth = size().width() - verticalScrollBar()->size().width();

    for (BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it)
        if ((*it).visible)
            sum += (*it).width;

    int col = 0;
    for (BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it, ++col) {
        setColumnWidth(col, (*it).width * widgetWidth / sum);
        setColumnHidden(col, !((*it).visible));
    }
}

void BibTeXFileView::headerActionToggled(QObject *obj)
{
    KAction *action = dynamic_cast<KAction*>(obj);
    if (action == NULL) return;
    bool ok = false;
    int col = (int)action->data().toInt(&ok);
    if (!ok) return;

    BibTeXFields *bibtexFields = BibTeXFields::self();
    FieldDescription fd = bibtexFields->at(col);
    fd.visible = action->isChecked();
    if (fd.width < 4) fd.width = width() / 10;
    bibtexFields->replace(col, fd);

    resizeEvent(NULL);
}

void BibTeXFileView::headerResetToDefaults()
{
    BibTeXFields::self()->resetToDefaults();
    resizeEvent(NULL);
}

void BibTeXFileView::sort(int t, Qt::SortOrder s)
{
    SortFilterBibTeXFileModel *sortedModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    if (sortedModel != NULL)
        sortedModel->sort(t, s);
}
