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

#include "mdiwidget.h"

#include <QVector>
#include <QPair>
#include <QLabel>
#include <QApplication>
#include <QSignalMapper>
#include <QLayout>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPushButton>
#include <QIcon>
#include <QDebug>

#include <KParts/Part>
#include <KParts/ReadOnlyPart>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KMessageBox>

#include "kbibtex.h"
#include "partwidget.h"
#include "logging_program.h"

class LRUItemModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    OpenFileInfoManager *ofim;

public:
    static const int URLRole = Qt::UserRole + 235;
    static const int SortRole = Qt::UserRole + 236;

    LRUItemModel(OpenFileInfoManager *openFileInfoManager, QObject *parent = nullptr)
            : QAbstractItemModel(parent), ofim(openFileInfoManager) {
        /// nothing
    }

    void reset() {
        beginResetModel();
        endResetModel();
    }

    int columnCount(const QModelIndex &) const override {
        return 3;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        OpenFileInfoManager::OpenFileInfoList ofiList = ofim->filteredItems(OpenFileInfo::RecentlyUsed);
        if (index.row() < ofiList.count()) {
            OpenFileInfo *ofiItem = ofiList[index.row()];
            if (index.column() == 0) {
                if (role == Qt::DisplayRole || role == SortRole) {
                    const QUrl url = ofiItem->url();
                    const QString fileName = url.fileName();
                    return fileName.isEmpty() ? squeeze_text(url.url(QUrl::PreferLocalFile), 32) : fileName;
                } else if (role == Qt::DecorationRole)
                    return QIcon::fromTheme(ofiItem->mimeType().replace(QLatin1Char('/'), QLatin1Char('-')));
                else if (role == Qt::ToolTipRole)
                    return squeeze_text(ofiItem->url().url(QUrl::PreferLocalFile), 64);
            } else if (index.column() == 1) {
                if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
                    return ofiItem->lastAccess().toString(Qt::TextDate);
                else if (role == SortRole)
                    return ofiItem->lastAccess().toTime_t();
            } else if (index.column() == 2) {
                if (role == Qt::DisplayRole || role == Qt::ToolTipRole || role == SortRole)
                    return ofiItem->url().url(QUrl::PreferLocalFile);
            }
            if (role == URLRole)
                return ofiItem->url();
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || section > 2) return QVariant();
        else if (section == 0) return i18n("Filename");
        else if (section == 1) return i18n("Date/time of last use");
        else return i18n("Full filename");
    }

    QModelIndex index(int row, int column, const QModelIndex &) const override {
        return createIndex(row, column, row);
    }

    QModelIndex parent(const QModelIndex &) const override {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent == QModelIndex())
            return ofim->filteredItems(OpenFileInfo::RecentlyUsed).count();
        else
            return 0;
    }
};

class MDIWidget::MDIWidgetPrivate
{
private:
    QTreeView *listLRU;
    LRUItemModel *modelLRU;
    QVector<QWidget *> welcomeWidgets;

    static const QString configGroupName;
    static const QString configHeaderState;

    void createWelcomeWidget() {
        welcomeWidget = new QWidget(p);
        QGridLayout *layout = new QGridLayout(welcomeWidget);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);
        layout->setRowStretch(2, 0);
        layout->setRowStretch(3, 0);
        layout->setRowStretch(4, 10);
        layout->setRowStretch(5, 1);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 10);
        layout->setColumnStretch(2, 1);
        layout->setColumnStretch(3, 0);
        layout->setColumnStretch(4, 1);
        layout->setColumnStretch(5, 10);
        layout->setColumnStretch(6, 1);

        QLabel *label = new QLabel(i18n("<qt>Welcome to <b>KBibTeX</b></qt>"), welcomeWidget);
        layout->addWidget(label, 1, 2, 1, 3, Qt::AlignHCenter | Qt::AlignTop);

        QPushButton *buttonNew = new QPushButton(QIcon::fromTheme(QStringLiteral("document-new")), i18n("New"), welcomeWidget);
        layout->addWidget(buttonNew, 2, 2, 1, 1, Qt::AlignLeft | Qt::AlignBottom);
        connect(buttonNew, &QPushButton::clicked, p, &MDIWidget::documentNew);

        QPushButton *buttonOpen = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open..."), welcomeWidget);
        layout->addWidget(buttonOpen, 2, 4, 1, 1, Qt::AlignRight | Qt::AlignBottom);
        connect(buttonOpen, &QPushButton::clicked, p, &MDIWidget::documentOpen);

        label = new QLabel(i18n("List of recently used files:"), welcomeWidget);
        layout->addWidget(label, 3, 1, 1, 5, Qt::AlignLeft | Qt::AlignBottom);
        listLRU = new QTreeView(p);
        listLRU->setRootIsDecorated(false);
        listLRU->setSortingEnabled(true);
        listLRU->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        layout->addWidget(listLRU, 4, 1, 1, 5);
        connect(listLRU, &QTreeView::activated, p, &MDIWidget::slotOpenLRU);
        label->setBuddy(listLRU);

        p->addWidget(welcomeWidget);
    }

    void saveColumnsState() {
        QByteArray headerState = listLRU->header()->saveState();
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configHeaderState, headerState);
        config->sync();
    }

    void restoreColumnsState() {
        KConfigGroup configGroup(config, configGroupName);
        QByteArray headerState = configGroup.readEntry(configHeaderState, QByteArray());
        if (!headerState.isEmpty())
            listLRU->header()->restoreState(headerState);
    }

public:
    MDIWidget *p;
    OpenFileInfoManager *ofim;
    OpenFileInfo *currentFile;
    QWidget *welcomeWidget;
    QSignalMapper signalMapperCompleted;

    KSharedConfigPtr config;

    MDIWidgetPrivate(MDIWidget *parent)
            : p(parent), ofim(OpenFileInfoManager::instance()), currentFile(nullptr), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))) {
        createWelcomeWidget();

        modelLRU = new LRUItemModel(ofim, listLRU);
        QSortFilterProxyModel *sfpm = new QSortFilterProxyModel(listLRU);
        sfpm->setSourceModel(modelLRU);
        sfpm->setSortRole(LRUItemModel::SortRole);
        listLRU->setModel(sfpm);

        connect(&signalMapperCompleted, static_cast<void(QSignalMapper::*)(QObject *)>(&QSignalMapper::mapped), p, &MDIWidget::slotCompleted);

        restoreColumnsState();
    }

    ~MDIWidgetPrivate() {
        saveColumnsState();
        delete welcomeWidget;
    }

    void addToMapper(OpenFileInfo *openFileInfo) {
        KParts::ReadOnlyPart *part = openFileInfo->part(p);
        signalMapperCompleted.setMapping(part, openFileInfo);
        connect(part, static_cast<void(KParts::ReadOnlyPart::*)()>(&KParts::ReadOnlyPart::completed), &signalMapperCompleted, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
    }

    void updateLRU() {
        modelLRU->reset();
    }
};

const QString MDIWidget::MDIWidgetPrivate::configGroupName = QStringLiteral("WelcomeWidget");
const QString MDIWidget::MDIWidgetPrivate::configHeaderState = QStringLiteral("LRUlistHeaderState");

MDIWidget::MDIWidget(QWidget *parent)
        : QStackedWidget(parent), d(new MDIWidgetPrivate(this))
{
    connect(d->ofim, &OpenFileInfoManager::flagsChanged, this, &MDIWidget::slotStatusFlagsChanged);
}

MDIWidget::~MDIWidget()
{
    delete d;
}

/// Clazy warns: "Missing reference on non-trivial type" for argument 'servicePtr',
/// but type 'KService::Ptr' is actually a pointer (QExplicitlySharedDataPointer).
void MDIWidget::setFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr)
{
    KParts::Part *part = openFileInfo == nullptr ? nullptr : openFileInfo->part(this, servicePtr);
    /// 'part' will only be NULL if no OpenFileInfo object was given,
    /// or if the OpenFileInfo could not locate a KPart
    QWidget *widget = d->welcomeWidget; ///< by default use Welcome widget
    if (part != nullptr) {
        /// KPart object was located, use its object (instead of Welcome widget)
        widget = part->widget();
    } else if (openFileInfo != nullptr) {
        /// A valid OpenFileInfo was given, but no KPart could be located

        d->ofim->close(openFileInfo); // FIXME does not close correctly if file is new
        const QString filename = openFileInfo->url().fileName();
        if (filename.isEmpty())
            KMessageBox::error(this, i18n("No part available for file of mime type '%1'.", openFileInfo->mimeType()), i18n("No Part Available"));
        else
            KMessageBox::error(this, i18n("No part available for file '%1'.", filename), i18n("No Part Available"));
        return;
    }

    FileView *oldEditor = nullptr;
    bool hasChanged = true;
    if (indexOf(widget) >= 0) {
        /// Chosen widget is already known (Welcome widget or a previously used KPart)

        /// In case previous (still current) widget was a KBibTeX Part, remember its editor
        PartWidget *currentPartWidget = qobject_cast<PartWidget *>(currentWidget());
        oldEditor = currentPartWidget == nullptr ? nullptr : currentPartWidget->fileView();
        /// Record if set widget is different from previous (still current) widget
        hasChanged = widget != currentWidget();
    } else if (openFileInfo != nullptr) {
        /// Widget was not known previously, but a valid (new?) OpenFileInfo was given
        addWidget(widget);
        d->addToMapper(openFileInfo);
    }
    setCurrentWidget(widget);
    d->currentFile = openFileInfo;

    if (hasChanged) {
        /// This signal gets forwarded to KParts::MainWindow::createGUI(KParts::Part*)
        emit activePartChanged(part);
        /// If new widget comes from a KBibTeX Part, retrieve its editor
        PartWidget *newPartWidget = qobject_cast<PartWidget *>(widget);
        FileView *newEditor = newPartWidget == nullptr ? nullptr : newPartWidget->fileView();
        emit documentSwitched(oldEditor, newEditor);
    }

    /// Notify main window about a change of current file,
    /// so that the title may contain the current file's URL.
    /// This signal will be connected to KMainWindow::setCaption.
    if (openFileInfo != nullptr) {
        QUrl url = openFileInfo->url();
        if (url.isValid())
            emit setCaption(QString(QStringLiteral("%1 [%2]")).arg(openFileInfo->shortCaption(), squeeze_text(openFileInfo->fullCaption(), 64)));
        else
            emit setCaption(openFileInfo->shortCaption());
    } else
        emit setCaption(QString());
}

FileView *MDIWidget::fileView()
{
    OpenFileInfo *ofi = d->ofim->currentFile();
    return qobject_cast<PartWidget *>(ofi->part(this)->widget())->fileView();
}

OpenFileInfo *MDIWidget::currentFile()
{
    return d->currentFile;
}

void MDIWidget::slotCompleted(QObject *obj)
{
    OpenFileInfo *ofi = static_cast<OpenFileInfo *>(obj);
    QUrl oldUrl = ofi->url();
    QUrl newUrl = ofi->part(this)->url();

    if (oldUrl != newUrl) {
        qCDebug(LOG_KBIBTEX_PROGRAM) << "Url changed from " << oldUrl.url(QUrl::PreferLocalFile) << " to " << newUrl.url(QUrl::PreferLocalFile) << endl;
        d->ofim->changeUrl(ofi, newUrl);

        /// completely opened or saved files should be marked as "recently used"
        ofi->addFlags(OpenFileInfo::RecentlyUsed);

        emit setCaption(QString(QStringLiteral("%1 [%2]")).arg(ofi->shortCaption(), squeeze_text(ofi->fullCaption(), 64)));
    }
}

void MDIWidget::slotStatusFlagsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(OpenFileInfo::RecentlyUsed))
        d->updateLRU();
}

void MDIWidget::slotOpenLRU(const QModelIndex &index)
{
    QUrl url = index.data(LRUItemModel::URLRole).toUrl();
    if (url.isValid())
        emit documentOpenURL(url);
}

#include "mdiwidget.moc"
