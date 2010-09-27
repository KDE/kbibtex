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
#include <QSignalMapper>
#include <QGridLayout>
#include <QLabel>

#include <KDebug>
#include <KLocale>
#include <KGlobal>
#include <KIcon>
#include <KMimeType>
#include <KAction>

#include "documentlist.h"

class DocumentList::DocumentListPrivate
{
public:
    DocumentList *p;

    OpenFileInfoManager *openFileInfoManager;
    KListWidget *listOpenFiles;
    KListWidget *listRecentFiles;
    KListWidget *listFavorites;
    KAction *addFavListOpenFiles, *addFavListRecentFiles, *remFavListFavorites;
    KAction *closeListOpenFiles, *openListRecentFiles, *openListFavorites;

    DocumentListPrivate(OpenFileInfoManager *openFileInfoManager, DocumentList *p) {
        this->openFileInfoManager = openFileInfoManager;
        this->p = p;

        setupGui();
    }

    void setupGui() {
        QStringList overlays;

        listOpenFiles = new KListWidget(p);
        p->addTab(listOpenFiles, i18n("Open Files"));
        listOpenFiles->setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(listOpenFiles, SIGNAL(itemActivated(QListWidgetItem*)), p, SLOT(itemExecuted(QListWidgetItem*))); // FIXME Signal itemExecute(..) triggers *twice* ???
        connect(listOpenFiles, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), p, SLOT(updateContextMenu()));
        connect(listOpenFiles, SIGNAL(itemSelectionChanged()), p, SLOT(updateContextMenu()));

        listRecentFiles = new KListWidget(p);
        p->addTab(listRecentFiles, i18n("Recent Files"));
        listRecentFiles->setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(listRecentFiles, SIGNAL(itemActivated(QListWidgetItem*)), p, SLOT(itemExecuted(QListWidgetItem*))); // FIXME Signal itemExecute(..) triggers *twice* ???
        connect(listRecentFiles, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), p, SLOT(updateContextMenu()));
        connect(listRecentFiles, SIGNAL(itemSelectionChanged()), p, SLOT(updateContextMenu()));

        listFavorites = new KListWidget(p);
        p->addTab(listFavorites, i18n("Favorites"));
        listFavorites->setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(listFavorites, SIGNAL(itemActivated(QListWidgetItem*)), p, SLOT(itemExecuted(QListWidgetItem*))); // FIXME Signal itemExecute(..) triggers *twice* ???
        connect(listFavorites, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), p, SLOT(updateContextMenu()));
        connect(listFavorites, SIGNAL(itemSelectionChanged()), p, SLOT(updateContextMenu()));


        QSignalMapper *openCloseSignalMapper = new QSignalMapper(p);
        connect(openCloseSignalMapper, SIGNAL(mapped(QWidget*)), p, SLOT(closeFile(QWidget*)));

        closeListOpenFiles = new KAction(KIcon("document-close"), i18n("Close File"), p);
        connect(closeListOpenFiles, SIGNAL(triggered()), openCloseSignalMapper, SLOT(map()));
        openCloseSignalMapper->setMapping(closeListOpenFiles, listOpenFiles);
        listOpenFiles->addAction(closeListOpenFiles);

        openCloseSignalMapper = new QSignalMapper(p);
        connect(openCloseSignalMapper, SIGNAL(mapped(QWidget*)), p, SLOT(openFile(QWidget*)));

        openListRecentFiles = new KAction(KIcon("document-open"), i18n("Open File"), p);
        connect(openListRecentFiles, SIGNAL(triggered()), openCloseSignalMapper, SLOT(map()));
        openCloseSignalMapper->setMapping(openListRecentFiles, listRecentFiles);
        listRecentFiles->addAction(openListRecentFiles);

        openListFavorites = new KAction(KIcon("document-open"), i18n("Open File"), p);
        connect(openListFavorites, SIGNAL(triggered()), openCloseSignalMapper, SLOT(map()));
        openCloseSignalMapper->setMapping(openListFavorites, listFavorites);
        listFavorites->addAction(openListFavorites);


        QSignalMapper *favoritesSignalMapper = new QSignalMapper(p);
        connect(favoritesSignalMapper, SIGNAL(mapped(QWidget*)), p, SLOT(addToFavorites(QWidget*)));

        overlays.clear();
        overlays << "list-add";
        addFavListOpenFiles = new KAction(KIcon("favorites", NULL, overlays), i18n("Add to Favorites"), p);
        connect(addFavListOpenFiles, SIGNAL(triggered()), favoritesSignalMapper, SLOT(map()));
        favoritesSignalMapper->setMapping(addFavListOpenFiles, listOpenFiles);
        listOpenFiles->addAction(addFavListOpenFiles);

        addFavListRecentFiles = new KAction(KIcon("favorites", NULL, overlays), i18n("Add to Favorites"), p);
        connect(addFavListRecentFiles, SIGNAL(triggered()), favoritesSignalMapper, SLOT(map()));
        favoritesSignalMapper->setMapping(addFavListRecentFiles, listRecentFiles);
        listRecentFiles->addAction(addFavListRecentFiles);

        favoritesSignalMapper = new QSignalMapper(p);
        connect(favoritesSignalMapper, SIGNAL(mapped(QWidget*)), p, SLOT(removeFromFavorites(QWidget*)));

        overlays.clear();
        overlays << "list-remove";
        remFavListFavorites = new KAction(KIcon("favorites", NULL, overlays), i18n("Remove from Favorites"), p);
        connect(remFavListFavorites, SIGNAL(triggered()), favoritesSignalMapper, SLOT(map()));
        favoritesSignalMapper->setMapping(remFavListFavorites, listFavorites);
        listFavorites->addAction(remFavListFavorites);

        connect(openFileInfoManager, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), p, SLOT(listsChanged(OpenFileInfo::StatusFlags)));

        /** set minimum width of widget depending on tab's text width */
        QFontMetrics fm(p->font());
        p->setMinimumWidth(fm.width(p->tabText(0))*(p->count() + 1));
    }

    void updateList(Category category) {
        /// determine list widget and status flags for given category
        KListWidget *listWidget = category == DocumentList::OpenFiles ? listOpenFiles : category == DocumentList::RecentFiles ? listRecentFiles : listFavorites;
        OpenFileInfo::StatusFlags statusFlags = category == DocumentList::OpenFiles ? OpenFileInfo::Open : category == DocumentList::RecentFiles ? OpenFileInfo::RecentlyUsed : OpenFileInfo::Favorite;
        /// get list of file handles where status flags are set accordingly

        QList<OpenFileInfo*> ofiList = openFileInfoManager->filteredItems(statusFlags);
        //OpenFileInfo* currentFile = openFileInfoManager->currentFile();

        QString currentItemText = listWidget->currentItem() != NULL ? listWidget->currentItem()->text() : QString::null;
        listWidget->clear();
        int h = 1;
        for (QList<OpenFileInfo*>::Iterator it = ofiList.begin(); it != ofiList.end(); ++it) {
            OpenFileInfo *ofi = *it;
            DocumentListItem *item = new DocumentListItem(ofi);
            QWidget *w = new DocumentListItemWidget(item, listWidget);
            item->setSizeHint(w->sizeHint());
            h = qMax(h, w->height());

            listWidget->addItem(item);
            listWidget->setItemSelected(item, item->text() == currentItemText || openFileInfoManager->currentFile() == ofi);
            if (item->text() == currentItemText || openFileInfoManager->currentFile() == ofi)
                listWidget->setCurrentItem(item);

            listWidget->setItemWidget(item, w);
        }
        listWidget->setIconSize(QSize(h*3 / 4, h*3 / 4));
    }

    void itemExecuted(QListWidgetItem * item) {
        if (item != NULL) {
            const DocumentListItem *dli = static_cast<const DocumentListItem*>(item);
            openFileInfoManager->setCurrentFile(dli->openFileInfo());
        }
    }

    void listsChanged(OpenFileInfo::StatusFlags statusFlags) {
        Q_UNUSED(statusFlags);

        updateList(OpenFiles);
        updateList(RecentFiles);
        updateList(Favorites);

        updateContextMenu();
    }

    void updateContextMenu() {
        const DocumentListItem *dli = static_cast<const DocumentListItem*>(listOpenFiles->currentItem());
        if (dli == NULL && !listOpenFiles->selectedItems().isEmpty()) dli = static_cast<const DocumentListItem*>(listOpenFiles->selectedItems().first());
        addFavListOpenFiles->setEnabled(!listOpenFiles->selectedItems().isEmpty() && !dli->openFileInfo()->flags().testFlag(OpenFileInfo::Favorite));
        closeListOpenFiles->setEnabled(!listOpenFiles->selectedItems().isEmpty());

        dli = static_cast<const DocumentListItem*>(listRecentFiles->currentItem());
        if (dli == NULL && !listRecentFiles->selectedItems().isEmpty()) dli = static_cast<const DocumentListItem*>(listRecentFiles->selectedItems().first());
        addFavListRecentFiles->setEnabled(!listRecentFiles->selectedItems().isEmpty() && !dli->openFileInfo()->flags().testFlag(OpenFileInfo::Favorite));
        openListRecentFiles->setEnabled(!listRecentFiles->selectedItems().isEmpty() && !dli->openFileInfo()->flags().testFlag(OpenFileInfo::Open));

        dli = static_cast<const DocumentListItem*>(listFavorites->currentItem());
        if (dli == NULL && !listFavorites->selectedItems().isEmpty()) dli = static_cast<const DocumentListItem*>(listFavorites->selectedItems().first());
        remFavListFavorites->setEnabled(!listFavorites->selectedItems().isEmpty());
        openListFavorites->setEnabled(!listFavorites->selectedItems().isEmpty() && !dli->openFileInfo()->flags().testFlag(OpenFileInfo::Open));
    }
};

DocumentList::DocumentList(OpenFileInfoManager *openFileInfoManager, QWidget *parent)
        : QTabWidget(parent), d(new DocumentListPrivate(openFileInfoManager, this))
{
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

void DocumentList::addToFavorites(QWidget *w)
{
    KListWidget *list = static_cast<KListWidget*>(w);
    QListWidgetItem * item = list->currentItem();
    if (item != NULL) {
        const DocumentListItem *dli = static_cast<const DocumentListItem*>(item);
        dli->openFileInfo()->addFlags(OpenFileInfo::Favorite);
    }
}

void DocumentList::removeFromFavorites(QWidget *w)
{
    KListWidget *list = static_cast<KListWidget*>(w);
    QListWidgetItem * item = list->currentItem();
    if (item != NULL) {
        const DocumentListItem *dli = static_cast<const DocumentListItem*>(item);
        dli->openFileInfo()->removeFlags(OpenFileInfo::Favorite);
    }
}

void DocumentList::openFile(QWidget *w)
{
    KListWidget *list = static_cast<KListWidget*>(w);
    QListWidgetItem * item = list->currentItem();
    if (item != NULL) {
        const DocumentListItem *dli = static_cast<const DocumentListItem*>(item);
        OpenFileInfoManager::getOpenFileInfoManager()->setCurrentFile(dli->openFileInfo());
    }
}

void DocumentList::closeFile(QWidget *w)
{
    KListWidget *list = static_cast<KListWidget*>(w);
    QListWidgetItem * item = list->currentItem();
    if (item != NULL) {
        const DocumentListItem *dli = static_cast<const DocumentListItem*>(item);
        OpenFileInfoManager::getOpenFileInfoManager()->close(dli->openFileInfo());
    }
}
void DocumentList::updateContextMenu()
{
    d->updateContextMenu();
}

class DocumentListItem::DocumentListItemPrivate
{
public:
    DocumentListItem *p;
    OpenFileInfo *openFileInfo;

    DocumentListItemPrivate(OpenFileInfo *openFileInfo, DocumentListItem *p) {
        this->openFileInfo = openFileInfo;
        this->p = p;
    }
};

DocumentListItem::DocumentListItem(OpenFileInfo *openFileInfo, KListWidget *parent, int type)
        : QListWidgetItem(parent, type), d(new DocumentListItemPrivate(openFileInfo, this))
{
    //setText(openFileInfo->shortCaption());
    setToolTip(openFileInfo->fullCaption());

    /// determine mime type-based icon and overlays (e.g. for modified files)
    QStringList overlays;
    QString iconName = openFileInfo->mimeType().replace("/", "-");
    if (openFileInfo->flags().testFlag(OpenFileInfo::IsModified))
        overlays << "document-save";
    if (openFileInfo->flags().testFlag(OpenFileInfo::Favorite))
        overlays << "favorites";
    if (openFileInfo->flags().testFlag(OpenFileInfo::RecentlyUsed))
        overlays << "clock";
    if (openFileInfo->flags().testFlag(OpenFileInfo::Open))
        overlays << "folder-open";
    setIcon(KIcon(iconName, NULL, overlays));
}

OpenFileInfo *DocumentListItem::openFileInfo() const
{
    return d->openFileInfo;
}

DocumentListItemWidget::DocumentListItemWidget(DocumentListItem *item, QWidget *parent)
        : QWidget(parent), m_item(item)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    QLabel *label = new QLabel(item->openFileInfo()->shortCaption());
    layout->addWidget(label, 0, 1, 1, 1);
    label = new QLabel(item->openFileInfo()->fullCaption());
    QFont font = label->font();
    font.setPointSize(font.pointSize()*3 / 4);
    label->setFont(font);
    layout->addWidget(label, 1, 1, 1, 1);

    label->updateGeometry();
}
