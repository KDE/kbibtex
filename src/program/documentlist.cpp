/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "documentlist.h"

#include <QStringListModel>
#include <QTimer>
#include <QSignalMapper>
#include <QGridLayout>
#include <QMessageBox>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QAction>

#include <KIconLoader>
#include <KLocalizedString>
#include <KActionMenu>
#include <KDirOperator>
#include <KService>

#include "kbibtexnamespace.h"

class DirOperatorWidget : public QWidget
{
public:
    KDirOperator *dirOperator;

    DirOperatorWidget(QWidget *parent)
            : QWidget(parent) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);
        layout->setColumnStretch(0, 0);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 1);

        QPushButton *buttonUp = new QPushButton(QIcon::fromTheme("go-up"), "", this);
        buttonUp->setToolTip(i18n("One level up"));
        layout->addWidget(buttonUp, 0, 0, 1, 1);

        QPushButton *buttonHome = new QPushButton(QIcon::fromTheme("user-home"), "", this);
        buttonHome->setToolTip(i18n("Go to Home folder"));
        layout->addWidget(buttonHome, 0, 1, 1, 1);

        dirOperator = new KDirOperator(QUrl("file:" + QDir::homePath()), this);
        layout->addWidget(dirOperator, 1, 0, 1, 3);
        dirOperator->setView(KFile::Detail);

        connect(buttonUp, SIGNAL(clicked()), dirOperator, SLOT(cdUp()));
        connect(buttonHome, SIGNAL(clicked()), dirOperator, SLOT(home()));
    }
};

DocumentListDelegate::DocumentListDelegate(OpenFileInfoManager *openFileInfoManager, QObject *parent)
        : QStyledItemDelegate(parent), ofim(openFileInfoManager)
{
    // nothing
}

void DocumentListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int height = option.rect.height();

    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlightedText().color()));
    } else {
        painter->setPen(QPen(option.palette.text().color()));
    }

    OpenFileInfo *ofi = qvariant_cast<OpenFileInfo *>(index.data(Qt::UserRole));

    if (ofim->currentFile() == ofi) {
        /// for the currently open file, use a bold font to write file name
        QFont font = painter->font();
        font.setBold(true);
        painter->setFont(font);
    }

    QRect textRect = option.rect;
    textRect.setLeft(textRect.left() + height + 4);
    textRect.setHeight(height / 2);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());

    textRect = option.rect;
    textRect.setLeft(textRect.left() + height + 4);
    textRect.setTop(textRect.top() + height / 2);
    textRect.setHeight(height * 3 / 8);
    QFont font = painter->font();
    font.setPointSize(font.pointSize() * 7 / 8);
    painter->setFont(font);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, ofi->fullCaption());

    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    painter->drawPixmap(option.rect.left() + 1, option.rect.top() + 1, height - 2, height - 2, icon.pixmap(height - 2, height - 2));

    painter->restore();

}

QSize DocumentListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(size.height() * 9 / 4);
    return size;
}

class DocumentListModel::DocumentListModelPrivate
{
public:
    OpenFileInfo::StatusFlag sf;
    OpenFileInfoManager *ofim;
    OpenFileInfoManager::OpenFileInfoList ofiList;

private:
    // UNUSED DocumentListModel *p;

public:
    DocumentListModelPrivate(OpenFileInfo::StatusFlag statusFlag, OpenFileInfoManager *openFileInfoManager, DocumentListModel */* UNUSED parent*/)
            : sf(statusFlag), ofim(openFileInfoManager)// UNUSED , p(parent)
    {
        /// nothing
    }
};

DocumentListModel::DocumentListModel(OpenFileInfo::StatusFlag statusFlag, OpenFileInfoManager *openFileInfoManager, QObject *parent)
        : QAbstractListModel(parent), d(new DocumentListModel::DocumentListModelPrivate(statusFlag, openFileInfoManager, this))
{
    listsChanged(d->sf);

    connect(openFileInfoManager, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SLOT(listsChanged(OpenFileInfo::StatusFlags)));
}

DocumentListModel::~DocumentListModel()
{
    delete d;
}

int DocumentListModel::rowCount(const QModelIndex &parent) const
{
    if (parent != QModelIndex()) return 0;
    return d->ofiList.count();
}

QVariant DocumentListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

    OpenFileInfo *openFileInfo = d->ofiList[index.row()];

    switch (role) {
    case Qt::DisplayRole: return openFileInfo->shortCaption();
    case Qt::DecorationRole: {
        /// determine mime type-based icon and overlays (e.g. for modified files)
        QStringList overlays;
        QString iconName = openFileInfo->mimeType().replace(QLatin1Char('/'), QLatin1Char('-'));
        if (openFileInfo->flags().testFlag(OpenFileInfo::Favorite))
            overlays << "favorites";
        else
            overlays << "";
        if (openFileInfo->flags().testFlag(OpenFileInfo::RecentlyUsed))
            overlays << "clock";
        else
            overlays << "";
        if (openFileInfo->flags().testFlag(OpenFileInfo::Open))
            overlays << "folder-open";
        else
            overlays << "";
        if (openFileInfo->isModified())
            overlays << "document-save";
        else
            overlays << "";
        return KDE::icon(iconName, overlays, 0);
    }
    case Qt::ToolTipRole: return squeeze_text(openFileInfo->fullCaption(), 64);
    case Qt::UserRole: return qVariantFromValue(openFileInfo);
    default: return QVariant();
    }
}

QVariant DocumentListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || section != 0 || role != Qt::DisplayRole) return QVariant();
    return QVariant("List of Files");
}

void DocumentListModel::listsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(d->sf)) {
        beginResetModel();
        d->ofiList = d->ofim->filteredItems(d->sf);
        endResetModel();
    }
}

class DocumentListView::DocumentListViewPrivate
{
private:
    DocumentListView *p;

public:
    OpenFileInfoManager *ofim;
    QAction *actionAddToFav, *actionRemFromFav;
    QAction *actionCloseFile, *actionOpenFile;
    KActionMenu *actionOpenMenu;
    QList<QAction *> openMenuActions;
    KService::List openMenuServices;
    QSignalMapper openMenuSignalMapper;

    DocumentListViewPrivate(DocumentListView *parent, OpenFileInfoManager *openFileInfoManager)
            : p(parent), ofim(openFileInfoManager), actionAddToFav(NULL), actionRemFromFav(NULL), actionCloseFile(NULL), actionOpenFile(NULL) {
        connect(&openMenuSignalMapper, SIGNAL(mapped(int)), p, SLOT(openFileWithService(int)));
    }
};

DocumentListView::DocumentListView(OpenFileInfoManager *openFileInfoManager, OpenFileInfo::StatusFlag statusFlag, QWidget *parent)
        : QListView(parent), d(new DocumentListViewPrivate(this, openFileInfoManager))
{
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setItemDelegate(new DocumentListDelegate(openFileInfoManager, this));

    if (statusFlag == OpenFileInfo::Open) {
        d->actionCloseFile = new QAction(QIcon::fromTheme("document-close"), i18n("Close File"), this);
        connect(d->actionCloseFile, SIGNAL(triggered()), this, SLOT(closeFile()));
        addAction(d->actionCloseFile);
    } else {
        d->actionOpenFile = new QAction(QIcon::fromTheme("document-open"), i18n("Open File"), this);
        connect(d->actionOpenFile, SIGNAL(triggered()), this, SLOT(openFile()));
        addAction(d->actionOpenFile);
    }

    d->actionOpenMenu = new KActionMenu(QIcon::fromTheme("document-open"), i18n("Open with"), this);
    addAction(d->actionOpenMenu);

    if (statusFlag == OpenFileInfo::Favorite) {
        d->actionRemFromFav = new QAction(QIcon::fromTheme("favorites"), i18n("Remove from Favorites"), this);
        connect(d->actionRemFromFav, SIGNAL(triggered()), this, SLOT(removeFromFavorites()));
        addAction(d->actionRemFromFav);
    } else {
        d->actionAddToFav = new QAction(QIcon::fromTheme("favorites"), i18n("Add to Favorites"), this);
        connect(d->actionAddToFav, SIGNAL(triggered()), this, SLOT(addToFavorites()));
        addAction(d->actionAddToFav);
    }

    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(openFile()));

    currentChanged(QModelIndex(), QModelIndex());
}

DocumentListView::~DocumentListView()
{
    delete d;
}

void DocumentListView::addToFavorites()
{
    QModelIndex modelIndex = currentIndex();
    if (modelIndex != QModelIndex()) {
        OpenFileInfo *ofi = qvariant_cast<OpenFileInfo *>(modelIndex.data(Qt::UserRole));
        ofi->addFlags(OpenFileInfo::Favorite);
    }
}

void DocumentListView::removeFromFavorites()
{
    QModelIndex modelIndex = currentIndex();
    if (modelIndex != QModelIndex()) {
        OpenFileInfo *ofi = qvariant_cast<OpenFileInfo *>(modelIndex.data(Qt::UserRole));
        ofi->removeFlags(OpenFileInfo::Favorite);
    }
}

void DocumentListView::openFile()
{
    QModelIndex modelIndex = currentIndex();
    if (modelIndex != QModelIndex()) {
        OpenFileInfo *ofi = qvariant_cast<OpenFileInfo *>(modelIndex.data(Qt::UserRole));
        d->ofim->setCurrentFile(ofi);
    }
}

void DocumentListView::openFileWithService(int i)
{
    QModelIndex modelIndex = currentIndex();
    if (modelIndex != QModelIndex()) {
        OpenFileInfo *ofi = qvariant_cast<OpenFileInfo *>(modelIndex.data(Qt::UserRole));
        if (!ofi->isModified() || (QMessageBox::question(this, i18n("Save before switching?"), i18n("The current document has to be saved before switching the viewer/editor component.")/*, KGuiItem(i18n("Save document"), QIcon::fromTheme("document-save")), KGuiItem(i18n("Do not switch"), QIcon::fromTheme("dialog-cancel"))*/) == QMessageBox::Yes && ofi->save()))
            d->ofim->setCurrentFile(ofi, d->openMenuServices[i]);
    }
}

void DocumentListView::closeFile()
{
    QModelIndex modelIndex = currentIndex();
    if (modelIndex != QModelIndex()) {
        OpenFileInfo *ofi = qvariant_cast<OpenFileInfo *>(modelIndex.data(Qt::UserRole));
        d->ofim->close(ofi);
    }
}

void DocumentListView::currentChanged(const QModelIndex &current, const QModelIndex &)
{
    bool hasCurrent = current != QModelIndex();
    OpenFileInfo *ofi = hasCurrent ? qvariant_cast<OpenFileInfo *>(current.data(Qt::UserRole)) : NULL;
    bool isOpen = hasCurrent ? ofi->flags().testFlag(OpenFileInfo::Open) : false;
    bool isFavorite = hasCurrent ? ofi->flags().testFlag(OpenFileInfo::Favorite) : false;
    bool hasName = hasCurrent ? ofi->flags().testFlag(OpenFileInfo::HasName) : false;

    if (d->actionOpenFile != NULL)
        d->actionOpenFile->setEnabled(hasCurrent && !isOpen);
    if (d->actionCloseFile != NULL)
        d->actionCloseFile->setEnabled(hasCurrent && isOpen);
    if (d->actionAddToFav != NULL)
        d->actionAddToFav->setEnabled(hasCurrent && !isFavorite && hasName);
    if (d->actionRemFromFav != NULL)
        d->actionRemFromFav->setEnabled(hasCurrent && isFavorite);

    foreach(QAction *action, d->openMenuActions) {
         d->actionOpenMenu->removeAction(action);
    }
    d->openMenuServices.clear();
    if (ofi != NULL) {
        d->openMenuServices = ofi->listOfServices();
        int i = 0;
        foreach(KService::Ptr servicePtr, d->openMenuServices) {
            QAction *menuItem = new QAction(QIcon::fromTheme(servicePtr->icon()), servicePtr->name(), this);
            d->actionOpenMenu->addAction(menuItem);
            d->openMenuActions << menuItem;

            d->openMenuSignalMapper.setMapping(menuItem, i);
            connect(menuItem, SIGNAL(triggered()), &d->openMenuSignalMapper, SLOT(map()));
            ++i;
        }
    }
    d->actionOpenMenu->setEnabled(!d->openMenuActions.isEmpty());
}

class DocumentList::DocumentListPrivate
{
public:
    DocumentList *p;

    DocumentListView *listOpenFiles;
    DocumentListView *listRecentFiles;
    DocumentListView *listFavorites;
    DirOperatorWidget *dirOperator;

    DocumentListPrivate(OpenFileInfoManager *openFileInfoManager, DocumentList *p) {
        listOpenFiles = new DocumentListView(openFileInfoManager, OpenFileInfo::Open, p);
        DocumentListModel *model = new DocumentListModel(OpenFileInfo::Open, openFileInfoManager, listOpenFiles);
        listOpenFiles->setModel(model);
        p->addTab(listOpenFiles, QIcon::fromTheme("document-open"), i18n("Open Files"));

        listRecentFiles = new DocumentListView(openFileInfoManager, OpenFileInfo::RecentlyUsed, p);
        model = new DocumentListModel(OpenFileInfo::RecentlyUsed, openFileInfoManager, listRecentFiles);
        listRecentFiles->setModel(model);
        p->addTab(listRecentFiles, QIcon::fromTheme("clock"), i18n("Recently Used"));

        listFavorites = new DocumentListView(openFileInfoManager, OpenFileInfo::Favorite, p);
        model = new DocumentListModel(OpenFileInfo::Favorite, openFileInfoManager, listFavorites);
        listFavorites->setModel(model);
        p->addTab(listFavorites, QIcon::fromTheme("favorites"), i18n("Favorites"));

        dirOperator = new DirOperatorWidget(p);
        p->addTab(dirOperator,  QIcon::fromTheme("system-file-manager"), i18n("Filesystem Browser"));
        connect(dirOperator->dirOperator, SIGNAL(fileSelected(KFileItem)), p, SLOT(fileSelected(KFileItem)));
    }
};

DocumentList::DocumentList(OpenFileInfoManager *openFileInfoManager, QWidget *parent)
        : QTabWidget(parent), d(new DocumentListPrivate(openFileInfoManager, this))
{
    setDocumentMode(true);
}

void DocumentList::fileSelected(const KFileItem &item)
{
    if (item.isFile() && item.isReadable())
        emit openFile(item.url());
}
