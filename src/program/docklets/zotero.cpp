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

#include "zotero.h"

#include <QTreeView>
#include <QLayout>

#include "collectionmodel.h"

Zotero::Zotero(QWidget *parent)
        : QWidget(parent)
{
    QBoxLayout *layout = new QVBoxLayout(this);
    QTreeView *treeView = new QTreeView(this);
    layout->addWidget(treeView);
    QAbstractItemModel *model = new CollectionModel();
    treeView->setModel(model);
    treeView->setHeaderHidden(true);
}

Zotero::~Zotero()
{
    // TODO
}

