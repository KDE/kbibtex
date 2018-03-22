/*****************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#include "italictextitemmodel.h"

#include <QFont>

#include "logging_gui.h"

class ItalicTextItemModel::Private
{
public:
    QList<QPair<QString, QString> > data;

    Private(ItalicTextItemModel *parent)
    {
        Q_UNUSED(parent)
    }
};

ItalicTextItemModel::ItalicTextItemModel(QObject *parent)
        : QAbstractItemModel(parent), d(new Private(this))
{
    /// nothing
}

ItalicTextItemModel::~ItalicTextItemModel()
{
    delete d;
}

void ItalicTextItemModel::addItem(const QString &a, const QString &b)
{
    /// Store both arguments in a pair of strings,
    /// added to the data structure
    d->data.append(QPair<QString, QString>(a, b));
}

QVariant ItalicTextItemModel::data(const QModelIndex &index, int role) const
{
    /// Test and ignore for invalid rows
    if (index.row() < 0 || index.row() >= d->data.count())
        return QVariant();

    if (role == Qt::FontRole) {
        QFont font;
        /// If the identifier for a data row is empty,
        /// set the row's text in italics
        if (d->data[index.row()].second.isEmpty())
            font.setItalic(true);
        return font;
    } else if (role == Qt::DisplayRole) {
        /// Show text as passed as first parameter to function addItem
        return d->data[index.row()].first;
    } else if (role == Qt::UserRole) {
        qCWarning(LOG_KBIBTEX_GUI) << "Requesting data from Qt::UserRole is deprecated, should not happen";
    } else if (role == IdentifierRole) {
        /// Return string as passed as second parameter to function addItem
        return d->data[index.row()].second;
    }

    return QVariant();
}

QModelIndex ItalicTextItemModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column);
}

QModelIndex ItalicTextItemModel::parent(const QModelIndex &) const
{
    /// Flat, single-level model
    return QModelIndex();
}

int ItalicTextItemModel::rowCount(const QModelIndex &) const
{
    /// As many rows as elements in data structure
    /// (should be as many calls to addItem were made)
    return d->data.count();
}

int ItalicTextItemModel::columnCount(const QModelIndex &) const
{
    /// Just one column, thus suitable for combo boxes
    return 1;
}
