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
#include <QTimer>

#include <KDebug>
#include <KLocale>
#include <KGlobal>
#include <KConfig>
#include <KConfigGroup>

#include "documentlist.h"

using namespace KBibTeX::Program;

DocumentList::DocumentList(QWidget *parent)
        : QTabWidget(parent), m_rowToMoveUpInternallyRecentlyUsed(-1)
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

    readConfig();
}

DocumentList::~DocumentList()
{
    writeConfig();
}

void DocumentList::addToOpen(const KUrl &url, const QString& encoding)
{
    /** check if given URL is already contained in Open Files list*/
    DocumentListItem *item = NULL;
    for (int i = m_listOpenFiles->count() - 1; item == NULL && i >= 0; --i) {
        item = dynamic_cast<DocumentListItem*>(m_listOpenFiles->item(i));
        if (!url.equals(item->url())) item = NULL;
        else item->setEncoding(encoding); //< update encoding
    }

    if (item == NULL) {
        /** file was not already in Open Files list */
        m_listOpenFiles->addItem(new DocumentListItem(url, encoding));
    }

    addToRecentFiles(url, encoding);
}

void DocumentList::addToRecentFiles(const KUrl &url, const QString& encoding)
{
    /** check if given URL is already contained in Recent Files list*/
    DocumentListItem *item = NULL;
    for (int i = m_listRecentFiles->count() - 1; item == NULL && i >= 0; --i) {
        item = dynamic_cast<DocumentListItem*>(m_listRecentFiles->item(i));
        if (!url.equals(item->url())) item = NULL;
        else item->setEncoding(encoding); //< update encoding
    }

    if (item == NULL) {
        /** file was not already in Open Files list */
        item = new DocumentListItem(url, encoding);
        m_listRecentFiles->insertItem(0, item);
    } else if (m_rowToMoveUpInternallyRecentlyUsed==-1) {
        /** move existing item up to first position */
        m_rowToMoveUpInternallyRecentlyUsed = m_listRecentFiles->row(item);
        QTimer::singleShot(300, this, SLOT(moveUpInternallyRecentlyUsed()));
    }
}

void DocumentList::moveUpInternallyRecentlyUsed()
{
    QListWidgetItem *item = m_listRecentFiles->item(m_rowToMoveUpInternallyRecentlyUsed);
    m_listRecentFiles->takeItem(m_rowToMoveUpInternallyRecentlyUsed);
    m_listRecentFiles->insertItem(0, item);
    m_listRecentFiles->setCurrentRow(0);
    m_rowToMoveUpInternallyRecentlyUsed = -1;
}

void DocumentList::itemExecuted(QListWidgetItem * item)
{
    DocumentListItem *dli = dynamic_cast<DocumentListItem*>(item);
    if (dli != NULL) {
        KUrl url = dli->url();
        QString encoding = dli->encoding();
        addToOpen(url, encoding);
        emit open(url, encoding);
    }
}

const QString DocumentList::configGroupNameRecentlyUsed = QLatin1String("DocumentList-RecentlyUsed");
const QString DocumentList::configGroupNameFavorites = QLatin1String("DocumentList-Favorites");
const int DocumentList::maxNumRecentlyUsedFiles = 32;

void DocumentList::readConfig()
{
    readConfig(m_listRecentFiles, configGroupNameRecentlyUsed);
    readConfig(m_listFavorites, configGroupNameFavorites);
}

void DocumentList::writeConfig()
{
    writeConfig(m_listRecentFiles, configGroupNameRecentlyUsed);
    writeConfig(m_listFavorites, configGroupNameFavorites);
}

void DocumentList::readConfig(KListWidget *fromList, const QString& configGroupName)
{
    KSharedConfig::Ptr config = KGlobal::config();

    fromList->clear();
    KConfigGroup cg(config, configGroupName);
    for (int i = 0; i < maxNumRecentlyUsedFiles; ++i) {
        QString fileUrl = cg.readEntry(QString("URL-%1").arg(i), "");
        if (fileUrl == "") break;
        QString encoding = cg.readEntry(QString("Encoding-%1").arg(i), "");
        fromList->addItem(new DocumentListItem(KUrl(fileUrl), encoding, m_listRecentFiles, RecentlyUsedItemType));
    }
}

void DocumentList::writeConfig(KListWidget *fromList, const QString& configGroupName)
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cg(config, configGroupName);
    for (int i = qMin(maxNumRecentlyUsedFiles, fromList->count()) - 1; i >= 0; --i) {
        DocumentListItem *item = dynamic_cast<DocumentListItem*>(fromList->item(i));
        if (item == NULL) break;
        cg.writeEntry(QString("URL-%1").arg(i), item->url().prettyUrl());
        cg.writeEntry(QString("Encoding-%1").arg(i), item->encoding());
    }
    config->sync();
}

DocumentListItem::DocumentListItem(const KUrl &url, const QString &encoding, KListWidget *parent, int type)
        : QListWidgetItem(parent, type), m_url(url), m_encoding(encoding)
{
    setText(url.fileName());
    setToolTip(url.prettyUrl());
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
