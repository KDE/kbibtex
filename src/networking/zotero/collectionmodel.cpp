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

#include "collectionmodel.h"

QVariant CollectionModel::data(const QModelIndex &, int) const
{
    // TODO
    return QVariant();
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &) const
{
    // TODO
    return createIndex(row, column);
}

QModelIndex CollectionModel::parent(const QModelIndex &) const
{
    // TODO
    return QModelIndex();
}

int CollectionModel::rowCount(const QModelIndex &) const
{
    // TODO
    return 0;
}

int CollectionModel::columnCount(const QModelIndex &) const
{
    return 0;
}
