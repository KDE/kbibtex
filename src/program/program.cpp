/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <kcmdlineargs.h>
#include <KApplication>
#include <KDebug>

#include "program.h"
#include "version.h"
#include "mainwindow.h"

static KAboutData aboutData("kbibtex", 0,
                            ki18n("KBibTeX"), versionNumber,
                            ki18n("A bibliography manager supporting BibTeX and other formats."), KAboutData::License_GPL_V2,
                            ki18n("Copyright 2004-2010, Thomas Fischer"), ki18n("Feedback:\nfischer@unix-ag.uni-kl.de"),
                            "http://home.gna.org/kbibtex/");

KBibTeXProgram::KBibTeXProgram(int argc, char *argv[])
/*: m_documentManager(new KDocumentManager()), m_viewManager(new KViewManager(m_documentManager))*/
{
    KCmdLineOptions programOptions;
    programOptions.add("+[URL(s)]", ki18n("File(s) to load"), 0);

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(programOptions);
}


KBibTeXProgram::~KBibTeXProgram()
{
    /*
        delete m_documentManager;
        delete m_viewManager;
    */
}

int KBibTeXProgram::execute()
{
    KApplication programCore;

    KGlobal::locale()->insertCatalog("libkbibtexio");
    KGlobal::locale()->insertCatalog("libkbibtexgui");

    // started by session management?
    if (programCore.isSessionRestored()) {
        RESTORE(KBibTeXMainWindow(this));
    } else {
        // no session.. just start up normally
        KBibTeXMainWindow *mainWindow = new KBibTeXMainWindow(this);

        KCmdLineArgs *arguments = KCmdLineArgs::parsedArgs();

        for (int i = 0; i < arguments->count(); ++i) {
            kDebug(0) << "Arg-" << i << " : " << arguments->arg(i) << endl;
            KUrl url(arguments->arg(i));
            if (url.isValid())
                mainWindow->openDocument(url, QString::null);
        }
        mainWindow->show();

        arguments->clear();
    }

    qApp->setStyleSheet("QFrame#FieldLineEdit { background-color: " + QPalette().color(QPalette::Base).name() + "; } QFrame#FieldLineEdit > QTextEdit { border-style: none; } QFrame#FieldLineEdit > KLineEdit { border-style: none; } QFrame#FieldLineEdit > KPushButton { border-style: none; background-color: " + QPalette().color(QPalette::Base).name() + "; padding: 0px; margin-left:2px; margin-right:2px; text-align: left; } QScrollArea QFrame#FieldLineEdit { border-style: none; }");
    return programCore.exec();
}


void KBibTeXProgram::quit()
{
    kapp->quit();
}




