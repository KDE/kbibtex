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

#include <KGlobal>

#include "mainwindow.h"
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

void KBibTeXMainWindow::saveProperties(KConfigGroup &configGroup)
{
    // TODO
}

void KBibTeXMainWindow::readProperties(const KConfigGroup &configGroup)
{
    // TOD
}
