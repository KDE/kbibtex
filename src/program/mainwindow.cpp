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

#include <kio/netaccess.h>
#include <KDebug>
#include <KApplication>
#include <KAction>
#include <KEncodingFileDialog>
#include <KGlobal>
#include <KActionCollection>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KMessageBox>

#include "mainwindow.h"
#include "documentlist.h"
#include "program.h"
#include "mdiwidget.h"
#include "referencepreview.h"
#include "openfileinfo.h"
#include "bibtexeditor.h"
#include "documentlist.h"

using namespace KBibTeX::Program;

class KBibTeXMainWindow::KBibTeXMainWindowPrivate
{
private:
    KBibTeXMainWindow *p;

public:
    KAction *actionClose;
    QDockWidget *dockDocumentList;
    QDockWidget *dockReferencePreview;
    KBibTeXProgram *program;
    DocumentList *listDocumentList;
    MDIWidget *mdiWidget;
    ReferencePreview *referencePreview;
    OpenFileInfoManager *openFileInfoManager;

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

    d->dockDocumentList = new QDockWidget(i18n("List of Documents"), this);
    d->dockDocumentList->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, d->dockDocumentList);
    d->listDocumentList = new DocumentList(OpenFileInfoManager::getOpenFileInfoManager(), d->dockDocumentList);
    d->dockDocumentList->setWidget(d->listDocumentList);
    d->dockDocumentList->setObjectName("dockDocumentList");
    d->dockDocumentList->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    // connect(d->listDocumentList, SIGNAL(open(const KUrl &, const QString&)), this, SLOT(openDocument(const KUrl&, const QString&)));

    d->dockReferencePreview = new QDockWidget(i18n("Reference Preview"), this);
    d->dockReferencePreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, d->dockReferencePreview);
    d->referencePreview = new ReferencePreview(d->dockReferencePreview);
    d->dockReferencePreview->setWidget(d->referencePreview);
    d->dockReferencePreview->setObjectName("dockReferencePreview");
    d->dockReferencePreview->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    setXMLFile("kbibtexui.rc");

    d->mdiWidget = new MDIWidget(this);
    setCentralWidget(d->mdiWidget);
    connect(d->mdiWidget, SIGNAL(documentSwitch(KBibTeX::GUI::BibTeXEditor *, KBibTeX::GUI::BibTeXEditor *)), this, SLOT(documentSwitched(KBibTeX::GUI::BibTeXEditor *, KBibTeX::GUI::BibTeXEditor *)));
    connect(d->mdiWidget, SIGNAL(activePartChanged(KParts::Part*)), this, SLOT(createGUI(KParts::Part*)));
    connect(d->openFileInfoManager, SIGNAL(currentChanged(OpenFileInfo*)), d->mdiWidget, SLOT(setFile(OpenFileInfo*)));
    connect(d->openFileInfoManager, SIGNAL(closing(OpenFileInfo*)), d->mdiWidget, SLOT(closeFile(OpenFileInfo*)));

    actionCollection()->addAction(KStandardAction::New, this, SLOT(newDocument()));
    actionCollection()->addAction(KStandardAction::Open, this, SLOT(openDocumentDialog()));
    d->actionClose = actionCollection()->addAction(KStandardAction::Close, this, SLOT(closeDocument()));
    d->actionClose->setEnabled(false);
    actionCollection()->addAction(KStandardAction::Quit,  kapp, SLOT(quit()));

    setupControllers();
    setupGUI();
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
    QString startDir = QLatin1String(":open");
    OpenFileInfo *ofi = d->openFileInfoManager->currentFile();
    if (ofi != NULL) {
        KUrl url = ofi->url();
        if (url.isValid()) startDir = url.path();
    }

    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getOpenUrlAndEncoding(QString(), startDir, QLatin1String("text/x-bibtex application/x-research-info-systems application/x-endnote-refer all/all"), this);
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

void KBibTeXMainWindow::documentSwitched(KBibTeX::GUI::BibTeXEditor *oldEditor, KBibTeX::GUI::BibTeXEditor *newEditor)
{
    OpenFileInfo *openFileInfo = OpenFileInfoManager::getOpenFileInfoManager()->currentFile();
    bool validFile = openFileInfo != NULL;
    d->actionClose->setEnabled(validFile);

    setCaption(validFile ? i18n("%1 - KBibTeX", openFileInfo->caption()) : i18n("KBibTeX"));

    d->referencePreview->setEnabled(newEditor != NULL);
    if (oldEditor != NULL)
        disconnect(oldEditor, SIGNAL(currentElementChanged(const KBibTeX::IO::Element*)), d->referencePreview, SLOT(setElement(const KBibTeX::IO::Element*)));
    if (newEditor != NULL)
        connect(newEditor, SIGNAL(currentElementChanged(const KBibTeX::IO::Element*)), d->referencePreview, SLOT(setElement(const KBibTeX::IO::Element*)));
    d->referencePreview->setElement(NULL);
}

