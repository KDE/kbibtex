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
#include <valuelist.h>
#include "documentlist.h"
#include "program.h"
#include "mdiwidget.h"
#include "referencepreview.h"
#include "urlpreview.h"
#include "searchform.h"
#include "searchresults.h"
#include "elementform.h"
#include "openfileinfo.h"
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
    KBibTeXProgram *program;
    DocumentList *listDocumentList;
    MDIWidget *mdiWidget;
    ReferencePreview *referencePreview;
    UrlPreview *urlPreview;
    SearchForm *searchForm;
    SearchResults *searchResults;
    ElementForm *elementForm;
    OpenFileInfoManager *openFileInfoManager;
    QLabel *emptyValueList;

    KBibTeXMainWindowPrivate(KBibTeXMainWindow *parent)
            : p(parent) {
        // nothing
    }
};

KBibTeXMainWindow::KBibTeXMainWindow(KBibTeXProgram *program)
        : KParts::MainWindow(), d(new KBibTeXMainWindowPrivate(this))
{
    d->program = program;
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
    connect(d->openFileInfoManager, SIGNAL(currentChanged(OpenFileInfo*)), d->mdiWidget, SLOT(setFile(OpenFileInfo*)));
    connect(d->openFileInfoManager, SIGNAL(closing(OpenFileInfo*)), d->mdiWidget, SLOT(closeFile(OpenFileInfo*)));


    KActionMenu *showPanelsAction = new KActionMenu(i18n("Show Panels"), this);
    actionCollection()->addAction("settings_shown_panels", showPanelsAction);
    KMenu *showPanelsMenu = new KMenu(showPanelsAction->text(), widget());
    showPanelsAction->setMenu(showPanelsMenu);

    d->dockDocumentList = new QDockWidget(i18n("List of Documents"), this);
    d->dockDocumentList->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockDocumentList);
    d->listDocumentList = new DocumentList(OpenFileInfoManager::getOpenFileInfoManager(), d->dockDocumentList);
    d->dockDocumentList->setWidget(d->listDocumentList);
    d->dockDocumentList->setObjectName("dockDocumentList");
    d->dockDocumentList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    // connect(d->listDocumentList, SIGNAL(open(const KUrl &, const QString&)), this, SLOT(openDocument(const KUrl&, const QString&)));
    showPanelsMenu->addAction(d->dockDocumentList->toggleViewAction());

    d->dockReferencePreview = new QDockWidget(i18n("Reference Preview"), this);
    d->dockReferencePreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockReferencePreview);
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
    d->dockSearchForm->setWidget(d->searchForm);
    d->dockSearchForm->setObjectName("dockSearchFrom");
    d->dockSearchForm->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    showPanelsMenu->addAction(d->dockSearchForm->toggleViewAction());

    d->dockValueList = new QDockWidget(i18n("List of Values"), this);
    d->dockValueList->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockValueList);
    d->emptyValueList = new QLabel(i18n("No bibliography selected"), d->dockValueList);
    d->emptyValueList->setAlignment(Qt::AlignCenter);
    d->dockValueList->setWidget(d->emptyValueList);
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
            openDocument(*it, "latex");
}

void KBibTeXMainWindow::newDocument()
{
    const QString mimeType = OpenFileInfo::mimetypeBibTeX;
    OpenFileInfo *openFileInfo = d->openFileInfoManager->createNew(mimeType);
    if (openFileInfo) {
        openFileInfo->setProperty(OpenFileInfo::propertyEncoding, "UTF-8");
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

    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getOpenUrlAndEncoding(QString(), startDir, QLatin1String("text/x-bibtex application/xml application/pdf all/all"), this); // TODO application/x-research-info-systems application/x-endnote-refer
    if (!loadResult.URLs.isEmpty()) {
        KUrl url = loadResult.URLs.first();
        if (!url.isEmpty()) {
            openDocument(url, loadResult.encoding);
        }
    }
}

void KBibTeXMainWindow::openDocument(const KUrl& url, const QString& encoding)
{
    kDebug() << "Opening document " << url.prettyUrl() << " with encoding " << encoding << endl;
    OpenFileInfo *openFileInfo = d->openFileInfoManager->open(url);
    openFileInfo->setProperty(OpenFileInfo::propertyEncoding, encoding);
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
        disconnect(d->elementForm, SIGNAL(elementModified()), newEditor, SLOT(externalModification()));
    }
    if (newEditor != NULL) {
        connect(newEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->referencePreview, SLOT(setElement(Element*, const File *)));
        connect(newEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->elementForm, SLOT(setElement(Element*, const File *)));
        connect(newEditor, SIGNAL(currentElementChanged(Element*, const File *)), d->urlPreview, SLOT(setElement(Element*, const File *)));
        connect(d->elementForm, SIGNAL(elementModified()), newEditor, SLOT(externalModification()));

        d->dockValueList->setWidget(newEditor->valueListWidget());
    } else
        d->dockSearchForm->setWidget(d->emptyValueList);

    d->urlPreview->setBibTeXUrl(validFile ? openFileInfo->url() : KUrl());
    d->referencePreview->setElement(NULL, NULL);
    d->elementForm->setElement(NULL, NULL);
    d->urlPreview->setElement(NULL, NULL);
}
