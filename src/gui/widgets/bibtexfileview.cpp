/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <KAction>
#include <KDebug>

#include <bibtexfields.h>
#include "bibtexfilemodel.h"
#include "bibtexfileview.h"

using namespace KBibTeX::GUI::Widgets;

BibTeXFileView::BibTeXFileView(QWidget * parent)
        : QTreeView(parent), m_signalMapperBibTeXFields(new QSignalMapper(this))
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    header()->setContextMenuPolicy(Qt::ActionsContextMenu);
    KBibTeX::GUI::Config::BibTeXFields *bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();

    int col = 0;
    for (KBibTeX::GUI::Config::BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it, ++col) {
        QString title = (*it).label;
        KAction *action = new KAction(title, header());
        action->setData(col);
        action->setCheckable(true);
        action->setChecked((*it).visible);
        connect(action, SIGNAL(triggered()), m_signalMapperBibTeXFields, SLOT(map()));
        m_signalMapperBibTeXFields->setMapping(action, action);
        header()->addAction(action);
    }
    connect(m_signalMapperBibTeXFields, SIGNAL(mapped(QObject*)), this, SLOT(headerActionToggled(QObject*)));

    KAction *action = new KAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    action = new KAction("Reset to defaults", header());
    connect(action, SIGNAL(triggered()), this, SLOT(headerResetToDefaults()));
    header()->addAction(action);
}

BibTeXFileView::~BibTeXFileView()
{
    int columnCount = header()->count();
    KBibTeX::GUI::Config::BibTeXFields *bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();

    for (int i = 0; i < columnCount; ++i) {
        KBibTeX::GUI::Config::FieldDescription fd = bibtexFields->at(i);
        fd.width = columnWidth(i);
        bibtexFields->replace(i, fd);
    }
    bibtexFields->save();
}

const QList<KBibTeX::IO::Element*>& BibTeXFileView::selectedElements() const
{
    return m_selection;
}

void BibTeXFileView::resizeEvent(QResizeEvent */*event*/)
{
    KBibTeX::GUI::Config::BibTeXFields *bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();
    int sum = 1, widgetWidth = size().width() - 64; // FIXME Use action width value
    for (KBibTeX::GUI::Config::BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it)
        if ((*it).visible)
            sum += (*it).width;
    int col = 0;
    for (KBibTeX::GUI::Config::BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it, ++col) {
        setColumnWidth(col, (*it).width * widgetWidth / sum);
        setColumnHidden(col, !((*it).visible));
    }
}

void BibTeXFileView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    QTreeView::currentChanged(current, previous);

    BibTeXFileModel *bibTeXFileModel = dynamic_cast<BibTeXFileModel*>(model());
    m_current = bibTeXFileModel->element(current.row());

    emit currentElementChanged(m_current);
}

void BibTeXFileView::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    BibTeXFileModel *bibTeXFileModel = dynamic_cast<BibTeXFileModel*>(model());

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.append(bibTeXFileModel->element(it->row()));
    }

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.removeOne(bibTeXFileModel->element(it->row()));
    }

    emit selectedElementsChanged();
}

void BibTeXFileView::headerActionToggled(QObject *obj)
{
    KAction *action = dynamic_cast<KAction*>(obj);
    if (action == NULL) return;
    bool ok = false;
    int col = (int)action->data().toInt(&ok);
    if (!ok) return;

    KBibTeX::GUI::Config::BibTeXFields *bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();
    KBibTeX::GUI::Config::FieldDescription fd = bibtexFields->at(col);
    fd.visible = action->isChecked();
    if (fd.width < 4) fd.width = width() / 10;
    bibtexFields->replace(col, fd);

    resizeEvent(NULL);
}

void BibTeXFileView::headerResetToDefaults()
{
    KBibTeX::GUI::Config::BibTeXFields::self()->resetToDefaults();
    resizeEvent(NULL);
}
