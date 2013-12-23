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
#include <QAbstractItemModel>

#include "element.h"
#include "searchresults.h"
#include "zotero/collectionmodel.h"
#include "zotero/collection.h"
#include "zotero/items.h"

ZoteroBrowser::ZoteroBrowser(SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), m_searchResults(searchResults)
{
    QBoxLayout *layout = new QVBoxLayout(this);
    QTreeView *treeView = new QTreeView(this);
    layout->addWidget(treeView);
    m_items = new Zotero::Items(this);
    m_collection = Zotero::Collection::fromUserId(475425, this);
    m_model = new Zotero::CollectionModel(m_collection, this);
    treeView->setModel(m_model);
    treeView->setHeaderHidden(true);
    treeView->setExpandsOnDoubleClick(false);

    setEnabled(false);
    setCursor(Qt::WaitCursor);
    connect(m_model, SIGNAL(modelReset()), this, SLOT(modelReset()));
    connect(treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(collectionDoubleClicked(QModelIndex)));
    connect(m_items, SIGNAL(foundElement(QSharedPointer<Element>)), this, SLOT(showItem(QSharedPointer<Element>)));
}

ZoteroBrowser::~ZoteroBrowser()
{
    // TODO
}

void ZoteroBrowser::modelReset()
{
    setEnabled(true);
    setCursor(Qt::ArrowCursor); // TODO should be default cursor
}

void ZoteroBrowser::collectionDoubleClicked(const QModelIndex &index)
{
    const QString collectionId = index.data(Zotero::CollectionModel::CollectionIdRole).toString();
    if (!collectionId.isEmpty()) {
        m_searchResults->clear();
        m_items->retrieveItems(m_collection->baseUrl(), collectionId);
    }
}

void ZoteroBrowser::showItem(QSharedPointer<Element> e)
{
    m_searchResults->insertElement(e);
}
