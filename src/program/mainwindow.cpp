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

#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>

#include <kio/netaccess.h>
#include <KDebug>
#include <KApplication>
#include <KAction>
#include <KActionMenu>
#include <KEncodingFileDialog>
#include <KGlobal>
#include <KActionCollection>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KMessageBox>
#include <KMenu>

#include "mainwindow.h"
#include "valuelist.h"
#include "documentlist.h"
#include "mdiwidget.h"
#include "referencepreview.h"
#include "urlpreview.h"
#include "searchform.h"
#include "searchresults.h"
#include "elementform.h"
#include "bibtexeditor.h"
#include "documentlist.h"

class KBibTeXMainWindow::KBibTeXMainWindowPrivate
{
private:
    KBibTeXMainWindow *p;

public:
    KAction *actionClose;
    QDockWidget *dockDocumentList;
    QDockWidget *dockReferencePreview;
    QDockWidget *dockUrlPreview;
    QDockWidget *dockValueList;
    QDockWidget *dockSearchForm;
    QDockWidget *dockSearchResults;
    QDockWidget *dockElementForm;
    DocumentList *listDocumentList;
    MDIWidget *mdiWidget;
    ReferencePreview *referencePreview;
    UrlPreview *urlPreview;
    ValueList *valueList;
    SearchForm *searchForm;
    SearchResults *searchResults;
    ElementForm *elementForm;
    OpenFileInfoManager *openFileInfoManager;
    KMenu *actionMenuRecentFilesMenu;

    KBibTeXMainWindowPrivate(KBibTeXMainWindow *parent)
            : p(parent) {
// nothing
    }
};

KBibTeXMainWindow::KBibTeXMainWindow()
        : KParts::MainWindow(), d(new KBibTeXMainWindowPrivate(this))
{
    d->openFileInfoManager = OpenFileInfoManager::getOpenFileInfoManager();

    setObjectName(QLatin1String("KBibTeXShell"));

    /*
        const char mainWindowStateKey[] = "State";
        KConfigGroup group( KGlobal::config(), "MainWindow" );
        if( !group.hasKey(mainWindowStateKey) )
            group.writeEntry( mainWindowStateKey, mainWindowState );
    */

    setXMLFile("kbibtexui.rc");

    d->mdiWidget = new MDIWidget(this);
    setCentralWidget(d->mdiWidget);
    connect(d->mdiWidget, SIGNAL(documentSwitch(BibTeXEditor *, BibTeXEditor *)), this, SLOT(documentSwitched(BibTeXEditor *, BibTeXEditor *)));
    connect(d->mdiWidget, SIGNAL(activePartChanged(KParts::Part*)), this, SLOT(createGUI(KParts::Part*)));
    connect(d->mdiWidget, SIGNAL(documentNew()), this, SLOT(newDocument()));
    connect(d->mdiWidget, SIGNAL(documentOpen()), this, SLOT(openDocumentDialog()));
    connect(d->openFileInfoManager, SIGNAL(currentChanged(OpenFileInfo*, KService::Ptr)), d->mdiWidget, SLOT(setFile(OpenFileInfo*, KService::Ptr)));
    connect(d->openFileInfoManager, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SLOT(documentListsChanged(OpenFileInfo::StatusFlags)));
    connect(d->mdiWidget, SIGNAL(setCaption(QString)), this, SLOT(setCaption(QString)));

    KActionMenu *showPanelsAction = new KActionMenu(i18n("Show Panels"), this);
    actionCollection()->addAction("settings_shown_panels", showPanelsAction);
    KMenu *showPanelsMenu = new KMenu(showPanelsAction->text(), widget());
    showPanelsAction->setMenu(showPanelsMenu);

    KActionMenu *actionMenuRecentFiles = new KActionMenu(KIcon("document-open-recent"), i18n("Recently used files"), this);
    actionCollection()->addAction("file_open_recent", actionMenuRecentFiles);
    d->actionMenuRecentFilesMenu = new KMenu(actionMenuRecentFiles->text(), widget());
    actionMenuRecentFiles->setMenu(d->actionMenuRecentFilesMenu);

    d->dockDocumentList = new QDockWidget(i18n("List of Documents"), this);
    d->dockDocumentList->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockDocumentList);
    d->listDocumentList = new DocumentList(OpenFileInfoManager::getOpenFileInfoManager(), d->dockDocumentList);
    d->dockDocumentList->setWidget(d->listDocumentList);
    d->dockDocumentList->setObjectName("dockDocumentList");
    d->dockDocumentList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    connect(d->listDocumentList, SIGNAL(openFile(const KUrl&)), this, SLOT(openDocument(const KUrl&)));
    showPanelsMenu->addAction(d->dockDocumentList->toggleViewAction());

    d->dockReferencePreview = new QDockWidget(i18n("Reference Preview"), this);
    d->dockReferencePreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, d->dockReferencePreview);
    d->referencePreview = new ReferencePreview(d->dockReferencePreview);
    d->dockReferencePreview->setWidget(d->referencePreview);
    d->dockReferencePreview->setObjectName("dockReferencePreview");
    d->dockReferencePreview->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockReferencePreview->toggleViewAction());

    d->dockUrlPreview = new QDockWidget(i18n("Preview"), this);
    d->dockUrlPreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, d->dockUrlPreview);
    d->urlPreview = new UrlPreview(d->dockUrlPreview);
    d->dockUrlPreview->setWidget(d->urlPreview);
    d->dockUrlPreview->setObjectName("dockUrlPreview");
    d->dockUrlPreview->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockUrlPreview->toggleViewAction());

    d->dockSearchResults = new QDockWidget(i18n("Search Results"), this);
    d->dockSearchResults->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, d->dockSearchResults);
    d->searchResults = new SearchResults(d->mdiWidget, d->dockSearchResults);
    d->dockSearchResults->setWidget(d->searchResults);
    d->dockSearchResults->setObjectName("dockResultsFrom");
    d->dockSearchResults->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockSearchResults->toggleViewAction());
    connect(d->mdiWidget, SIGNAL(documentSwitch(BibTeXEditor *, BibTeXEditor *)), d->searchResults, SLOT(documentSwitched(BibTeXEditor *, BibTeXEditor *)));

    d->dockSearchForm = new QDockWidget(i18n("Online Search"), this);
    d->dockSearchForm->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockSearchForm);
    d->searchForm = new SearchForm(d->mdiWidget, d->searchResults, d->dockSearchForm);
    connect(d->searchForm, SIGNAL(doneSearching()), this, SLOT(showSearchResults()));
    d->dockSearchForm->setWidget(d->searchForm);
    d->dockSearchForm->setObjectName("dockSearchFrom");
    d->dockSearchForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockSearchForm->toggleViewAction());

    d->dockValueList = new QDockWidget(i18n("List of Values"), this);
    d->dockValueList->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockValueList);
    d->valueList = new ValueList(d->dockValueList);
    d->dockValueList->setWidget(d->valueList);
    d->dockValueList->setObjectName("dockValueList");
    d->dockValueList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockValueList->toggleViewAction());

    d->dockElementForm = new QDockWidget(i18n("Element Editor"), this);
    d->dockElementForm->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, d->dockElementForm);
    d->elementForm = new ElementForm(d->mdiWidget, d->dockElementForm);
    d->dockElementForm->setWidget(d->elementForm);
    d->dockElementForm->setObjectName("dockElementFrom");
    d->dockElementForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockElementForm->toggleViewAction());

    actionCollection()->addAction(KStandardAction::New, this, SLOT(newDocument()));
    actionCollection()->addAction(KStandardAction::Open, this, SLOT(openDocumentDialog()));
    d->actionClose = actionCollection()->addAction(KStandardAction::Close, this, SLOT(closeDocument()));
    d->actionClose->setEnabled(false);
    actionCollection()->addAction(KStandardAction::Quit,  kapp, SLOT(quit()));

    documentListsChanged(OpenFileInfo::RecentlyUsed); /// force initialization of menu of recently used files

    setupControllers();
    setupGUI();

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    setAcceptDrops(true);
}

KBibTeXMainWindow::~KBibTeXMainWindow()
{
    delete d->openFileInfoManager;
}

void KBibTeXMainWindow::setupControllers()
{
    // TODO
}

void KBibTeXMainWindow::saveProperties(KConfigGroup &/*configGroup*/)
{
    // TODO
}

void KBibTeXMainWindow::readProperties(const KConfigGroup &/*configGroup*/)
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
    const QString mimeType = OpenFileInfo::mimetypeBibTeX;
    OpenFileInfo *openFileInfo = d->openFileInfoManager->createNew(mimeType);
    if (openFileInfo) {
        d->openFileInfoManager->setCurrentFile(openFileInfo);
        openFileInfo->setFlags(OpenFileInfo::Open);
    } else
        KMessageBox::error(this, i18n("Creating a new document of mime type \"%1\" failed as no editor component could be instanticated.", mimeType), i18n("Creating document failed"));
}

void KBibTeXMainWindow::openDocumentDialog()
{
    QString startDir = QString();// QLatin1String(":open"); // FIXME: Does not work yet
    OpenFileInfo *ofi = d->openFileInfoManager->currentFile();
    if (ofi != NULL) {
        KUrl url = ofi->url();
        if (url.isValid()) startDir = url.path();
    }

    // TODO application/x-research-info-systems application/x-endnote-refer
    KUrl url = KFileDialog::getOpenUrl(startDir, QLatin1String("text/x-bibtex application/x-research-info-systems application/xml all/all"), this);
    if (!url.isEmpty()) {
        openDocument(url);
    }
}

void KBibTeXMainWindow::openDocument(const KUrl& url)
{
    OpenFileInfo *openFileInfo = d->openFileInfoManager->open(url);
    d->openFileInfoManager->setCurrentFile(openFileInfo);
}

void KBibTeXMainWindow::closeDocument()
{
    d->actionClose->setEnabled(false);
    d->openFileInfoManager->close(d->openFileInfoManager->currentFile());
}

void KBibTeXMainWindow::documentSwitched(BibTeXEditor *oldEditor, BibTeXEditor *newEditor)
{
    OpenFileInfo *openFileInfo = OpenFileInfoManager::getOpenFileInfoManager()->currentFile();
    bool validFile = openFileInfo != NULL;
    d->actionClose->setEnabled(validFile);

    setCaption(validFile ? i18n("%1 - KBibTeX", openFileInfo->shortCaption()) : i18n("KBibTeX"));

    d->referencePreview->setEnabled(newEditor != NULL);
    d->elementForm->setEnabled(newEditor != NULL);
    d->urlPreview->setEnabled(newEditor != NULL);
    if (oldEditor != NULL) {
        disconnect(oldEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->referencePreview, SLOT(setElement(Element*, const File *)));
        disconnect(oldEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->elementForm, SLOT(setElement(Element*, const File *)));
        disconnect(oldEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->urlPreview, SLOT(setElement(Element*, const File *)));
        disconnect(oldEditor, SIGNAL(modified()), d->valueList, SLOT(update()));
        disconnect(d->elementForm, SIGNAL(elementModified()), newEditor, SLOT(externalModification()));
    }
    if (newEditor != NULL) {
        connect(newEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->referencePreview, SLOT(setElement(Element*, const File *)));
        connect(newEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->elementForm, SLOT(setElement(Element*, const File *)));
        connect(newEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->urlPreview, SLOT(setElement(Element*, const File *)));
        connect(newEditor, SIGNAL(modified()), d->valueList, SLOT(update()));
        connect(d->elementForm, SIGNAL(elementModified()), newEditor, SLOT(externalModification()));
    }

    d->urlPreview->setBibTeXUrl(validFile ? openFileInfo->url() : KUrl());
    d->referencePreview->setElement(NULL, NULL);
    d->elementForm->setElement(NULL, NULL);
    d->urlPreview->setElement(NULL, NULL);
    d->valueList->setEditor(newEditor);
}

void KBibTeXMainWindow::showSearchResults()
{
    d->dockSearchResults->show();
}

void KBibTeXMainWindow::documentListsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(OpenFileInfo::RecentlyUsed)) {
        QList<OpenFileInfo*> list = d->openFileInfoManager->filteredItems(OpenFileInfo::RecentlyUsed);
        d->actionMenuRecentFilesMenu->clear();
        foreach(OpenFileInfo* cur, list) {
            KAction *action = new KAction(QString("%1 [%2]").arg(cur->shortCaption()).arg(cur->fullCaption()), this);
            action->setData(cur->url());
            action->setIcon(KIcon(cur->mimeType().replace("/", "-")));
            d->actionMenuRecentFilesMenu->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        }
    }
}

void KBibTeXMainWindow::openRecentFile()
{
    KAction *action = static_cast<KAction*>(sender());
    KUrl url = action->data().value<KUrl>();
    openDocument(url);
}
