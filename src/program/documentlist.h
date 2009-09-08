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

namespace KBibTeX
{
namespace Program {

class DocumentList : public QTabWidget
{
    Q_OBJECT
public:
    enum Category { OpenFiles = 0, RecentFiles = 1, Favorites = 2 };

    DocumentList(QWidget *parent = 0);
    virtual ~DocumentList();

    void addToOpen(const KUrl &url, const QString& encoding);
    void closeUrl(const KUrl &url);
    void highlightUrl(const KUrl &url);

signals:
    void open(const KUrl &url, const QString& encoding);

private slots:
    void itemExecuted(QListWidgetItem * item);

protected:
    static const QString configGroupNameRecentlyUsed;
    static const QString configGroupNameFavorites;
    static const int maxNumRecentlyUsedFiles;

    void addToRecentFiles(const KUrl &url, const QString& encoding);
    void readConfig();
    void writeConfig();
    void highlightUrl(const KUrl &url, KListWidget *list);

private:
    KListWidget *m_listOpenFiles;
    KListWidget *m_listRecentFiles;
    KListWidget *m_listFavorites;

    void readConfig(KListWidget *fromList, const QString& configGroupName);
    void writeConfig(KListWidget *fromList, const QString& configGroupName);
    void refreshRecentlyUsed(const KUrl& url);
    void refreshOpenFiles(const KUrl& url);

    int m_rowToMoveUpInternallyRecentlyUsed;

private slots:
    void moveUpInternallyRecentlyUsed();
};

static const int RecentlyUsedItemType = QListWidgetItem::UserType + 23;
static const int FavoritesItemType = QListWidgetItem::UserType + 24;

class DocumentListItem : public QListWidgetItem
{
public:
    DocumentListItem(const KUrl &url, const QString &encoding, KListWidget *parent = NULL, int type = QListWidgetItem::UserType);

    const KUrl url();
    const QString encoding();
    void setEncoding(const QString &encoding);

protected:
    KUrl m_url;
    QString m_encoding;
};

}
}

#endif // KBIBTEX_PROGRAM_DOCUMENTLIST_H
