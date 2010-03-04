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

#include <QScrollArea>
#include <QLabel>
#include <QLayout>
#include <QListView>

#include <fieldeditor.h>
#include <value.h>
#include <entrylistview.h>
#include <entrylistmodel.h>
#include "entryviewer.h"

using namespace KBibTeX::GUI::Dialogs;

class EntryViewer::EntryViewerPrivate
{
public:
    EntryViewer *p;
    const KBibTeX::IO::Entry *entry;
    KBibTeX::GUI::Widgets::EntryListView *listView;
    KBibTeX::GUI::Widgets::EntryListModel *listModel;
    KBibTeX::GUI::Widgets::ValueItemDelegate *delegate;

    EntryViewerPrivate(const KBibTeX::IO::Entry *e, EntryViewer *parent)
            : p(parent), entry(e) {
        listView = new KBibTeX::GUI::Widgets::EntryListView(p);
        listModel = new KBibTeX::GUI::Widgets::EntryListModel(p);
        listView->setModel(listModel);
        delegate = new KBibTeX::GUI::Widgets::ValueItemDelegate(listView);
        listView->setItemDelegate(delegate);

        QVBoxLayout *layout = new QVBoxLayout(p);
        layout->addWidget(listView);
        p->setMinimumSize(512, 384);

        reset();
    }

    void reset() {
        listModel->setEntry(*entry);
    }
};

EntryViewer::EntryViewer(const KBibTeX::IO::Entry *entry, QWidget *parent)
        : QWidget(parent), d(new EntryViewerPrivate(entry, this))
{
    // TODO
}

KBibTeX::GUI::Widgets::EntryListModel *EntryViewer::model()
{
    return d->listModel;
}

KBibTeX::GUI::Widgets::ValueItemDelegate *EntryViewer::delegate()
{
    return d->delegate;
}

void EntryViewer::reset()
{
    d->reset();
}
