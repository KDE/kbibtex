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

#ifndef KBIBTEX_NETWORKING_ZOTERO_READAPIMODEL_H
#define KBIBTEX_NETWORKING_ZOTERO_READAPIMODEL_H

#include "kbibtexnetworking_export.h"

#include "file.h"
#include <QAbstractItemModel>

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT ZoteroReadAPI : public QObject
{
    Q_OBJECT
public:
    static const QString configGroupNameZotero;
    static const QString keyUserID;
    static const QString defaultUserID;
    static const QString keyPrivateKey;
    static const QString defaultPrivateKey;
    static const QString zoteroApiUrlPrefix;

    struct ZoteroCollection {
        QString self, parent, title;
        QStringList childCollections;
        QStringList items;

        ZoteroCollection(const QString &_self, const QString &_parent, const QString &_title)
                : self(_self), parent(_parent), title(_title) { /* nothing */ }
    };

    QHash<QString, ZoteroCollection*> collectionTable;
    File bibTeXfile;

    ZoteroReadAPI(QObject *parent = NULL);

public slots:
    void scanLibrary();

signals:
    void busy(bool isBusy);
    void done(ZoteroReadAPI*);

private:
    QString m_userId, m_privateKey;
    int m_runningNetworkConn;

    void fetchCollections(const QUrl &url);
    void fetchItems(const QUrl &url);
    void fetchItemsInCollections(const QUrl &url);
    void rebuildTables();
    void cleanTables();

    void parseCollectionXMLFragement(const QString &xml);

private slots:
    void readCollectionsFinished();
    void readItemsFinished();
    void readItemsInCollectionsFinished();
};


/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT ZoteroReadAPICollectionsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    static const int FilterStringListRole;

    ZoteroReadAPICollectionsModel(QObject *parent = NULL);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;

public slots:
    void setReadAPI(ZoteroReadAPI*);

private:
    ZoteroReadAPI *m_zoteroReadAPI;
};

#endif // KBIBTEX_NETWORKING_ZOTERO_READAPIMODEL_H
