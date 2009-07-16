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

#include <QStringListModel>

#include <KDebug>
#include <KLocale>

#include "documentlist.h"

using namespace KBibTeX::Program;

DocumentList::DocumentList(QWidget *parent)
        : QTabWidget(parent)
{
    m_listOpenFiles = new KListWidget(this);
    addTab(m_listOpenFiles, i18n("Open Files"));
    connect(m_listOpenFiles, SIGNAL(executed(QListWidgetItem*)), this, SLOT(itemExecuted(QListWidgetItem*)));
    m_listRecentFiles = new KListWidget(this);
    addTab(m_listRecentFiles, i18n("Recent Files"));
    connect(m_listRecentFiles, SIGNAL(executed(QListWidgetItem*)), this, SLOT(itemExecuted(QListWidgetItem*)));
    m_listFavorites = new KListWidget(this);
    addTab(m_listFavorites, i18n("Favorites"));
    connect(m_listFavorites, SIGNAL(executed(QListWidgetItem*)), this, SLOT(itemExecuted(QListWidgetItem*)));
}

DocumentList::~DocumentList()
{
    //nothing
}

void DocumentList::add(const KUrl &url, const QString& encoding, Category category)
{
    KListWidget *list = NULL;
    switch (category) {
    case OpenFiles: list=m_listOpenFiles; break;
    case RecentFiles: list=m_listRecentFiles; break;
    case Favorites: list=m_listFavorites; break;
    }

    bool match = false;
    DocumentListItem *it = NULL;
    for (int i = list->count() - 1; !match && i >= 0; --i) {
        it = dynamic_cast<DocumentListItem*>(list->item(i));
        match = url.equals(it->url());
    }

    if (match && it != NULL)
        it->setEncoding(encoding);
    else if (!match) {
        it = new DocumentListItem(url, encoding, list);
    }
    if (it != NULL)
        list->setCurrentItem(it);

    if (category != RecentFiles)
        add(url, encoding, RecentFiles);
}

void DocumentList::itemExecuted(QListWidgetItem * item)
{
    DocumentListItem *it = dynamic_cast<DocumentListItem*>(item);
    if (it != NULL) {
        KUrl url = it->url();
        QString encoding = it->encoding();
        emit open(url, encoding);
    }
}

DocumentListItem::DocumentListItem(const KUrl &url, const QString &encoding, KListWidget *parent, int type)
        : QListWidgetItem(parent, type), m_url(url), m_encoding(encoding)
{
    setText(url.fileName());
}

const KUrl DocumentListItem::url()
{
    return m_url;
}

const QString DocumentListItem::encoding()
{
    return m_encoding;
}

void DocumentListItem::setEncoding(const QString &encoding)
{
    m_encoding = encoding;
}
