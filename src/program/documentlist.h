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

    void add(const KUrl &url, const QString& encoding, Category category = OpenFiles);

signals:
    void open(const KUrl &url, const QString& encoding);

private slots:
    void itemExecuted(QListWidgetItem * item);

private:
    KListWidget *m_listOpenFiles;
    KListWidget *m_listRecentFiles;
    KListWidget *m_listFavorites;
};

class DocumentListItem : public QListWidgetItem
{
public:
    DocumentListItem(const KUrl &url, const QString &encoding, KListWidget *parent, int type = QListWidgetItem::Type);

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
