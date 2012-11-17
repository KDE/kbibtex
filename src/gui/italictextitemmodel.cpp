/*****************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#include <QFont>

#include "italictextitemmodel.h"

ItalicTextItemModel::ItalicTextItemModel(QObject *parent)
        : QAbstractItemModel(parent)
{
    // nothing
}

void ItalicTextItemModel::addItem(const QString &a, const QString &b)
{
    m_data.append(QPair<QString, QString>(a, b));
}

QVariant ItalicTextItemModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_data.count())
        return QVariant();

    if (role == Qt::FontRole) {
        QFont font;
        if (m_data[index.row()].second.isEmpty())
            font.setItalic(true);
        return font;
    } else if (role == Qt::DisplayRole) {
        return m_data[index.row()].first;
    } else if (role == Qt::UserRole) {
        return m_data[index.row()].second;
    }

    return QVariant();
}

QModelIndex ItalicTextItemModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column);
}

QModelIndex ItalicTextItemModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int ItalicTextItemModel::rowCount(const QModelIndex &) const
{
    return m_data.count();
}

int ItalicTextItemModel::columnCount(const QModelIndex &) const
{
    return 1;
}
