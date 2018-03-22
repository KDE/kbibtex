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

#include "tagmodel.h"

#include <QHash>

#include <QIcon>

#include "zotero/tags.h"

using namespace Zotero;

class Zotero::TagModel::Private
{
public:
    Private(Tags *t, Zotero::TagModel *parent)
            : tags(t) {
        Q_UNUSED(parent)
    }

    Tags *tags;

    QHash<QString, QModelIndex> tagToModelIndex;
};

TagModel::TagModel(Tags *tags, QObject *parent)
        : QAbstractItemModel(parent), d(new Zotero::TagModel::Private(tags, this))
{
    beginResetModel();
    connect(tags, &Tags::finishedLoading, this, &TagModel::fetchingDone);
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
    if (!d->tags->initialized())
        return QVariant();

    if (index == QModelIndex())
        return QVariant();

    const QMap<QString, int> data = d->tags->tags();
    if (index.row() < 0 || index.row() >= data.count())
        return QVariant();

    const QList<QString> dataKeys = data.keys();
    if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return dataKeys[index.row()];
        else if (index.column() == 1)
            return data.value(dataKeys[index.row()]);
    } else if (role == Qt::DecorationRole) {
        if (index.column() == 0)
            return QIcon::fromTheme(QStringLiteral("mail-tagged"));
    } else if (role == TagRole)
        return dataKeys[index.row()];
    else if (role == TagCountRole)
        return data.value(dataKeys[index.row()]);

    return QVariant();
}

QModelIndex TagModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!d->tags->initialized() || parent != QModelIndex())
        return QModelIndex();

    const QMap<QString, int> data = d->tags->tags();
    if (row < 0 || column < 0 || row >= data.count() || column >= 2)
        return QModelIndex();

    const QList<QString> dataKeys = data.keys();
    return createIndex(row, column, qHash(dataKeys[row]));
}

QModelIndex TagModel::parent(const QModelIndex &) const
{
    /// This is a flat list
    return QModelIndex();
}

int TagModel::rowCount(const QModelIndex &parent) const
{
    if (!d->tags->initialized() || parent != QModelIndex())
        return 0;

    const QMap<QString, int> data = d->tags->tags();
    return data.count();
}

int TagModel::columnCount(const QModelIndex &) const
{
    /// Double column design;
    return 2;
}

bool TagModel::hasChildren(const QModelIndex &) const
{
    return false;
}

void TagModel::fetchingDone()
{
    endResetModel();
}
