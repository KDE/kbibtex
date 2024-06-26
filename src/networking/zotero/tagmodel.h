/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_NETWORKING_ZOTERO_TAGMODEL_H
#define KBIBTEX_NETWORKING_ZOTERO_TAGMODEL_H

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <QSet>

#ifdef HAVE_KF
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF

namespace Zotero
{

class Tags;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT TagModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum TagModelRoles {
        TagRole = Qt::UserRole + 6685,
        TagCountRole = Qt::UserRole + 6686
    };

    explicit TagModel(Zotero::Tags *tags, QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &) const override;
    QModelIndex parent(const QModelIndex &) const override;
    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

private:
    class Private;
    Private *const d;
};

} // end of namespace Zotero

#endif // KBIBTEX_NETWORKING_ZOTERO_TAGMODEL_H
