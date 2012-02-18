/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextStream>
#include <QFile>

#include <KIcon>
#include <KLocale>
#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KMessageBox>

#include "xsltransform.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "readapi.h"

const QString ZoteroReadAPI::configGroupNameZotero = QLatin1String("Zotero");
const QString ZoteroReadAPI::keyUserID = QLatin1String("userId");
const QString ZoteroReadAPI::defaultUserID = QString::null;
const QString ZoteroReadAPI::keyPrivateKey = QLatin1String("privateKey");
const QString ZoteroReadAPI::defaultPrivateKey = QString::null;
const QString ZoteroReadAPI::zoteroApiUrlPrefix = QLatin1String("https://api.zotero.org");
const int ZoteroReadAPICollectionsModel::FilterStringListRole = Qt::UserRole + 671;

/// http://www.zotero.org/support/dev/server_api/read_api

ZoteroReadAPI::ZoteroReadAPI(QObject *parent)
        : QObject(parent)
{
// TODO
}

void ZoteroReadAPI::scanLibrary()
{
    m_runningNetworkConn = 0;
    bibTeXfile.clear();

    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("kbibtexrc"));
    KConfigGroup configGroup(config, configGroupNameZotero);

    m_userId = configGroup.readEntry(keyUserID, defaultUserID);
    m_privateKey = configGroup.readEntry(keyPrivateKey, defaultPrivateKey);
    if (m_userId.isEmpty()) {
        KMessageBox::information(NULL, i18n("Cannot sync without proper userId.\n\nConfigure Zotero synchronizations in the settings first."), i18n("Zotero"));
        return;
    }

    QString url = zoteroApiUrlPrefix + QString("/users/%1/collections?format=atom").arg(m_userId);
    if (!m_privateKey.isEmpty()) url.append("&key=").append(m_privateKey);

    emit busy(true);
    cleanTables();
    fetchCollections(QUrl(url));
}

void ZoteroReadAPI::fetchCollections(const QUrl &url)
{
    ++m_runningNetworkConn;
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(readCollectionsFinished()));
}

void ZoteroReadAPI::fetchItems(const QUrl &url)
{
    ++m_runningNetworkConn;
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(readItemsFinished()));
}

void ZoteroReadAPI::fetchItemsInCollections(const QUrl &url)
{
    ++m_runningNetworkConn;
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(readItemsInCollectionsFinished()));
}

void ZoteroReadAPI::cleanTables()
{
    QHash<QString, ZoteroCollection*>::Iterator it = collectionTable.begin();
    while (it != collectionTable.end()) {
        delete it.value();
        it = collectionTable.erase(it);
    }
}

void ZoteroReadAPI::rebuildTables()
{
    for (QHash<QString, ZoteroCollection*>::Iterator it = collectionTable.begin(); it != collectionTable.end(); ++it) {
        if (it.key().isEmpty()) continue;
        const QString parent = it.value()->parent;
        ZoteroCollection *parentCollection = collectionTable.value(parent, collectionTable.value(QString::null, NULL));
        if (parentCollection != NULL)
            parentCollection->childCollections.append(it.key());
    }
}

void ZoteroReadAPI::parseCollectionXMLFragement(const QString &xml)
{
    QString title, self, parent;
    int p1, p2;

    if ((p1 = xml.indexOf(QLatin1String("<title>"))) >= 0 && (p2 = xml.indexOf(QLatin1String("</title>"), p1)) >= 0) {
        title = xml.mid(p1 + 7, p2 - p1 - 7);
    }

    if ((p1 = xml.indexOf(QLatin1String("<zapi:key>"))) >= 0 && (p2 = xml.indexOf(QLatin1String("</zapi:key>"), p1)) >= 0) {
        self = xml.mid(p1 + 10, p2 - p1 - 10);
    } else if ((p1 = xml.indexOf(QLatin1String("<link rel=\"self\""))) >= 0 && (p1 = xml.indexOf(QLatin1String("collections/"), p1)) >= 0) {
        p2 = xml.indexOf(QChar('"'), p1);
        self = xml.mid(p1 + 12, p2 - p1 - 12);
    }

    if ((p1 = xml.indexOf(QLatin1String("<link rel=\"up\""))) >= 0 && (p1 = xml.indexOf(QLatin1String("collections/"), p1)) >= 0) {
        p2 = xml.indexOf(QChar('"'), p1);
        parent = xml.mid(p1 + 12, p2 - p1 - 12);
    }

    ZoteroCollection *collection = collectionTable.value(self, NULL);
    if (collection == NULL) {
        collection = new ZoteroCollection(self, parent, title);
        collectionTable.insert(self, collection);

        if (!self.isEmpty()) {
            int numItems = 0;
            if ((p1 = xml.indexOf(QLatin1String("<zapi:numItems>"))) >= 0) {
                p2 = xml.indexOf(QLatin1String("</zapi:numItems>"), p1);
                bool ok = false;
                numItems = xml.mid(p1 + 15, p2 - p1 - 15).toInt(&ok);
                if (!ok) numItems = 0;
            }

            if (numItems > 0 && (p1 = xml.indexOf(QLatin1String("<link rel=\"self\""))) >= 0 && (p1 = xml.indexOf(QLatin1String(" href=\""), p1)) >= 0) {
                p2 = xml.indexOf(QLatin1String("\""), p1 + 8);
                const QString selfUrl = xml.mid(p1 + 7, p2 - p1 - 7);
                QString itemsInCollectionUrl = selfUrl + QLatin1String("/items?format=atom");
                if (!m_privateKey.isEmpty()) itemsInCollectionUrl.append("&key=").append(m_privateKey);
                fetchItemsInCollections(QUrl(itemsInCollectionUrl));
            }
        }
    } else {
        collection->title = title;
        if (collection->parent.isEmpty())
            collection->parent = parent;
    }
}

void ZoteroReadAPI::readCollectionsFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    --m_runningNetworkConn;

    if (reply->error() == QNetworkReply::NoError) {
        QTextStream ts(reply);
        QString text = ts.readAll();

        int p1, p2;
        while ((p1 = text.indexOf(QLatin1String("<entry>"))) >= 0 && (p2 = text.indexOf(QLatin1String("</entry>"), p1)) >= 0) {
            parseCollectionXMLFragement(text.mid(p1, p2 - p1));
            text = text.left(p1) + text.mid(p2 + 8);
        }
        parseCollectionXMLFragement(text);

        if ((p1 = text.indexOf(QLatin1String("<link rel=\"next\" "))) >= 0 && (p1 = text.indexOf(QLatin1String(" href=\""), p1)) >= 0) {
            p2 = text.indexOf(QLatin1String("\""), p1 + 7);
            QString nextUrl = text.mid(p1 + 7, p2 - p1 - 7).append(QLatin1String("?format=atom"));
            if (!m_privateKey.isEmpty()) nextUrl.append("&key=").append(m_privateKey);

            fetchCollections(QUrl(nextUrl));
        }

        p1 = text.indexOf(QLatin1String("<link rel=\"first\" "));
        p1 = text.indexOf(QLatin1String(" href=\""), p1);
        p2 = text.indexOf(QLatin1String("\""), p1 + 7);
        QString itemsUrl = text.mid(p1 + 7, p2 - p1 - 7).replace(QLatin1String("/collections"), QLatin1String("/items")).append(QLatin1String("?format=atom"));
        if (!m_privateKey.isEmpty()) itemsUrl.append("&key=").append(m_privateKey);

        fetchItems(QUrl(itemsUrl));
    } else
        kDebug() << "Download for " << reply->url().toString() << "failed:" << reply->error() << reply->errorString();

    if (m_runningNetworkConn <= 0) {
        rebuildTables();

        emit busy(false);
        emit done(this);
    }
}

void ZoteroReadAPI::readItemsFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    --m_runningNetworkConn;

    if (reply->error() == QNetworkReply::NoError) {
        static XSLTransform xslt(KStandardDirs::locate("data", "kbibtex/zoteroitems2bibtex.xsl"));
        QTextStream ts(reply);
        QString xmlCode = ts.readAll();

        const QString bibTeXcode = xslt.transform(xmlCode);

        static FileImporterBibTeX importer;
        File *thisPageBibTeXFile = importer.fromString(bibTeXcode);
        bibTeXfile.append(*thisPageBibTeXFile);
        delete thisPageBibTeXFile;

        int p1 = -1;
        if ((p1 = xmlCode.indexOf(QLatin1String("<link rel=\"next\" "))) >= 0 && (p1 = xmlCode.indexOf(QLatin1String(" href=\""), p1)) >= 0) {
            int p2 = xmlCode.indexOf(QLatin1String("\""), p1 + 7);
            QString nextUrl = xmlCode.mid(p1 + 7, p2 - p1 - 7).append(QLatin1String("?format=atom"));
            if (!m_privateKey.isEmpty()) nextUrl.append("&key=").append(m_privateKey);

            fetchItems(QUrl(nextUrl));
        }
    } else
        kDebug() << "Download for " << reply->url().toString() << "failed:" << reply->error() << reply->errorString();

    if (m_runningNetworkConn <= 0) {
        rebuildTables();

        emit busy(false);
        emit done(this);
    }
}

void ZoteroReadAPI::readItemsInCollectionsFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    --m_runningNetworkConn;

    if (reply->error() == QNetworkReply::NoError) {
        QTextStream ts(reply);
        QString text = ts.readAll();

        int p1 = -1;
        if ((p1 = text.indexOf(QLatin1String("<link rel=\"self\""))) >= 0 && (p1 = text.indexOf(QLatin1String("collections/"), p1)) >= 0) {
            int p2 = text.indexOf(QLatin1String("/"), p1 + 12);
            const QString collectionId = text.mid(p1 + 12, p2 - p1 - 12);
            ZoteroCollection *collection = collectionTable.value(collectionId, NULL);
            if (collection != NULL) {
                p1 = -1;
                while ((p1 = text.indexOf(QLatin1String("<zapi:key>"), p1 + 1)) > 0) {
                    p2 = text.indexOf(QLatin1String("</zapi:key>"), p1);
                    const QString itemId = text.mid(p1 + 10, p2 - p1 - 10);
                    collection->items.append(itemId);
                }
            } else
                kDebug() << "No entry for collection" << collectionId << "in table";
        } else
            kDebug() << "Could not find self in result";
    }

    if (m_runningNetworkConn <= 0) {
        rebuildTables();

        emit busy(false);
        emit done(this);
    }
}

ZoteroReadAPICollectionsModel::ZoteroReadAPICollectionsModel(QObject *parent)
        : QAbstractItemModel(parent), m_zoteroReadAPI(NULL)
{
    // TODO
}

QModelIndex ZoteroReadAPICollectionsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_zoteroReadAPI == NULL)
        return QModelIndex();

    if (parent == QModelIndex()) {
        /// Root node
        return createIndex(row, column, 0);
    }

    const QStringList &childCollections = parent.internalId() == 0 ? m_zoteroReadAPI->collectionTable.value(QString::null, NULL)->childCollections : ((ZoteroReadAPI::ZoteroCollection*)parent.internalPointer())->childCollections;
    if (row >= 0 && row < childCollections.count())
        return createIndex(row, column, m_zoteroReadAPI->collectionTable.value(childCollections[row], NULL));

    return QModelIndex();
}

QModelIndex ZoteroReadAPICollectionsModel::parent(const QModelIndex &index) const
{
    if (m_zoteroReadAPI == NULL || index == QModelIndex() || index.internalId() == 0)
        return QModelIndex();

    const QString &parent = ((ZoteroReadAPI::ZoteroCollection*)index.internalPointer())->parent;
    if (parent.isEmpty())
        return createIndex(0, 0, 0);

    const QString &grandParent = m_zoteroReadAPI->collectionTable.value(parent, NULL)->parent;
    const QStringList &grandParentsChildCollections = m_zoteroReadAPI->collectionTable.value(grandParent, NULL)->childCollections;
    int row = grandParentsChildCollections.indexOf(parent);

    if (row >= 0)
        return createIndex(row, 0, m_zoteroReadAPI->collectionTable.value(parent, NULL));
    else
        kWarning() << "Cannot find" << parent << "(parent of" << ((ZoteroReadAPI::ZoteroCollection*)index.internalPointer())->self << ") in grandparent" << grandParent;

    return QModelIndex();
}

int ZoteroReadAPICollectionsModel::rowCount(const QModelIndex &parent) const
{
    if (m_zoteroReadAPI == NULL)
        return 0;
    if (parent == QModelIndex())
        return 1;

    const QString &parentKey = parent.internalId() == 0 ? QString::null : ((ZoteroReadAPI::ZoteroCollection*)parent.internalPointer())->self;
    ZoteroReadAPI::ZoteroCollection *parentCollection = m_zoteroReadAPI->collectionTable.value(parentKey, NULL);
    const QStringList parentsChildCollections = parentCollection == NULL ? QStringList() : parentCollection->childCollections;

    return parentsChildCollections.count();
}

int ZoteroReadAPICollectionsModel::columnCount(const QModelIndex &) const
{
    return 1;
}

bool ZoteroReadAPICollectionsModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

QVariant ZoteroReadAPICollectionsModel::data(const QModelIndex &index, int role) const
{
    if (m_zoteroReadAPI == NULL || index == QModelIndex())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (index.internalId() == 0)
            return i18n("My Library");
        if (index.internalPointer() != NULL)
            return ((ZoteroReadAPI::ZoteroCollection*)index.internalPointer())->title;
    } else if (role == Qt::DecorationRole) {
        if (index.internalId() == 0)
            return KIcon("go-home");
        if (index.internalPointer() != NULL)
            return KIcon("folder-yellow");
    } else if (role == FilterStringListRole) {
        if (index.internalId() == 0)
            return QStringList();
        if (index.internalPointer() != NULL) {
            if (((ZoteroReadAPI::ZoteroCollection*)index.internalPointer())->items.isEmpty())
                return QStringList() << QLatin1String("Something which is not a valid Zotero key");
            else
                return ((ZoteroReadAPI::ZoteroCollection*)index.internalPointer())->items;
        }
    }

    return QVariant();
}

QVariant ZoteroReadAPICollectionsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole || section != 0)
        return QVariant();

    return i18n("Collection");
}

void ZoteroReadAPICollectionsModel::setReadAPI(ZoteroReadAPI *zoteroReadAPI)
{
    m_zoteroReadAPI = zoteroReadAPI;
    reset();
}
