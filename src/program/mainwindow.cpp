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
#include <QListWidget>

#include <kio/netaccess.h>
#include <KDebug>
#include <KApplication>
#include <KEncodingFileDialog>
#include <KGlobal>
#include <KActionCollection>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KMessageBox>

#include "mainwindow.h"
#include "documentlist.h"
#include "program.h"

using namespace KBibTeX::Program;

KBibTeXMainWindow::KBibTeXMainWindow(KBibTeXProgram *program)
        : /* ShellWindow( program->documentManager(), program->viewManager() ),*/ m_program(program)
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

    // this routine will find and load our Part.
    KPluginFactory* factory = KPluginLoader("libsimplekbibtexpart").factory();
    if (factory) {
        // now that the Part is loaded, we cast it to a Part to get
        // our hands on it
        m_part = factory->create<KParts::ReadWritePart>(this);

        if (m_part) {
            // tell the KParts::MainWindow that this is indeed
            // the main widget
            setCentralWidget(m_part->widget());

            setupGUI(ToolBar | Keys | StatusBar | Save);

            // and integrate the part's GUI with the shell's
            createGUI(m_part);
        }
    } else {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        KMessageBox::error(this, "Could not find our Part!");
        kapp->quit();
        // we return here, cause qApp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }

    actionCollection()->addAction(KStandardAction::Open,  this, SLOT(slotOpenFile()));
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

void KBibTeXMainWindow::slotOpenFile()
{
    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getOpenUrlAndEncoding(QString(), ":open", QString(), this);
    KUrl url = loadResult.URLs.first();
    if (!url.isEmpty())
        m_listDocumentList->addOpen(url);

}

void KBibTeXMainWindow::openDocument(const KUrl& url)
{
    kDebug() << "Opening document \"" << url.prettyUrl() << "\"" << endl;
    m_part->openUrl(url);
}


