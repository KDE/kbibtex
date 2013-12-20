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

#ifndef KBIBTEX_NETWORKING_ZOTERO_COLLECTIONMODEL_H
#define KBIBTEX_NETWORKING_ZOTERO_COLLECTIONMODEL_H

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <QSet>

#include "kbibtexnetworking_export.h"

class InternalNetworkAccessManager;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT CollectionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit CollectionModel(int userId = 475425, QObject *parent = NULL);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &) const;
    int columnCount(const QModelIndex &) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    virtual bool canFetchMore(const QModelIndex &parent) const;
    virtual void fetchMore(const QModelIndex &parent);

private:
    int m_userId;
    InternalNetworkAccessManager *m_nam;

    QHash<QString, QString> m_collectionToLabel;
    QHash<QString, QModelIndex> m_collectionToModelIndexParent;
    QHash<QString, QString> m_collectionToParent;
    QHash<QString, QVector<QString> > m_collectionToChildren;
    QSet<QString> m_downloadingKeys;

private slots:
    void finishedFetchingCollection();
};

#endif // KBIBTEX_NETWORKING_ZOTERO_COLLECTIONMODEL_H
