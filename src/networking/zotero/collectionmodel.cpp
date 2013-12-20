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
#include <QXmlStreamReader>

#include <KLocale>
#include <KIcon>

#include "internalnetworkaccessmanager.h"

#define zoteroId(index) ((index)==QModelIndex()?QString():QLatin1String((const char*)((index).internalPointer())))

Q_DECLARE_METATYPE(QModelIndex)

const QString rootKey = QLatin1String("ROOT_KEY_KBIBTEX");

CollectionModel::CollectionModel(int userId, QObject *parent)
        : QAbstractItemModel(parent), m_userId(userId), m_nam(InternalNetworkAccessManager::self())
{
    /// Initialize root node
    m_collectionToLabel.insert(rootKey, i18n("Library for user id %1", m_userId));
    m_collectionToModelIndexParent.insert(rootKey, QModelIndex());
    m_collectionToParent.insert(rootKey, QString());
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
    const QString id = zoteroId(index);
    if (!m_collectionToLabel.contains(id))
        return QVariant();

    if (role == Qt::DisplayRole && m_collectionToLabel.contains(id))
        return m_collectionToLabel[id];
    else if (role == Qt::DecorationRole) {
        if (id == rootKey)
            return KIcon("folder-tar");
        else
            return KIcon("folder-yellow");
    }

    // TODO more roles to cover

    return QVariant();
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &parent) const
{
    /// Cover root node
    if (parent == QModelIndex())
        return createIndex(row, column, strdup(rootKey.toAscii().data()));

    const QString parentId = zoteroId(parent);
    /// For known children, create an index containing
    /// the children's unique id as data
    if (row >= 0 && m_collectionToChildren.contains(parentId) && row < m_collectionToChildren[parentId].count())
        return createIndex(row, column, strdup(m_collectionToChildren[parentId][row].toAscii().data()));

    /// This should never happen!
    return QModelIndex();
}

QModelIndex CollectionModel::parent(const QModelIndex &index) const
{
    const QString id = zoteroId(index);

    if (m_collectionToModelIndexParent.contains(id))
        return m_collectionToModelIndexParent[id];
    else
        return QModelIndex();
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    /// There is one root node
    if (parent == QModelIndex())
        return 1;

    const QString parentId = zoteroId(parent);
    if (m_collectionToChildren.contains(parentId)) {
        /// Node has known children
        return m_collectionToChildren[parentId].count();
    } else {
        /// No knowledge about child nodes
        return 0;
    }
}

int CollectionModel::columnCount(const QModelIndex &) const
{
    /// Singe column design;
    return 1;
}

bool CollectionModel::hasChildren(const QModelIndex &parent) const
{
    const QString parentId = zoteroId(parent);
    return !m_collectionToChildren.contains(parentId) || !m_collectionToChildren[parentId].isEmpty();
}

bool CollectionModel::canFetchMore(const QModelIndex &parent) const
{
    const QString parentId = zoteroId(parent);
    /// Only for nodes where no more children are known
    /// and where download is running, allow to fetch more
    const bool canFetch = !parentId.isEmpty() && !m_collectionToChildren.contains(parentId) && !m_downloadingKeys.contains(parentId);
    return canFetch;
}

void CollectionModel::fetchMore(const QModelIndex &parent)
{
    const QString parentId = zoteroId(parent);

    /// Only for nodes where no more children are known
    /// and where download is running, download more data
    if (!m_collectionToChildren.contains(parentId) && !m_downloadingKeys.contains(parentId)) {
        m_downloadingKeys.insert(parentId);
        QUrl url(QLatin1String("https://api.zotero.org"));
        if (parentId == rootKey)
            url.setPath(QString(QLatin1String("/users/%1/collections/top")).arg(m_userId));
        else
            url.setPath(QString(QLatin1String("/users/%1/collections/%2/collections")).arg(m_userId).arg(parentId));
        url.addQueryItem(QLatin1String("limit"), QLatin1String("25"));

        QNetworkRequest request(url);
        request.setRawHeader("Zotero-API-Version", "2");

        QNetworkReply *reply = m_nam->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(finishedFetchingCollection()));
        reply->setProperty("parentId", QVariant::fromValue<QString>(parentId));
        reply->setProperty("parent", QVariant::fromValue<QModelIndex>(parent));
    }
}

void CollectionModel::finishedFetchingCollection()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    const QString parentId = reply->property("parentId").toString();
    const QModelIndex parent = reply->property("parent").value<QModelIndex>();

    m_collectionToChildren[parentId] = QVector<QString>();

    QXmlStreamReader xmlReader(reply);
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        const QXmlStreamReader::TokenType tt = xmlReader.readNext();
        if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("entry")) {
            QString title = QLatin1String("unknown"), key;
            while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("title"))
                    title = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("key"))
                    key = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("entry"))
                    break;
            }

            m_collectionToLabel.insert(key, title);
            m_collectionToModelIndexParent.insert(key, parent);
            m_collectionToParent.insert(key, parentId);
            QVector<QString> vec = m_collectionToChildren[parentId];
            vec.append(key);
            m_collectionToChildren[parentId] = vec;
        } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("feed"))
            break;
    }
    m_downloadingKeys.remove(parentId);

    if (m_collectionToChildren.contains(parentId) && !m_collectionToChildren[parentId].isEmpty())
        insertRows(0, m_collectionToChildren[parentId].count(), parent);
}
