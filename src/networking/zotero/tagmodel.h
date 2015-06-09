/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_NETWORKING_ZOTERO_TAGMODEL_H
#define KBIBTEX_NETWORKING_ZOTERO_TAGMODEL_H

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <QSet>

#include "kbibtexnetworking_export.h"

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
    enum Roles { TagRole = Qt::UserRole + 6685, TagCountRole = Qt::UserRole + 6686 };

    explicit TagModel(Zotero::Tags *tags, QObject *parent = NULL);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &) const;
    int columnCount(const QModelIndex &) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

private slots:
    void fetchingDone();

private:
    class Private;
    Private *const d;
};

} // end of namespace Zotero

#endif // KBIBTEX_NETWORKING_ZOTERO_TAGMODEL_H
