/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "zoterobrowser.h"

#include <QTreeView>
#include <QLayout>

#include "zotero/collectionmodel.h"
#include "zotero/collection.h"

ZoteroBrowser::ZoteroBrowser(QWidget *parent)
        : QWidget(parent)
{
    QBoxLayout *layout = new QVBoxLayout(this);
    QTreeView *treeView = new QTreeView(this);
    layout->addWidget(treeView);
    Zotero::Collection *collection = Zotero::Collection::fromUserId(475425, this);
    QAbstractItemModel *model = new Zotero::CollectionModel(collection, this);
    treeView->setModel(model);
    treeView->setHeaderHidden(true);
}

ZoteroBrowser::~ZoteroBrowser()
{
    // TODO
}

