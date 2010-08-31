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

#include <QFontMetrics>
#include <QStringListModel>
#include <QTimer>

#include <KDebug>
#include <KLocale>
#include <KGlobal>
#include <KIcon>
#include <KMimeType>

#include "documentlist.h"

class DocumentList::DocumentListPrivate
{
public:
    DocumentList *p;

    OpenFileInfoManager *openFileInfoManager;
    KListWidget *listOpenFiles;
    KListWidget *listRecentFiles;
    KListWidget *listFavorites;

    DocumentListPrivate(DocumentList *p) {
        this->p = p;
    }

    void setupGui() {
        listOpenFiles = new KListWidget(p);
        p->addTab(listOpenFiles, i18n("Open Files"));
        connect(listOpenFiles, SIGNAL(itemDoubleClicked(QListWidgetItem*)), p, SLOT(itemExecuted(QListWidgetItem*))); // FIXME Signal itemExecute(..) triggers *twice* ???
        listRecentFiles = new KListWidget(p);
        p->addTab(listRecentFiles, i18n("Recent Files"));
        connect(listRecentFiles, SIGNAL(itemDoubleClicked(QListWidgetItem*)), p, SLOT(itemExecuted(QListWidgetItem*))); // FIXME Signal itemExecute(..) triggers *twice* ???
        listFavorites = new KListWidget(p);
        p->addTab(listFavorites, i18n("Favorites"));
        connect(listFavorites, SIGNAL(itemDoubleClicked(QListWidgetItem*)), p, SLOT(itemExecuted(QListWidgetItem*))); // FIXME Signal itemExecute(..) triggers *twice* ???

        /** set minimum width of widget depending on tab's text width */
        QFontMetrics fm(p->font());
        p->setMinimumWidth(fm.width(p->tabText(0))*(p->count() + 1));
    }

    void updateList(Category category) {
        KListWidget *listWidget = category == DocumentList::OpenFiles ? listOpenFiles : category == DocumentList::RecentFiles ? listRecentFiles : listFavorites;
        OpenFileInfo::StatusFlags statusFlags = category == DocumentList::OpenFiles ? OpenFileInfo::Open : category == DocumentList::RecentFiles ? OpenFileInfo::RecentlyUsed : OpenFileInfo::Favorite;
        QList<OpenFileInfo*> ofiList = openFileInfoManager->filteredItems(statusFlags);

        listWidget->clear();
        for (QList<OpenFileInfo*>::Iterator it = ofiList.begin(); it != ofiList.end(); ++it) {
            OpenFileInfo *ofi = *it;
            DocumentListItem *item = new DocumentListItem(ofi);

            listWidget->addItem(item);
            listWidget->setItemSelected(item, openFileInfoManager->currentFile() == ofi);

            KUrl url(ofi->fullCaption());
            if (url.isValid()) {
                KIcon icon(KMimeType::iconNameForUrl(url));
                item->setIcon(icon);
            }
        }
    }

    void itemExecuted(QListWidgetItem * item) {
        const DocumentListItem *dli = dynamic_cast<const DocumentListItem*>(item);
        if (item != NULL) {
            kDebug() << "itemExecuted on " << dli->openFileInfo()->fullCaption() << endl;
            openFileInfoManager->setCurrentFile(dli->openFileInfo());
        }
    }

    void listsChanged(OpenFileInfo::StatusFlags statusFlags) {
        if (statusFlags.testFlag(OpenFileInfo::Open))
            updateList(OpenFiles);
        if (statusFlags.testFlag(OpenFileInfo::RecentlyUsed))
            updateList(RecentFiles);
        if (statusFlags.testFlag(OpenFileInfo::Favorite))
            updateList(Favorites);
    }
};

DocumentList::DocumentList(OpenFileInfoManager *openFileInfoManager, QWidget *parent)
        : QTabWidget(parent), d(new DocumentListPrivate(this))
{
    d->openFileInfoManager = openFileInfoManager;
    connect(openFileInfoManager, SIGNAL(listsChanged(OpenFileInfo::StatusFlags)), this, SLOT(listsChanged(OpenFileInfo::StatusFlags)));
    d->setupGui();
    QFlags<OpenFileInfo::StatusFlag> flags = OpenFileInfo::RecentlyUsed;
    flags |= OpenFileInfo::Favorite;
    d->listsChanged(flags);
}

void DocumentList::itemExecuted(QListWidgetItem * item)
{
    d->itemExecuted(item);
}

void DocumentList::listsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    d->listsChanged(statusFlags);
}

class DocumentListItem::DocumentListItemPrivate
{
public:
    DocumentListItem *p;
    OpenFileInfo *openFileInfo;

    DocumentListItemPrivate(DocumentListItem *p) {
        this->p = p;
    }
};

DocumentListItem::DocumentListItem(OpenFileInfo *openFileInfo, KListWidget *parent, int type)
        : QListWidgetItem(parent, type), d(new DocumentListItemPrivate(this))
{
    setText(openFileInfo->caption());
    setToolTip(openFileInfo->fullCaption());

    d->openFileInfo = openFileInfo;
}

OpenFileInfo *DocumentListItem::openFileInfo() const
{
    return d->openFileInfo;
}
