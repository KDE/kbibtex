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

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <KLocale>
#include <KIcon>
#include <KDebug>

#include "internalnetworkaccessmanager.h"

#define collectionId(index) (index.internalId())

CollectionModel::CollectionModel(int userId, QObject *parent)
        : QAbstractItemModel(parent), m_userId(userId), m_nam(InternalNetworkAccessManager::self())
{
    m_collectionToLabel.insert(0, i18n("Library for user id %1", m_userId));
    m_collectionToParent.insert(0, -1);
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
    const int cId = collectionId(index);
    if (cId < 0)
        return QVariant();

    if (role == Qt::DisplayRole && m_collectionToLabel.contains(cId))
        return m_collectionToLabel[cId];
    else if (role == Qt::DecorationRole) {
        if (cId == 0)
            return KIcon("folder-tar");
        else
            return KIcon("folder-yellow");
    }

    // TODO
    return QVariant();
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &parent) const
{
    /// Cover root node
    if (parent == QModelIndex())
        return createIndex(row, column, 0);

    const int parent_cId = collectionId(parent);
    /// For invalid parents, create no index
    if (parent_cId < 0)
        return QModelIndex();

    /// For known children, create an index containing
    /// the children's unique id as data
    if (!m_collectionToChildren.contains(parent_cId) || m_collectionToChildren[parent_cId].count() <= row)
        return createIndex(row, column, m_collectionToChildren[parent_cId][row]);

    /// No previous case covered?
    return QModelIndex();
}

QModelIndex CollectionModel::parent(const QModelIndex &index) const
{
    const int cId = collectionId(index);

    /// Only invalid or unknown indices
    /// or the root node have no parents
    if (cId < 0 || !m_collectionToParent.contains(cId) || m_collectionToParent[cId] < 0)
        return QModelIndex();

    // TODO
    return QModelIndex();
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent == QModelIndex())
        // TODO
        return 1;

// TODO
    return 0;
}

int CollectionModel::columnCount(const QModelIndex &) const
{
    /// Singe column design;
    return 1;
}

bool CollectionModel::hasChildren(const QModelIndex &parent) const
{
    const int parent_cId = collectionId(parent);
    /// Only invalid elements and elements where it is known
    /// that no children exist will return that they have no chidren
    if (parent_cId < 0 || (m_collectionToChildren.contains(parent_cId) && m_collectionToChildren[parent_cId].isEmpty()))
        return false;

    if (!m_collectionToChildren.contains(parent_cId)) {
        QUrl url(QLatin1String("https://api.zotero.org"));
        if (parent_cId == 0)
            url.setPath(QString(QLatin1String("/users/%1/collections/top")).arg(m_userId));
        else
            url.setPath(QString(QLatin1String("/users/%1/collections/%2/collections")).arg(m_userId).arg(parent_cId));
        url.addQueryItem(QLatin1String("limit"), QLatin1String("25"));
        QNetworkRequest request(url);
        request.setRawHeader("Zotero-API-Version", "2");
        QNetworkReply *reply = m_nam->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(finishedFetchingCollection()));
        reply->setProperty("parent_cId", QVariant::fromValue<int>(parent_cId));
    }

    /// Default assumption: every collection has children
    return true;
}

void CollectionModel::finishedFetchingCollection()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    bool ok = false;
    const int parent_cId = reply->property("parent_cId").toInt(&ok);
    if (!ok) return;

    QString rawText = QString::fromUtf8(reply->readAll().data());
    kDebug() << rawText.left(1024);
}
