/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_PROGRAM_DOCUMENTLIST_H
#define KBIBTEX_PROGRAM_DOCUMENTLIST_H

#include <QListView>
#include <QAbstractListModel>
#include <QListWidgetItem>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QUrl>

#include "openfileinfo.h"

class KFileItem;

class OpenFileInfoManager;

class DocumentListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit DocumentListDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

};

class DocumentListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DocumentListModel(OpenFileInfo::StatusFlag statusFlag, QObject *parent = nullptr);
    ~DocumentListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

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
    DocumentListView(OpenFileInfo::StatusFlag statusFlag, QWidget *parent);
    ~DocumentListView() override;

private slots:
    void addToFavorites();
    void removeFromFavorites();
    void openFile();
    void openFileWithService(int i);
    void closeFile();

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

private:
    class DocumentListViewPrivate;
    DocumentListViewPrivate *d;
};

class DocumentList : public QTabWidget
{
    Q_OBJECT

public:
    enum Category { OpenFiles = 0, RecentFiles = 1, Favorites = 2 };

    explicit DocumentList(QWidget *parent = nullptr);

signals:
    void openFile(const QUrl &url);

private slots:
    void fileSelected(const KFileItem &item);

private:
    class DocumentListPrivate;
    DocumentListPrivate *d;
};

#endif // KBIBTEX_PROGRAM_DOCUMENTLIST_H
