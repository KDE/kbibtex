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

#include "mainwindow.h"

#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>
#include <QPointer>
#include <QMenu>
#include <QTimer>
#include <QApplication>
#include <QFileDialog>
#include <QAction>

#include <KActionMenu>
#include <KActionCollection>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KMessageBox>

#include "kbibtex.h"
#include "preferences/kbibtexpreferencesdialog.h"
#include "valuelist.h"
#ifdef HAVE_ZOTERO
#include "zoterobrowser.h"
#endif // HAVE_ZOTERO
#include "statistics.h"
#include "documentlist.h"
#include "mdiwidget.h"
#include "referencepreview.h"
#include "documentpreview.h"
#include "searchform.h"
#include "searchresults.h"
#include "elementform.h"
#include "fileview.h"
#include "filesettings.h"
#include "xsltransform.h"
#include "bibliographyservice.h"
#include "bibutils.h"

class KBibTeXMainWindow::KBibTeXMainWindowPrivate
{
private:
    KBibTeXMainWindow *p;

public:
    QAction *actionClose;
    QDockWidget *dockDocumentList;
    QDockWidget *dockReferencePreview;
    QDockWidget *dockDocumentPreview;
    QDockWidget *dockValueList;
#ifdef HAVE_ZOTERO
    QDockWidget *dockZotero;
#endif // HAVE_ZOTERO
    QDockWidget *dockStatistics;
    QDockWidget *dockSearchForm;
    QDockWidget *dockSearchResults;
    QDockWidget *dockElementForm;
    QDockWidget *dockFileSettings;
    DocumentList *listDocumentList;
    MDIWidget *mdiWidget;
    ReferencePreview *referencePreview;
    DocumentPreview *documentPreview;
    FileSettings *fileSettings;
    ValueList *valueList;
#ifdef HAVE_ZOTERO
    ZoteroBrowser *zotero;
#endif // HAVE_ZOTERO
    Statistics *statistics;
    SearchForm *searchForm;
    SearchResults *searchResults;
    ElementForm *elementForm;
    QMenu *actionMenuRecentFilesMenu;

    KBibTeXMainWindowPrivate(KBibTeXMainWindow *parent)
            : p(parent)
    {
        mdiWidget = new MDIWidget(p);

        KActionMenu *showPanelsAction = new KActionMenu(i18n("Show Panels"), p);
        p->actionCollection()->addAction(QStringLiteral("settings_shown_panels"), showPanelsAction);
        QMenu *showPanelsMenu = new QMenu(showPanelsAction->text(), p->widget());
        showPanelsAction->setMenu(showPanelsMenu);

        KActionMenu *actionMenuRecentFiles = new KActionMenu(QIcon::fromTheme(QStringLiteral("document-open-recent")), i18n("Recently used files"), p);
        p->actionCollection()->addAction(QStringLiteral("file_open_recent"), actionMenuRecentFiles);
        actionMenuRecentFilesMenu = new QMenu(actionMenuRecentFiles->text(), p->widget());
        actionMenuRecentFiles->setMenu(actionMenuRecentFilesMenu);

        /**
         * Docklets (a.k.a. panels) will be added by default to the following
         * positions unless otherwise configured by the user.
         * - "List of Values" on the left
         * - "Statistics" on the left
         * - "List of Documents" on the left in the same tab
         * - "Online Search" on the left in a new tab
         * - "Reference Preview" on the left in the same tab
         * - "Search Results" on the bottom
         * - "Document Preview" is hidden
         * - "Element Editor" is hidden
         */
        dockDocumentList = new QDockWidget(i18n("List of Documents"), p);
        dockDocumentList->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockDocumentList);
        listDocumentList = new DocumentList(dockDocumentList);
        dockDocumentList->setWidget(listDocumentList);
        dockDocumentList->setObjectName(QStringLiteral("dockDocumentList"));
        dockDocumentList->setFeatures(QDockWidget::DockWidgetClosable    | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        connect(listDocumentList, &DocumentList::openFile, p, &KBibTeXMainWindow::openDocument);
        showPanelsMenu->addAction(dockDocumentList->toggleViewAction());

        dockValueList = new QDockWidget(i18n("List of Values"), p);
        dockValueList->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockValueList);
        valueList = new ValueList(dockValueList);
        dockValueList->setWidget(valueList);
        dockValueList->setObjectName(QStringLiteral("dockValueList"));
        dockValueList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockValueList->toggleViewAction());

        dockStatistics = new QDockWidget(i18n("Statistics"), p);
        dockStatistics->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockStatistics);
        statistics = new Statistics(dockStatistics);
        dockStatistics->setWidget(statistics);
        dockStatistics->setObjectName(QStringLiteral("dockStatistics"));
        dockStatistics->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockStatistics->toggleViewAction());

        dockSearchResults = new QDockWidget(i18n("Search Results"), p);
        dockSearchResults->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::BottomDockWidgetArea, dockSearchResults);
        dockSearchResults->hide();
        searchResults = new SearchResults(mdiWidget, dockSearchResults);
        dockSearchResults->setWidget(searchResults);
        dockSearchResults->setObjectName(QStringLiteral("dockResultsFrom"));
        dockSearchResults->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockSearchResults->toggleViewAction());
        connect(mdiWidget, &MDIWidget::documentSwitched, searchResults, &SearchResults::documentSwitched);

        dockSearchForm = new QDockWidget(i18n("Online Search"), p);
        dockSearchForm->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockSearchForm);
        searchForm = new SearchForm(searchResults, dockSearchForm);
        connect(searchForm, &SearchForm::doneSearching, p, &KBibTeXMainWindow::showSearchResults);
        dockSearchForm->setWidget(searchForm);
        dockSearchForm->setObjectName(QStringLiteral("dockSearchFrom"));
        dockSearchForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockSearchForm->toggleViewAction());

#ifdef HAVE_ZOTERO
        dockZotero = new QDockWidget(i18n("Zotero"), p);
        dockZotero->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockZotero);
        zotero = new ZoteroBrowser(searchResults, dockZotero);
        connect(dockZotero, &QDockWidget::visibilityChanged, zotero, &ZoteroBrowser::visibiltyChanged);
        connect(zotero, &ZoteroBrowser::itemToShow, p, &KBibTeXMainWindow::showSearchResults);
        dockZotero->setWidget(zotero);
        dockZotero->setObjectName(QStringLiteral("dockZotero"));
        dockZotero->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockZotero->toggleViewAction());
#endif // HAVE_ZOTERO

        dockReferencePreview = new QDockWidget(i18n("Reference Preview"), p);
        dockReferencePreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockReferencePreview);
        referencePreview = new ReferencePreview(dockReferencePreview);
        dockReferencePreview->setWidget(referencePreview);
        dockReferencePreview->setObjectName(QStringLiteral("dockReferencePreview"));
        dockReferencePreview->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockReferencePreview->toggleViewAction());

        dockDocumentPreview = new QDockWidget(i18n("Document Preview"), p);
        dockDocumentPreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::RightDockWidgetArea, dockDocumentPreview);
        dockDocumentPreview->hide();
        documentPreview = new DocumentPreview(dockDocumentPreview);
        dockDocumentPreview->setWidget(documentPreview);
        dockDocumentPreview->setObjectName(QStringLiteral("dockDocumentPreview"));
        dockDocumentPreview->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockDocumentPreview->toggleViewAction());
        p->actionCollection()->setDefaultShortcut(dockDocumentPreview->toggleViewAction(), Qt::CTRL + Qt::SHIFT + Qt::Key_D);

        dockElementForm = new QDockWidget(i18n("Element Editor"), p);
        dockElementForm->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::BottomDockWidgetArea, dockElementForm);
        dockElementForm->hide();
        elementForm = new ElementForm(mdiWidget, dockElementForm);
        dockElementForm->setWidget(elementForm);
        dockElementForm->setObjectName(QStringLiteral("dockElementFrom"));
        dockElementForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockElementForm->toggleViewAction());

        dockFileSettings = new QDockWidget(i18n("File Settings"), p);
        dockFileSettings->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p->addDockWidget(Qt::LeftDockWidgetArea, dockFileSettings);
        fileSettings = new FileSettings(dockFileSettings);
        dockFileSettings->setWidget(fileSettings);
        dockFileSettings->setObjectName(QStringLiteral("dockFileSettings"));
        dockFileSettings->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        showPanelsMenu->addAction(dockFileSettings->toggleViewAction());

        p->tabifyDockWidget(dockFileSettings, dockSearchForm);
#ifdef HAVE_ZOTERO
        p->tabifyDockWidget(dockZotero, dockSearchForm);
#endif // HAVE_ZOTERO
        p->tabifyDockWidget(dockValueList, dockStatistics);
        p->tabifyDockWidget(dockStatistics, dockFileSettings);
        p->tabifyDockWidget(dockSearchForm, dockReferencePreview);
        p->tabifyDockWidget(dockFileSettings, dockDocumentList);

        QAction *action = p->actionCollection()->addAction(KStandardAction::New);
        connect(action, &QAction::triggered, p, &KBibTeXMainWindow::newDocument);
        action = p->actionCollection()->addAction(KStandardAction::Open);
        connect(action, &QAction::triggered, p, &KBibTeXMainWindow::openDocumentDialog);
        actionClose = p->actionCollection()->addAction(KStandardAction::Close);
        connect(actionClose, &QAction::triggered, p, &KBibTeXMainWindow::closeDocument);
        actionClose->setEnabled(false);
        action = p->actionCollection()->addAction(KStandardAction::Quit);
        connect(action, &QAction::triggered, p, &KBibTeXMainWindow::queryCloseAll);
        action = p->actionCollection()->addAction(KStandardAction::Preferences);
        connect(action, &QAction::triggered, p, &KBibTeXMainWindow::showPreferences);
    }

    ~KBibTeXMainWindowPrivate() {
        elementForm->deleteLater();
        delete mdiWidget;
        // TODO other deletes
    }
};

KBibTeXMainWindow::KBibTeXMainWindow(QWidget *parent)
        : KParts::MainWindow(parent, (Qt::WindowFlags)KDE_DEFAULT_WINDOWFLAGS), d(new KBibTeXMainWindowPrivate(this))
{
    setObjectName(QStringLiteral("KBibTeXShell"));

    /*
        const char mainWindowStateKey[] = "State";
        KConfigGroup group( KSharedConfig::openConfig(), "MainWindow" );
        if( !group.hasKey(mainWindowStateKey) )
            group.writeEntry( mainWindowStateKey, mainWindowState );
    */

    setXMLFile(QStringLiteral("kbibtexui.rc"));

    setCentralWidget(d->mdiWidget);

    connect(d->mdiWidget, &MDIWidget::documentSwitched, this, &KBibTeXMainWindow::documentSwitched);
    connect(d->mdiWidget, &MDIWidget::activePartChanged, this, &KBibTeXMainWindow::createGUI); ///< actually: KParts::MainWindow::createGUI
    connect(d->mdiWidget, &MDIWidget::documentNew, this, &KBibTeXMainWindow::newDocument);
    connect(d->mdiWidget, &MDIWidget::documentOpen, this, &KBibTeXMainWindow::openDocumentDialog);
    connect(d->mdiWidget, &MDIWidget::documentOpenURL, this, &KBibTeXMainWindow::openDocument);
    connect(OpenFileInfoManager::instance(), &OpenFileInfoManager::currentChanged, d->mdiWidget, &MDIWidget::setFile);
    connect(OpenFileInfoManager::instance(), &OpenFileInfoManager::flagsChanged, this, &KBibTeXMainWindow::documentListsChanged);
    connect(d->mdiWidget, &MDIWidget::setCaption, this, static_cast<void(KMainWindow::*)(const QString &)>(&KMainWindow::setCaption)); ///< actually: KMainWindow::setCaption

    documentListsChanged(OpenFileInfo::RecentlyUsed); /// force initialization of menu of recently used files

    setupControllers();
    setupGUI(KXmlGuiWindow::Create | KXmlGuiWindow::Save | KXmlGuiWindow::Keys | KXmlGuiWindow::ToolBar);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    setAcceptDrops(true);

    QTimer::singleShot(500, this, &KBibTeXMainWindow::delayed);
}

KBibTeXMainWindow::~KBibTeXMainWindow()
{
    delete d;
}

void KBibTeXMainWindow::setupControllers()
{
    // TODO
}

void KBibTeXMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void KBibTeXMainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urlList = event->mimeData()->urls();

    if (urlList.isEmpty()) {
        const QUrl url(event->mimeData()->text());
        if (url.isValid()) urlList << url;
    }

    if (!urlList.isEmpty())
        for (const QUrl &url : const_cast<const QList<QUrl> &>(urlList))
            openDocument(url);
}

void KBibTeXMainWindow::newDocument()
{
    const QString mimeType = FileInfo::mimetypeBibTeX;
    OpenFileInfo *openFileInfo = OpenFileInfoManager::instance()->createNew(mimeType);
    if (openFileInfo) {
        OpenFileInfoManager::instance()->setCurrentFile(openFileInfo);
        openFileInfo->addFlags(OpenFileInfo::Open);
    } else
        KMessageBox::error(this, i18n("Creating a new document of mime type '%1' failed as no editor component could be instantiated.", mimeType), i18n("Creating document failed"));
}

void KBibTeXMainWindow::openDocumentDialog()
{
    OpenFileInfo *currFile = OpenFileInfoManager::instance()->currentFile();
    QUrl currFileUrl = currFile == nullptr ? QUrl() : currFile->url();
    QString startDir = currFileUrl.isValid() ? QUrl(currFileUrl.url()).path() : QString();
    OpenFileInfo *ofi = OpenFileInfoManager::instance()->currentFile();
    if (ofi != nullptr) {
        QUrl url = ofi->url();
        if (url.isValid()) startDir = url.path();
    }

    /// Assemble list of supported mimetypes
    QStringList supportedMimeTypes {QStringLiteral("text/x-bibtex"), QStringLiteral("application/x-research-info-systems"), QStringLiteral("application/xml")};
    if (BibUtils::available()) {
        supportedMimeTypes.append(QStringLiteral("application/x-isi-export-format"));
        supportedMimeTypes.append(QStringLiteral("application/x-endnote-refer"));
    }
    supportedMimeTypes.append(QStringLiteral("application/pdf"));
    supportedMimeTypes.append(QStringLiteral("all/all"));
    QPointer<QFileDialog> dlg = new QFileDialog(this, i18n("Open file") /* TODO better text */, startDir);
    dlg->setMimeTypeFilters(supportedMimeTypes);
    dlg->setFileMode(QFileDialog::ExistingFile);

    const bool dialogAccepted = dlg->exec() != 0;
    const QUrl url = (dialogAccepted && !dlg->selectedUrls().isEmpty()) ? dlg->selectedUrls().first() : QUrl();

    delete dlg;

    if (!url.isEmpty())
        openDocument(url);
}

void KBibTeXMainWindow::openDocument(const QUrl &url)
{
    OpenFileInfo *openFileInfo = OpenFileInfoManager::instance()->open(url);
    OpenFileInfoManager::instance()->setCurrentFile(openFileInfo);
}

void KBibTeXMainWindow::closeDocument()
{
    OpenFileInfoManager::instance()->close(OpenFileInfoManager::instance()->currentFile());
}

void KBibTeXMainWindow::closeEvent(QCloseEvent *event)
{
    KMainWindow::closeEvent(event);

    if (OpenFileInfoManager::instance()->queryCloseAll())
        event->accept();
    else
        event->ignore();
}

void KBibTeXMainWindow::showPreferences()
{
    QPointer<KBibTeXPreferencesDialog> dlg = new KBibTeXPreferencesDialog(this);
    dlg->exec();
    delete dlg;
}

void KBibTeXMainWindow::documentSwitched(FileView *oldFileView, FileView *newFileView)
{
    OpenFileInfo *openFileInfo = d->mdiWidget->currentFile();
    bool validFile = openFileInfo != nullptr;
    d->actionClose->setEnabled(validFile);

    setCaption(validFile ? i18n("%1 - KBibTeX", openFileInfo->shortCaption()) : i18n("KBibTeX"));

    d->fileSettings->setEnabled(newFileView != nullptr);
    d->referencePreview->setEnabled(newFileView != nullptr);
    d->elementForm->setEnabled(newFileView != nullptr);
    d->documentPreview->setEnabled(newFileView != nullptr);
    if (oldFileView != nullptr) {
        disconnect(newFileView, &FileView::currentElementChanged, d->referencePreview, &ReferencePreview::setElement);
        disconnect(newFileView, &FileView::currentElementChanged, d->elementForm, &ElementForm::setElement);
        disconnect(newFileView, &FileView::currentElementChanged, d->documentPreview, &DocumentPreview::setElement);
        disconnect(newFileView, &FileView::currentElementChanged, d->searchForm, &SearchForm::setElement);
        disconnect(newFileView, &FileView::modified, d->valueList, &ValueList::update);
        disconnect(newFileView, &FileView::modified, d->statistics, &Statistics::update);
        // FIXME disconnect(oldEditor, SIGNAL(modified()), d->elementForm, SLOT(refreshElement()));
        disconnect(d->elementForm, &ElementForm::elementModified, newFileView, &FileView::externalModification);
    }
    if (newFileView != nullptr) {
        connect(newFileView, &FileView::currentElementChanged, d->referencePreview, &ReferencePreview::setElement);
        connect(newFileView, &FileView::currentElementChanged, d->elementForm, &ElementForm::setElement);
        connect(newFileView, &FileView::currentElementChanged, d->documentPreview, &DocumentPreview::setElement);
        connect(newFileView, &FileView::currentElementChanged, d->searchForm, &SearchForm::setElement);
        connect(newFileView, &FileView::modified, d->valueList, &ValueList::update);
        connect(newFileView, &FileView::modified, d->statistics, &Statistics::update);
        // FIXME connect(newEditor, SIGNAL(modified()), d->elementForm, SLOT(refreshElement()));
        connect(d->elementForm, &ElementForm::elementModified, newFileView, &FileView::externalModification);
        connect(d->elementForm, &ElementForm::elementModified, newFileView, &FileView::externalModification);
    }

    d->documentPreview->setBibTeXUrl(validFile ? openFileInfo->url() : QUrl());
    d->referencePreview->setElement(QSharedPointer<Element>(), nullptr);
    d->elementForm->setElement(QSharedPointer<Element>(), nullptr);
    d->documentPreview->setElement(QSharedPointer<Element>(), nullptr);
    d->valueList->setFileView(newFileView);
    d->fileSettings->setFileView(newFileView);
    d->statistics->setFileView(newFileView);
    d->referencePreview->setFileView(newFileView);
}

void KBibTeXMainWindow::showSearchResults()
{
    d->dockSearchResults->show();
}

void KBibTeXMainWindow::documentListsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(OpenFileInfo::RecentlyUsed)) {
        const OpenFileInfoManager::OpenFileInfoList list = OpenFileInfoManager::instance()->filteredItems(OpenFileInfo::RecentlyUsed);
        d->actionMenuRecentFilesMenu->clear();
        for (OpenFileInfo *cur : list) {
            /// Fixing bug 19511: too long filenames make menu too large,
            /// therefore squeeze text if it is longer than squeezeLen.
            const int squeezeLen = 64;
            const QString squeezedShortCap = squeeze_text(cur->shortCaption(), squeezeLen);
            const QString squeezedFullCap = squeeze_text(cur->fullCaption(), squeezeLen);
            QAction *action = new QAction(QString(QStringLiteral("%1 [%2]")).arg(squeezedShortCap, squeezedFullCap), this);
            action->setData(cur->url());
            action->setIcon(QIcon::fromTheme(cur->mimeType().replace(QLatin1Char('/'), QLatin1Char('-'))));
            d->actionMenuRecentFilesMenu->addAction(action);
            connect(action, &QAction::triggered, this, &KBibTeXMainWindow::openRecentFile);
        }
    }
}

void KBibTeXMainWindow::openRecentFile()
{
    QAction *action = static_cast<QAction *>(sender());
    QUrl url = action->data().toUrl();
    openDocument(url);
}

void KBibTeXMainWindow::queryCloseAll()
{
    if (OpenFileInfoManager::instance()->queryCloseAll())
        qApp->quit();
}

void KBibTeXMainWindow::delayed() {
    /// Static variable, memorizes the dynamically created
    /// BibliographyService instance and allows to tell if
    /// this slot was called for the first or second time.
    static BibliographyService *bs = nullptr;

    if (bs == nullptr) {
        /// First call to this slot
        bs = new BibliographyService(this);
        if (!bs->isKBibTeXdefault() && KMessageBox::questionYesNo(this, i18n("KBibTeX is not the default editor for its bibliography formats like BibTeX or RIS."), i18n("Default Bibliography Editor"), KGuiItem(i18n("Set as Default Editor")), KGuiItem(i18n("Keep settings unchanged"))) == KMessageBox::Yes) {
            bs->setKBibTeXasDefault();
            /// QTimer calls this slot again, but as 'bs' will not be NULL,
            /// the 'if' construct's 'else' path will be followed.
            QTimer::singleShot(5000, this, &KBibTeXMainWindow::delayed);
        } else {
            /// KBibTeX is default application or user doesn't care,
            /// therefore clean up memory
            delete bs;
            bs = nullptr;
        }
    } else {
        /// Second call to this slot. This time, clean up memory.
        bs->deleteLater();
        bs = nullptr;
    }
}
