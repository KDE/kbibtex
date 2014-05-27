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

#include <QPointer>

#include <KCmdLineArgs>
#include <KApplication>
#include <KAboutData>

#include "kbibtextest.h"

const char *description = I18N_NOOP("A BibTeX editor for KDE");
const char *programHomepage = "http://home.gna.org/kbibtex/";
const char *bugTrackerHomepage = "https://gna.org/bugs/?group=kbibtex";

int main(int argc, char *argv[])
{
    KAboutData aboutData("kbibtextest", 0, ki18n("KBibTeX Test"), "XXX",
                         ki18n(description), KAboutData::License_GPL_V2,
                         ki18n("Copyright 2004-2013 Thomas Fischer"), KLocalizedString(),
                         programHomepage, bugTrackerHomepage);
    aboutData.addAuthor(ki18n("Thomas Fischer"), ki18n("Maintainer"), "fischer@unix-ag.uni-kl.de", "http://www.t-fischer.net/");
    aboutData.setCustomAuthorText(ki18n("Please use https://gna.org/bugs/?group=kbibtex to report bugs.\n"), ki18n("Please use <a href=\"https://gna.org/bugs/?group=kbibtex\">https://gna.org/bugs/?group=kbibtex</a> to report bugs.\n"));

    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication programCore;

    QPointer<KBibTeXTest> test = new KBibTeXTest();
    test->exec();

    return programCore.exec();
}

