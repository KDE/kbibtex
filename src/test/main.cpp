#include <KCmdLineArgs>
#include <KApplication>
#include <KAboutData>

#include "kbibtextest.h"

const char *description = I18N_NOOP("A BibTeX editor for KDE");
const char *programHomepage = I18N_NOOP("http://home.gna.org/kbibtex/");
const char *bugTrackerHomepage = "https://gna.org/bugs/?group=kbibtex";

int main(int argc, char *argv[])
{
    KAboutData aboutData("kbibtextest", 0, ki18n("KBibTeX Test"), "XXX",
                         ki18n(description), KAboutData::License_GPL_V2,
                         ki18n("Copyright 2004-2012 Thomas Fischer"), KLocalizedString(),
                         programHomepage, bugTrackerHomepage);
    aboutData.addAuthor(ki18n("Thomas Fischer"), ki18n("Maintainer"), "fischer@unix-ag.uni-kl.de", "http://www.t-fischer.net/");
    aboutData.setCustomAuthorText(ki18n("Please use https://gna.org/bugs/?group=kbibtex to report bugs.\n"), ki18n("Please use <a href=\"https://gna.org/bugs/?group=kbibtex\">https://gna.org/bugs/?group=kbibtex</a> to report bugs.\n"));

    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication programCore;

    KBibTeXTest *test = new KBibTeXTest();
    test->exec();

    return programCore.exec();
}

