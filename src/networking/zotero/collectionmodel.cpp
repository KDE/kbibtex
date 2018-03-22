/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "collectionmodel.h"

#include <QHash>

#include <QIcon>

#include "zotero/collection.h"

using namespace Zotero;

const int Zotero::CollectionModel::CollectionIdRole = Qt::UserRole + 6681;

class Zotero::CollectionModel::Private
{
public:
    Private(Collection *c, Zotero::CollectionModel *parent)
            : collection(c) {
        Q_UNUSED(parent)
    }

    Collection *collection;

    QHash<QString, QModelIndex> collectionIdToModelIndex;
};

CollectionModel::CollectionModel(Collection *collection, QObject *parent)
        : QAbstractItemModel(parent), d(new Zotero::CollectionModel::Private(collection, this))
{
    beginResetModel();
    connect(collection, &Collection::finishedLoading, this, &CollectionModel::fetchingDone);
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
    if (!d->collection->initialized())
        return QVariant();

    if (index == QModelIndex())
        return QVariant();

    if (role == Qt::DecorationRole) {
        /// Fancy icons for collections
        if (index.internalId() == 0) /// root node
            return QIcon::fromTheme(QStringLiteral("folder-orange"));
        else /// any other node
            return QIcon::fromTheme(QStringLiteral("folder-yellow"));
    } else if (role == Qt::DisplayRole) {
        /// Show collections' human-readable description
        const QString collectionId = d->collection->collectionFromNumericId(index.internalId());
        return d->collection->collectionLabel(collectionId);
    } else if (role == CollectionIdRole) {
        if (index.internalId() == 0) /// root node
            return QString();
        else /// any other node
            return d->collection->collectionFromNumericId(index.internalId());
    }

    return QVariant();
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!d->collection->initialized())
        return QModelIndex();

    if (parent == QModelIndex()) { ///< root node
        const QModelIndex result = createIndex(row, column, (quintptr)0);
        d->collectionIdToModelIndex.insert(d->collection->collectionFromNumericId(0), result);
        return result;
    }

    const QString collectionId = d->collection->collectionFromNumericId(parent.internalId());
    QVector<QString> children = d->collection->collectionChildren(collectionId);
    if (row < children.count()) {
        const QModelIndex result = createIndex(row, column, d->collection->collectionNumericId(children[row]));
        d->collectionIdToModelIndex.insert(children[row], result);
        return result;
    }

    return QModelIndex();
}

QModelIndex CollectionModel::parent(const QModelIndex &index) const
{
    if (!d->collection->initialized())
        return QModelIndex();

    if (index == QModelIndex() || index.internalId() == 0)
        return QModelIndex(); ///< invalid index or root node

    const QString parentId = d->collection->collectionParent(d->collection->collectionFromNumericId(index.internalId()));
    return d->collectionIdToModelIndex[parentId];
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    if (!d->collection->initialized())
        return 0;

    if (parent == QModelIndex())
        return 1; ///< just one single root node

    const QString collectionId = d->collection->collectionFromNumericId(parent.internalId());
    QVector<QString> children = d->collection->collectionChildren(collectionId);
    return children.count();
}

int CollectionModel::columnCount(const QModelIndex &) const
{
    /// Singe column design;
    return 1;
}

bool CollectionModel::hasChildren(const QModelIndex &parent) const
{
    if (!d->collection->initialized())
        return false;

    return rowCount(parent) > 0;
}

void CollectionModel::fetchingDone()
{
    endResetModel();
}
