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

#ifndef KBIBTEX_PROGRAM_DOCUMENTLIST_H
#define KBIBTEX_PROGRAM_DOCUMENTLIST_H

#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>

#include <KTabWidget>
#include <KListWidget>
#include <KUrl>

#include "openfileinfo.h"

class KFileItem;

class OpenFileInfoManager;

class DocumentListDelegate : public QStyledItemDelegate
{
private:
    OpenFileInfoManager *ofim;

public:
    DocumentListDelegate(OpenFileInfoManager *openFileInfoManager, QObject *parent = NULL);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

class DocumentListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    DocumentListModel(OpenFileInfo::StatusFlag statusFlag, OpenFileInfoManager *openFileInfoManager, QObject *parent = NULL);
    ~DocumentListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    class DocumentListModelPrivate;
    DocumentListModelPrivate *d;

private slots:
    void listsChanged(OpenFileInfo::StatusFlags statusFlags);
};

class DocumentListView : public QListView
{
    Q_OBJECT

public:
    DocumentListView(OpenFileInfoManager *openFileInfoManager, OpenFileInfo::StatusFlag statusFlag, QWidget *parent);
    ~DocumentListView();

private slots:
    void addToFavorites();
    void removeFromFavorites();
    void openFile();
    void openFileWithService(int i);
    void closeFile();

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    class DocumentListViewPrivate;
    DocumentListViewPrivate *d;
};

class DocumentList : public KTabWidget
{
    Q_OBJECT

public:
    enum Category { OpenFiles = 0, RecentFiles = 1, Favorites = 2 };

    DocumentList(OpenFileInfoManager *openFileInfoManager, QWidget *parent = NULL);

signals:
    void openFile(const KUrl &url);

private slots:
    void fileSelected(const KFileItem &item);

private:
    class DocumentListPrivate;
    DocumentListPrivate *d;
};

static const int RecentlyUsedItemType = QListWidgetItem::UserType + 23;
static const int FavoritesItemType = QListWidgetItem::UserType + 24;


#endif // KBIBTEX_PROGRAM_DOCUMENTLIST_H
