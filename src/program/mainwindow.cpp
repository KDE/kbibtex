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

#include "mainwindow.h"

#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>
#include <QtCore/QPointer>
#include <QMessageBox>
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

#include "kbibtexnamespace.h"
#include "preferences/kbibtexpreferencesdialog.h"
#include "valuelist.h"
#include "zoterobrowser.h"
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
    // UNUSED KBibTeXMainWindow *p;

public:
    QAction *actionClose;
    QDockWidget *dockDocumentList;
    QDockWidget *dockReferencePreview;
    QDockWidget *dockDocumentPreview;
    QDockWidget *dockValueList;
    QDockWidget *dockZotero;
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
    ZoteroBrowser *zotero;
    Statistics *statistics;
    SearchForm *searchForm;
    SearchResults *searchResults;
    ElementForm *elementForm;
    QMenu *actionMenuRecentFilesMenu;

    KBibTeXMainWindowPrivate(KBibTeXMainWindow */* UNUSED parent*/)
    // UNUSED : p(parent)
    {
        /// nothing
    }

    ~KBibTeXMainWindowPrivate() {
        elementForm->deleteLater();
        delete mdiWidget;
        // TODO other deletes
    }
};

KBibTeXMainWindow::KBibTeXMainWindow()
        : KParts::MainWindow(), d(new KBibTeXMainWindowPrivate(this))
{
    setObjectName(QLatin1String("KBibTeXShell"));

    /*
        const char mainWindowStateKey[] = "State";
        KConfigGroup group( KSharedConfig::openConfig(), "MainWindow" );
        if( !group.hasKey(mainWindowStateKey) )
            group.writeEntry( mainWindowStateKey, mainWindowState );
    */

    setXMLFile("kbibtexui.rc");

    d->mdiWidget = new MDIWidget(this);
    setCentralWidget(d->mdiWidget);

    KActionMenu *showPanelsAction = new KActionMenu(i18n("Show Panels"), this);
    actionCollection()->addAction("settings_shown_panels", showPanelsAction);
    QMenu *showPanelsMenu = new QMenu(showPanelsAction->text(), widget());
    showPanelsAction->setMenu(showPanelsMenu);

    KActionMenu *actionMenuRecentFiles = new KActionMenu(QIcon::fromTheme("document-open-recent"), i18n("Recently used files"), this);
    actionCollection()->addAction("file_open_recent", actionMenuRecentFiles);
    d->actionMenuRecentFilesMenu = new QMenu(actionMenuRecentFiles->text(), widget());
    actionMenuRecentFiles->setMenu(d->actionMenuRecentFilesMenu);

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
    d->dockDocumentList = new QDockWidget(i18n("List of Documents"), this);
    d->dockDocumentList->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockDocumentList);
    d->listDocumentList = new DocumentList(d->dockDocumentList);
    d->dockDocumentList->setWidget(d->listDocumentList);
    d->dockDocumentList->setObjectName("dockDocumentList");
    d->dockDocumentList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    connect(d->listDocumentList, SIGNAL(openFile(QUrl)), this, SLOT(openDocument(QUrl)));
    showPanelsMenu->addAction(d->dockDocumentList->toggleViewAction());

    d->dockValueList = new QDockWidget(i18n("List of Values"), this);
    d->dockValueList->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockValueList);
    d->valueList = new ValueList(d->dockValueList);
    d->dockValueList->setWidget(d->valueList);
    d->dockValueList->setObjectName("dockValueList");
    d->dockValueList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockValueList->toggleViewAction());

    d->dockStatistics = new QDockWidget(i18n("Statistics"), this);
    d->dockStatistics->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockStatistics);
    d->statistics = new Statistics(d->dockStatistics);
    d->dockStatistics->setWidget(d->statistics);
    d->dockStatistics->setObjectName("dockStatistics");
    d->dockStatistics->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockStatistics->toggleViewAction());

    d->dockSearchResults = new QDockWidget(i18n("Search Results"), this);
    d->dockSearchResults->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, d->dockSearchResults);
    d->dockSearchResults->hide();
    d->searchResults = new SearchResults(d->mdiWidget, d->dockSearchResults);
    d->dockSearchResults->setWidget(d->searchResults);
    d->dockSearchResults->setObjectName("dockResultsFrom");
    d->dockSearchResults->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockSearchResults->toggleViewAction());
    connect(d->mdiWidget, SIGNAL(documentSwitch(FileView*,FileView*)), d->searchResults, SLOT(documentSwitched(FileView*,FileView*)));

    d->dockSearchForm = new QDockWidget(i18n("Online Search"), this);
    d->dockSearchForm->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockSearchForm);
    d->searchForm = new SearchForm(d->searchResults, d->dockSearchForm);
    connect(d->searchForm, SIGNAL(doneSearching()), this, SLOT(showSearchResults()));
    d->dockSearchForm->setWidget(d->searchForm);
    d->dockSearchForm->setObjectName("dockSearchFrom");
    d->dockSearchForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockSearchForm->toggleViewAction());

    d->dockZotero = new QDockWidget(i18n("Zotero"), this);
    d->dockZotero->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockZotero);
    d->zotero = new ZoteroBrowser(d->searchResults, d->dockZotero);
    d->dockZotero->setWidget(d->zotero);
    d->dockZotero->setObjectName("dockZotero");
    d->dockZotero->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockZotero->toggleViewAction());

    d->dockReferencePreview = new QDockWidget(i18n("Reference Preview"), this);
    d->dockReferencePreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockReferencePreview);
    d->referencePreview = new ReferencePreview(d->dockReferencePreview);
    d->dockReferencePreview->setWidget(d->referencePreview);
    d->dockReferencePreview->setObjectName("dockReferencePreview");
    d->dockReferencePreview->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockReferencePreview->toggleViewAction());

    d->dockDocumentPreview = new QDockWidget(i18n("Document Preview"), this);
    d->dockDocumentPreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, d->dockDocumentPreview);
    d->dockDocumentPreview->hide();
    d->documentPreview = new DocumentPreview(d->dockDocumentPreview);
    d->dockDocumentPreview->setWidget(d->documentPreview);
    d->dockDocumentPreview->setObjectName("dockDocumentPreview");
    d->dockDocumentPreview->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockDocumentPreview->toggleViewAction());
    actionCollection()->setDefaultShortcut(d->dockDocumentPreview->toggleViewAction(), Qt::CTRL + Qt::SHIFT + Qt::Key_D);

    d->dockElementForm = new QDockWidget(i18n("Element Editor"), this);
    d->dockElementForm->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, d->dockElementForm);
    d->dockElementForm->hide();
    d->elementForm = new ElementForm(d->mdiWidget, d->dockElementForm);
    d->dockElementForm->setWidget(d->elementForm);
    d->dockElementForm->setObjectName("dockElementFrom");
    d->dockElementForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockElementForm->toggleViewAction());

    d->dockFileSettings = new QDockWidget(i18n("File Settings"), this);
    d->dockFileSettings->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockFileSettings);
    d->fileSettings = new FileSettings(d->dockFileSettings);
    d->dockFileSettings->setWidget(d->fileSettings);
    d->dockFileSettings->setObjectName("dockFileSettings");
    d->dockFileSettings->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockFileSettings->toggleViewAction());

    tabifyDockWidget(d->dockFileSettings, d->dockSearchForm);
    tabifyDockWidget(d->dockZotero, d->dockSearchForm);
    tabifyDockWidget(d->dockValueList, d->dockStatistics);
    tabifyDockWidget(d->dockStatistics, d->dockFileSettings);
    tabifyDockWidget(d->dockSearchForm, d->dockReferencePreview);
    tabifyDockWidget(d->dockFileSettings, d->dockDocumentList);

    actionCollection()->addAction(KStandardAction::New, this, SLOT(newDocument()));
    actionCollection()->addAction(KStandardAction::Open, this, SLOT(openDocumentDialog()));
    d->actionClose = actionCollection()->addAction(KStandardAction::Close, this, SLOT(closeDocument()));
    d->actionClose->setEnabled(false);
    actionCollection()->addAction(KStandardAction::Quit, this, SLOT(queryCloseAll()));
    actionCollection()->addAction(KStandardAction::Preferences, this, SLOT(showPreferences()));

    connect(d->mdiWidget, SIGNAL(documentSwitch(FileView*,FileView*)), this, SLOT(documentSwitched(FileView*,FileView*)));
    connect(d->mdiWidget, SIGNAL(activePartChanged(KParts::Part*)), this, SLOT(createGUI(KParts::Part*)));
    connect(d->mdiWidget, SIGNAL(documentNew()), this, SLOT(newDocument()));
    connect(d->mdiWidget, SIGNAL(documentOpen()), this, SLOT(openDocumentDialog()));
    connect(d->mdiWidget, SIGNAL(documentOpenURL(QUrl)), this, SLOT(openDocument(QUrl)));
    connect(OpenFileInfoManager::instance(), SIGNAL(currentChanged(OpenFileInfo*,KService::Ptr)), d->mdiWidget, SLOT(setFile(OpenFileInfo*,KService::Ptr)));
    connect(OpenFileInfoManager::instance(), SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SLOT(documentListsChanged(OpenFileInfo::StatusFlags)));
    connect(d->mdiWidget, SIGNAL(setCaption(QString)), this, SLOT(setCaption(QString)));

    documentListsChanged(OpenFileInfo::RecentlyUsed); /// force initialization of menu of recently used files

    setupControllers();
    setupGUI(KXmlGuiWindow::Create | KXmlGuiWindow::Save | KXmlGuiWindow::Keys | KXmlGuiWindow::ToolBar);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    setAcceptDrops(true);

    QTimer::singleShot(500, this, SLOT(delayed()));
}

KBibTeXMainWindow::~KBibTeXMainWindow()
{
    delete d;
    XSLTransform::cleanupGlobals();
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
        QUrl url(event->mimeData()->text());
        if (url.isValid()) urlList << url;
    }

    if (!urlList.isEmpty())
        for (QList<QUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it)
            openDocument(*it);
}

void KBibTeXMainWindow::newDocument()
{
    const QString mimeType = FileInfo::mimetypeBibTeX;
    OpenFileInfo *openFileInfo = OpenFileInfoManager::instance()->createNew(mimeType);
    if (openFileInfo) {
        OpenFileInfoManager::instance()->setCurrentFile(openFileInfo);
        openFileInfo->addFlags(OpenFileInfo::Open);
    } else
        QMessageBox::warning(this, i18n("No Editor Component"), i18n("Creating a new document of mime type '%1' failed as no editor component could be instantiated.", mimeType), i18n("Creating document failed"));
}

void KBibTeXMainWindow::openDocumentDialog()
{
    OpenFileInfo *currFile = OpenFileInfoManager::instance()->currentFile();
    QUrl currFileUrl = currFile == NULL ? QUrl() : currFile->url();
    QString startDir = currFileUrl.isValid() ? QUrl(currFileUrl.url()).path() : QString();
    OpenFileInfo *ofi = OpenFileInfoManager::instance()->currentFile();
    if (ofi != NULL) {
        QUrl url = ofi->url();
        if (url.isValid()) startDir = url.path();
    }

    /// Assemble list of supported mimetypes
    QStringList supportedMimeTypes = QStringList() << QLatin1String("text/x-bibtex") << QLatin1String("application/x-research-info-systems") << QLatin1String("application/xml");
    if (BibUtils::available()) {
        supportedMimeTypes.append(QLatin1String("application/x-isi-export-format"));
        supportedMimeTypes.append(QLatin1String("application/x-endnote-refer"));
    }
    supportedMimeTypes.append(QLatin1String("all/all"));
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
    bool validFile = openFileInfo != NULL;
    d->actionClose->setEnabled(validFile);

    setCaption(validFile ? i18n("%1 - KBibTeX", openFileInfo->shortCaption()) : i18n("KBibTeX"));

    d->fileSettings->setEnabled(newFileView != NULL);
    d->referencePreview->setEnabled(newFileView != NULL);
    d->elementForm->setEnabled(newFileView != NULL);
    d->documentPreview->setEnabled(newFileView != NULL);
    if (oldFileView != NULL) {
        disconnect(newFileView, &FileView::currentElementChanged, d->referencePreview, &ReferencePreview::setElement);
        disconnect(newFileView, &FileView::currentElementChanged, d->elementForm, &ElementForm::setElement);
        disconnect(newFileView, &FileView::currentElementChanged, d->documentPreview, &DocumentPreview::setElement);
        disconnect(newFileView, &FileView::currentElementChanged, d->searchForm, &SearchForm::setElement);
        disconnect(newFileView, &FileView::modified, d->valueList, &ValueList::update);
        disconnect(newFileView, &FileView::modified, d->statistics, &Statistics::update);
        // FIXME disconnect(oldEditor, SIGNAL(modified()), d->elementForm, SLOT(refreshElement()));
        disconnect(d->elementForm, &ElementForm::elementModified, newFileView, &FileView::externalModification);
    }
    if (newFileView != NULL) {
        connect(newFileView, &FileView::currentElementChanged, d->referencePreview, &ReferencePreview::setElement);
        connect(newFileView, &FileView::currentElementChanged, d->elementForm, &ElementForm::setElement);
        connect(newFileView, &FileView::currentElementChanged, d->documentPreview, &DocumentPreview::setElement);
        connect(newFileView, &FileView::currentElementChanged, d->searchForm, &SearchForm::setElement);
        connect(newFileView, &FileView::modified, d->valueList, &ValueList::update);
        connect(newFileView, &FileView::modified, d->statistics, &Statistics::update);
        // FIXME connect(newEditor, SIGNAL(modified()), d->elementForm, SLOT(refreshElement()));
        connect(d->elementForm, SIGNAL(elementModified()), newFileView, SLOT(externalModification()));
        connect(d->elementForm, &ElementForm::elementModified, newFileView, &FileView::externalModification);
    }

    d->documentPreview->setBibTeXUrl(validFile ? openFileInfo->url() : QUrl());
    d->referencePreview->setElement(QSharedPointer<Element>(), NULL);
    d->elementForm->setElement(QSharedPointer<Element>(), NULL);
    d->documentPreview->setElement(QSharedPointer<Element>(), NULL);
    d->valueList->setFileView(newFileView);
    d->fileSettings->setFileView(newFileView);
    if (newFileView != NULL && newFileView->fileModel() != NULL)
        d->statistics->setFile(newFileView->fileModel()->bibliographyFile(), newFileView->selectionModel());
    else
        d->statistics->setFile(NULL, NULL);
    d->referencePreview->setFileView(newFileView);
}

void KBibTeXMainWindow::showSearchResults()
{
    d->dockSearchResults->show();
}

void KBibTeXMainWindow::documentListsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(OpenFileInfo::RecentlyUsed)) {
        OpenFileInfoManager::OpenFileInfoList list = OpenFileInfoManager::instance()->filteredItems(OpenFileInfo::RecentlyUsed);
        d->actionMenuRecentFilesMenu->clear();
        foreach (OpenFileInfo *cur, list) {
            /// Fixing bug 19511: too long filenames make menu too large,
            /// therefore squeeze text if it is longer than squeezeLen.
            const int squeezeLen = 64;
            const QString squeezedShortCap = squeeze_text(cur->shortCaption(), squeezeLen);
            const QString squeezedFullCap = squeeze_text(cur->fullCaption(), squeezeLen);
            QAction *action = new QAction(QString("%1 [%2]").arg(squeezedShortCap).arg(squeezedFullCap), this);
            action->setData(cur->url());
            action->setIcon(QIcon::fromTheme(cur->mimeType().replace(QLatin1Char('/'), QLatin1Char('-'))));
            d->actionMenuRecentFilesMenu->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        }
    }
}

void KBibTeXMainWindow::openRecentFile()
{
    QAction *action = static_cast<QAction *>(sender());
    QUrl url = action->data().value<QUrl>();
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
    static BibliographyService *bs = NULL;

    if (bs == NULL) {
        /// First call to this slot
        bs = new BibliographyService(this);
        if (!bs->isKBibTeXdefault() && QMessageBox::question(this, i18n("Default Bibliography Editor"), i18n("KBibTeX is not the default editor for its bibliography formats like BibTeX or RIS.")/*, KGuiItem(i18n("Set as Default Editor")), KGuiItem(i18n("Keep settings unchanged"))*/) == QMessageBox::Yes) {
            bs->setKBibTeXasDefault();
            /// QTimer calls this slot again, but as 'bs' will not be NULL,
            /// the 'if' construct's 'else' path will be followed.
            QTimer::singleShot(5000, this, SLOT(delayed()));
        } else {
            /// KBibTeX is default application or user doesn't care,
            /// therefore clean up memory
            delete bs;
            bs = NULL;
        }
    } else {
        /// Second call to this slot. This time, clean up memory.
        bs->deleteLater();
        bs = NULL;
    }
}
