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

#ifndef KBIBTEX_PROGRAM_ZOTEROBROWSER_H
#define KBIBTEX_PROGRAM_ZOTEROBROWSER_H

#include <QWidget>
#include <QModelIndex>

class Element;
class SearchResults;

namespace Zotero
{
class Items;
class Collection;
class CollectionModel;
}

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class ZoteroBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit ZoteroBrowser(SearchResults *searchResults, QWidget *parent);
    ~ZoteroBrowser();

private:
    Zotero::Items *m_items;
    Zotero::Collection *m_collection;
    Zotero::CollectionModel *m_model;
    SearchResults *m_searchResults;

private slots:
    void modelReset();
    void collectionDoubleClicked(const QModelIndex &index);
    void showItem(QSharedPointer<Element>);
};


#endif // KBIBTEX_PROGRAM_ZOTEROBROWSER_H
