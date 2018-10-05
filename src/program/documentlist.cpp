/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "documentlist.h"

#include <QStringListModel>
#include <QTimer>
#include <QSignalMapper>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QApplication>
#include <QIcon>
#include <QAction>

#include <KIconLoader>
#include <KLocalizedString>
#include <KActionMenu>
#include <KDirOperator>
#include <KService>
#include <KMessageBox>

#include "kbibtex.h"

class DirOperatorWidget : public QWidget
{
    Q_OBJECT

public:
    KDirOperator *dirOperator;

    DirOperatorWidget(QWidget *parent)
            : QWidget(parent) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);
        layout->setColumnStretch(0, 0);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 1);

        QPushButton *buttonUp = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), QString(), this);
        buttonUp->setToolTip(i18n("One level up"));
        layout->addWidget(buttonUp, 0, 0, 1, 1);

        QPushButton *buttonHome = new QPushButton(QIcon::fromTheme(QStringLiteral("user-home")), QString(), this);
        buttonHome->setToolTip(i18n("Go to Home folder"));
        layout->addWidget(buttonHome, 0, 1, 1, 1);

        dirOperator = new KDirOperator(QUrl(QStringLiteral("file:") + QDir::homePath()), this);
        layout->addWidget(dirOperator, 1, 0, 1, 3);
        dirOperator->setView(KFile::Detail);

        connect(buttonUp, &QPushButton::clicked, dirOperator, &KDirOperator::cdUp);
        connect(buttonHome, &QPushButton::clicked, dirOperator, &KDirOperator::home);
    }
};

DocumentListDelegate::DocumentListDelegate(QObject *parent)
        : QStyledItemDelegate(parent), ofim(OpenFileInfoManager::instance())
{
    /// nothing
}

void DocumentListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int height = option.rect.height();

    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, nullptr);

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

public:
    DocumentListModelPrivate(OpenFileInfo::StatusFlag statusFlag, DocumentListModel *parent)
            : sf(statusFlag), ofim(OpenFileInfoManager::instance())
    {
        Q_UNUSED(parent)
    }
};

DocumentListModel::DocumentListModel(OpenFileInfo::StatusFlag statusFlag, QObject *parent)
        : QAbstractListModel(parent), d(new DocumentListModel::DocumentListModelPrivate(statusFlag, this))
{
    listsChanged(d->sf);

    connect(OpenFileInfoManager::instance(), &OpenFileInfoManager::flagsChanged, this, &DocumentListModel::listsChanged);
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
    const QString iconName = openFileInfo->mimeType().replace(QLatin1Char('/'), QLatin1Char('-'));

    switch (role) {
    case Qt::DisplayRole: return openFileInfo->shortCaption();
    case Qt::DecorationRole: {
        /// determine mime type-based icon and overlays (e.g. for modified files)
        QStringList overlays;
        if (openFileInfo->flags().testFlag(OpenFileInfo::Favorite))
            overlays << QStringLiteral("favorites");
        else
            overlays << QString();
        if (openFileInfo->flags().testFlag(OpenFileInfo::RecentlyUsed))
            overlays << QStringLiteral("document-open-recent");
        else
            overlays << QString();
        if (openFileInfo->flags().testFlag(OpenFileInfo::Open))
            overlays << QStringLiteral("folder-open");
        else
            overlays << QString();
        if (openFileInfo->isModified())
            overlays << QStringLiteral("document-save");
        else
            overlays << QString();
        return KDE::icon(iconName, overlays, nullptr);
    }
    case Qt::ToolTipRole: {
        QString htmlText(QString(QStringLiteral("<qt><img src=\"%1\"> <b>%2</b>")).arg(KIconLoader::global()->iconPath(iconName, KIconLoader::Small), openFileInfo->shortCaption()));
        const QUrl url = openFileInfo->url();
        if (url.isValid()) {
            QString path(QFileInfo(url.path()).path());
            if (!path.endsWith(QLatin1Char('/')))
                path.append(QLatin1Char('/'));
            htmlText.append(i18n("<br/><small>located in <b>%1</b></small>", path));
        }
        QStringList flagListItems;
        if (openFileInfo->flags().testFlag(OpenFileInfo::Favorite))
            flagListItems << i18n("Favorite");
        if (openFileInfo->flags().testFlag(OpenFileInfo::RecentlyUsed))
            flagListItems << i18n("Recently Used");
        if (openFileInfo->flags().testFlag(OpenFileInfo::Open))
            flagListItems << i18n("Open");
        if (openFileInfo->isModified())
            flagListItems << i18n("Modified");
        if (!flagListItems.empty()) {
            htmlText.append(QStringLiteral("<ul>"));
            for (const QString &flagListItem : const_cast<const QStringList &>(flagListItems)) {
                htmlText.append(QString(QStringLiteral("<li>%1</li>")).arg(flagListItem));
            }
            htmlText.append(QStringLiteral("</ul>"));
        }
        htmlText.append(QStringLiteral("</qt>"));
        return htmlText;
    }
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

    DocumentListViewPrivate(DocumentListView *parent)
            : p(parent), ofim(OpenFileInfoManager::instance()), actionAddToFav(nullptr), actionRemFromFav(nullptr), actionCloseFile(nullptr), actionOpenFile(nullptr), actionOpenMenu(nullptr) {
        connect(&openMenuSignalMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), p, &DocumentListView::openFileWithService);
    }
};

DocumentListView::DocumentListView(OpenFileInfo::StatusFlag statusFlag, QWidget *parent)
        : QListView(parent), d(new DocumentListViewPrivate(this))
{
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setItemDelegate(new DocumentListDelegate(this));

    if (statusFlag == OpenFileInfo::Open) {
        d->actionCloseFile = new QAction(QIcon::fromTheme(QStringLiteral("document-close")), i18n("Close File"), this);
        connect(d->actionCloseFile, &QAction::triggered, this, &DocumentListView::closeFile);
        addAction(d->actionCloseFile);
    } else {
        d->actionOpenFile = new QAction(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open File"), this);
        connect(d->actionOpenFile, &QAction::triggered, this, &DocumentListView::openFile);
        addAction(d->actionOpenFile);
    }

    d->actionOpenMenu = new KActionMenu(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open with"), this);
    addAction(d->actionOpenMenu);

    if (statusFlag == OpenFileInfo::Favorite) {
        d->actionRemFromFav = new QAction(QIcon::fromTheme(QStringLiteral("favorites")), i18n("Remove from Favorites"), this);
        connect(d->actionRemFromFav, &QAction::triggered, this, &DocumentListView::removeFromFavorites);
        addAction(d->actionRemFromFav);
    } else {
        d->actionAddToFav = new QAction(QIcon::fromTheme(QStringLiteral("favorites")), i18n("Add to Favorites"), this);
        connect(d->actionAddToFav, &QAction::triggered, this, &DocumentListView::addToFavorites);
        addAction(d->actionAddToFav);
    }

    connect(this, &DocumentListView::activated, this, &DocumentListView::openFile);

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
        if (!ofi->isModified() || (KMessageBox::questionYesNo(this, i18n("The current document has to be saved before switching the viewer/editor component."), i18n("Save before switching?"), KGuiItem(i18n("Save document"), QIcon::fromTheme(QStringLiteral("document-save"))), KGuiItem(i18n("Do not switch"), QIcon::fromTheme(QStringLiteral("dialog-cancel")))) == KMessageBox::Yes && ofi->save()))
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
    OpenFileInfo *ofi = hasCurrent ? qvariant_cast<OpenFileInfo *>(current.data(Qt::UserRole)) : nullptr;
    bool isOpen = hasCurrent ? ofi->flags().testFlag(OpenFileInfo::Open) : false;
    bool isFavorite = hasCurrent ? ofi->flags().testFlag(OpenFileInfo::Favorite) : false;
    bool hasName = hasCurrent ? ofi->flags().testFlag(OpenFileInfo::HasName) : false;

    if (d->actionOpenFile != nullptr)
        d->actionOpenFile->setEnabled(hasCurrent && !isOpen);
    if (d->actionCloseFile != nullptr)
        d->actionCloseFile->setEnabled(hasCurrent && isOpen);
    if (d->actionAddToFav != nullptr)
        d->actionAddToFav->setEnabled(hasCurrent && !isFavorite && hasName);
    if (d->actionRemFromFav != nullptr)
        d->actionRemFromFav->setEnabled(hasCurrent && isFavorite);

    for (QAction *action : const_cast<const QList<QAction *> &>(d->openMenuActions)) {
        d->actionOpenMenu->removeAction(action);
    }
    d->openMenuServices.clear();
    if (ofi != nullptr) {
        d->openMenuServices = ofi->listOfServices();
        int i = 0;
        for (KService::Ptr servicePtr : const_cast<const KService::List &>(d->openMenuServices)) {
            QAction *menuItem = new QAction(QIcon::fromTheme(servicePtr->icon()), servicePtr->name(), this);
            d->actionOpenMenu->addAction(menuItem);
            d->openMenuActions << menuItem;

            d->openMenuSignalMapper.setMapping(menuItem, i);
            connect(menuItem, &QAction::triggered, &d->openMenuSignalMapper, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
            ++i;
        }
    }
    d->actionOpenMenu->setEnabled(!d->openMenuActions.isEmpty());
}

class DocumentList::DocumentListPrivate
{
public:
    DocumentListView *listOpenFiles;
    DocumentListView *listRecentFiles;
    DocumentListView *listFavorites;
    DirOperatorWidget *dirOperator;

    DocumentListPrivate(DocumentList *p) {
        listOpenFiles = new DocumentListView(OpenFileInfo::Open, p);
        DocumentListModel *model = new DocumentListModel(OpenFileInfo::Open, listOpenFiles);
        listOpenFiles->setModel(model);
        p->addTab(listOpenFiles, QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open Files"));

        listRecentFiles = new DocumentListView(OpenFileInfo::RecentlyUsed, p);
        model = new DocumentListModel(OpenFileInfo::RecentlyUsed, listRecentFiles);
        listRecentFiles->setModel(model);
        p->addTab(listRecentFiles, QIcon::fromTheme(QStringLiteral("document-open-recent")), i18n("Recently Used"));

        listFavorites = new DocumentListView(OpenFileInfo::Favorite, p);
        model = new DocumentListModel(OpenFileInfo::Favorite, listFavorites);
        listFavorites->setModel(model);
        p->addTab(listFavorites, QIcon::fromTheme(QStringLiteral("favorites")), i18n("Favorites"));

        dirOperator = new DirOperatorWidget(p);
        p->addTab(dirOperator,  QIcon::fromTheme(QStringLiteral("system-file-manager")), i18n("Filesystem Browser"));
        connect(dirOperator->dirOperator, &KDirOperator::fileSelected, p, &DocumentList::fileSelected);
    }
};

DocumentList::DocumentList(QWidget *parent)
        : QTabWidget(parent), d(new DocumentListPrivate(this))
{
    setDocumentMode(true);
}

void DocumentList::fileSelected(const KFileItem &item)
{
    if (item.isFile() && item.isReadable())
        emit openFile(item.url());
}

#include "documentlist.moc"
