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

#include "mainwindow.h"
#include "documentlist.h"
#include "program.h"
#include "mdiwidget.h"
#include "referencepreview.h"

using namespace KBibTeX::Program;

KBibTeXMainWindow::KBibTeXMainWindow(KBibTeXProgram *program)
        : KParts::MainWindow(), m_program(program)
{
    setObjectName(QLatin1String("Shell"));   // FIXME

    /*
        const char mainWindowStateKey[] = "State";
        KConfigGroup group( KGlobal::config(), "MainWindow" );
        if( !group.hasKey(mainWindowStateKey) )
            group.writeEntry( mainWindowStateKey, mainWindowState );
    */

    m_dockDocumentList = new QDockWidget(i18n("List of Documents"), this);
    m_dockDocumentList->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_dockDocumentList);
    m_listDocumentList = new DocumentList(m_dockDocumentList);
    m_dockDocumentList->setWidget(m_listDocumentList);
    m_dockDocumentList->setObjectName("dockDocumentList");
    m_dockDocumentList->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    connect(m_listDocumentList, SIGNAL(open(const KUrl &, const QString&)), this, SLOT(openDocument(const KUrl&, const QString&)));

    m_dockReferencePreview = new QDockWidget(i18n("Reference Preview"), this);
    m_dockReferencePreview->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockReferencePreview);
    m_referencePreview = new ReferencePreview(m_dockReferencePreview);
    m_dockReferencePreview->setWidget(m_referencePreview);
    m_dockReferencePreview->setObjectName("dockReferencePreview");
    m_dockReferencePreview->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    setXMLFile("kbibtexui.rc");

    m_mdiWidget = new MDIWidget(this);
    setCentralWidget(m_mdiWidget);
    connect(m_mdiWidget, SIGNAL(documentSwitch(KBibTeX::GUI::BibTeXEditor *, KBibTeX::GUI::BibTeXEditor *)), this, SLOT(documentSwitched(KBibTeX::GUI::BibTeXEditor *, KBibTeX::GUI::BibTeXEditor *)));
    connect(m_mdiWidget, SIGNAL(activePartChanged(KParts::Part*)), this, SLOT(createGUI(KParts::Part*)));

    actionCollection()->addAction(KStandardAction::New, this, SLOT(newDocument()));
    actionCollection()->addAction(KStandardAction::Open, this, SLOT(openDocumentDialog()));
    m_actionClose = actionCollection()->addAction(KStandardAction::Close, this, SLOT(closeDocument()));
    m_actionClose->setEnabled(false);
    actionCollection()->addAction(KStandardAction::Quit,  kapp, SLOT(quit()));

    setupControllers();
    setupGUI();
}

KBibTeXMainWindow::~KBibTeXMainWindow()
{
    // nothing
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
    // TODO
}

void KBibTeXMainWindow::openDocumentDialog()
{
    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getOpenUrlAndEncoding(QString(), ":open", QString(), this);
    if (!loadResult.URLs.isEmpty()) {
        KUrl url = loadResult.URLs.first();
        if (!url.isEmpty()) {
            m_listDocumentList->addToOpen(url, loadResult.encoding);
            openDocument(url, loadResult.encoding);
        }
    }
}

void KBibTeXMainWindow::closeDocument()
{
    KUrl url = m_mdiWidget->currentUrl();
    if (url.isValid()) {
        m_listDocumentList->closeUrl(url);
        m_mdiWidget->closeUrl(url);
    }
}

void KBibTeXMainWindow::documentSwitched(KBibTeX::GUI::BibTeXEditor *newEditor, KBibTeX::GUI::BibTeXEditor *oldEditor)
{
    KUrl url = m_mdiWidget->currentUrl();
    m_actionClose->setEnabled(url.isValid());

    setCaption(url.isValid() ? QString(i18n("%1 - KBibTeX")).arg(url.fileName()) : i18n("KBibTeX"));

    if (url.isValid())
        m_listDocumentList->highlightUrl(url);

    m_referencePreview->setEnabled(newEditor != NULL);
    if (oldEditor != NULL)
        disconnect(oldEditor, SIGNAL(currentElementChanged(const KBibTeX::IO::Element*)), m_referencePreview, SLOT(setElement(const KBibTeX::IO::Element*)));
    if (newEditor != NULL)
        connect(newEditor, SIGNAL(currentElementChanged(const KBibTeX::IO::Element*)), m_referencePreview, SLOT(setElement(const KBibTeX::IO::Element*)));
}

void KBibTeXMainWindow::openDocument(const KUrl& url, const QString& encoding)
{
    kDebug() << "Opening document " << url.prettyUrl() << " with encoding " << encoding << endl;
    m_mdiWidget->setUrl(url, encoding);
}
