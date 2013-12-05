/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <KCmdLineArgs>
#include <KApplication>
#include <KAboutData>
#include <KMessageBox>
#include <KDebug>

#include "mainwindow.h"
#include "version.h"

const char *description = I18N_NOOP("A BibTeX editor for KDE");
const char *programHomepage = I18N_NOOP("http://home.gna.org/kbibtex/");
const char *bugTrackerHomepage = "https://gna.org/bugs/?group=kbibtex";


int main(int argc, char *argv[])
{
    KAboutData aboutData("kbibtex", 0, ki18n("KBibTeX"), versionNumber,
                         ki18n(description), KAboutData::License_GPL_V2,
                         ki18n("Copyright 2004-2013 Thomas Fischer"), KLocalizedString(),
                         programHomepage, bugTrackerHomepage);
    aboutData.addAuthor(ki18n("Thomas Fischer"), ki18n("Maintainer"), "fischer@unix-ag.uni-kl.de", "http://www.t-fischer.net/");
    aboutData.setCustomAuthorText(ki18n("Please use https://gna.org/bugs/?group=kbibtex to report bugs.\n"), ki18n("Please use <a href=\"https://gna.org/bugs/?group=kbibtex\">https://gna.org/bugs/?group=kbibtex</a> to report bugs.\n"));

    KCmdLineOptions programOptions;
    programOptions.add("+[URL(s)]", ki18n("File(s) to load"), 0);
    KCmdLineArgs::addCmdLineOptions(programOptions);

    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication programCore;

    kDebug() << "Starting KBibTeX version" << versionNumber;

    KService::Ptr service = KService::serviceByStorageId("kbibtexpart.desktop");
    if (service.isNull())
        KMessageBox::error(NULL, i18n("KBibTeX seems to be not installed completely. KBibTeX could not locate its own KPart.\n\nOnly limited functionality will be available."), i18n("Incomplete KBibTeX Installation"));

    /// started by session management?
    if (programCore.isSessionRestored()) {
        RESTORE(KBibTeXMainWindow());
    } else {
        /// no session.. just start up normally
        KBibTeXMainWindow *mainWindow = new KBibTeXMainWindow();

        KCmdLineArgs *arguments = KCmdLineArgs::parsedArgs();

        for (int i = 0; i < arguments->count(); ++i) {
            KUrl url(arguments->arg(i));
            if (url.isValid())
                mainWindow->openDocument(url);
        }
        mainWindow->show();

        arguments->clear();
    }

    return programCore.exec();
}


