/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <QTabWidget>

#include <KListWidget>
#include <KUrl>

#include "openfileinfo.h"

namespace KBibTeX
{
namespace Program {

class OpenFileInfoManager;

class DocumentList : public QTabWidget
{
    Q_OBJECT
public:
    enum Category { OpenFiles = 0, RecentFiles = 1, Favorites = 2 };

    DocumentList(OpenFileInfoManager *openFileInfoManager, QWidget *parent = 0);

private slots:
    void itemExecuted(QListWidgetItem * item);
    void listsChanged(OpenFileInfo::StatusFlags statusFlags);

private:
    class DocumentListPrivate;
    DocumentListPrivate *d;
};

static const int RecentlyUsedItemType = QListWidgetItem::UserType + 23;
static const int FavoritesItemType = QListWidgetItem::UserType + 24;

class DocumentListItem : public QListWidgetItem
{
public:
    DocumentListItem(OpenFileInfo *openFileInfo, KListWidget *parent = NULL, int type = QListWidgetItem::UserType);
    OpenFileInfo *openFileInfo() const;

private:
    class DocumentListItemPrivate;
    DocumentListItemPrivate *d;
};

}
}

#endif // KBIBTEX_PROGRAM_DOCUMENTLIST_H
