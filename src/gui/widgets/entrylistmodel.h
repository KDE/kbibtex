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
#ifndef KBIBTEX_GUI_ENTRYLISTMODEL_H
#define KBIBTEX_GUI_ENTRYLISTMODEL_H

#include <QAbstractItemModel>

namespace KBibTeX
{
namespace IO {
class Entry;
}

namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class EntryListModel : public QAbstractItemModel
{
public:
    EntryListModel(const KBibTeX::IO::Entry *entry, QAbstractItemView * parent);

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex & index = QModelIndex()) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index = QModelIndex()) const;

private:
    class EntryListModelPrivate;
    EntryListModelPrivate *d;
};

}
}
}

#endif // KBIBTEX_GUI_ENTRYLISTMODEL_H
