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

#include <QDebug>
#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KLocalizedString>

#include "mainwindow.h"
#include "version.h"

const char *description = I18N_NOOP("A BibTeX editor for KDE");
const char *programHomepage = "http://home.gna.org/kbibtex/";

int main(int argc, char *argv[])
{
    QApplication programCore(argc, argv);

    KLocalizedString::setApplicationDomain("kbibtex");

    KAboutData aboutData(QLatin1String("kbibtex"), i18n("KBibTeX"), QLatin1String(versionNumber), i18n("A BibTeX editor for KDE"), KAboutLicense::GPL_V2, i18n("Copyright 2004-2015 Thomas Fischer"), QString(), QLatin1String(programHomepage));

    aboutData.addAuthor(i18n("Thomas Fischer"), i18n("Maintainer"), QLatin1String("fischer@unix-ag.uni-kl.de"));

    KAboutData::setApplicationData(aboutData);

    programCore.setApplicationName(aboutData.componentName());
    programCore.setOrganizationDomain(aboutData.organizationDomain());
    programCore.setApplicationVersion(aboutData.version());
    programCore.setApplicationDisplayName(aboutData.displayName());
    programCore.setWindowIcon(QIcon::fromTheme(QLatin1String("kbibtex")));

    //  KCmdLineOptions programOptions;
    //  programOptions.add("+[URL(s)]", ki18n("File(s) to load"), 0);
    //  KCmdLineArgs::addCmdLineOptions(programOptions);

    //  KAboutData::setApplicationData(aboutData);
    //  KCmdLineArgs::init(argc, argv, &aboutData);
    // KApplication programCore;

    qDebug() << "Starting KBibTeX version" << versionNumber;

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();
    cmdLineParser.addPositionalArgument(QStringLiteral("urls"), i18n("File(s) to load."), QStringLiteral("[urls...]"));

    cmdLineParser.process(programCore);
    aboutData.processCommandLine(&cmdLineParser);

    KBibTeXMainWindow *mainWindow = new KBibTeXMainWindow();

    const QStringList urls = cmdLineParser.positionalArguments();
    // Process arguments
    if (!urls.isEmpty())
    {
        const QRegExp withProtocolChecker(QStringLiteral("^[a-zA-Z]+:"));
        foreach (const QString &url, urls) {
            const QUrl u = (withProtocolChecker.indexIn(url) == 0) ? QUrl::fromUserInput(url) : QUrl::fromLocalFile(url);
            mainWindow->openDocument(u);
        }
    }

    mainWindow->show();

    //}


    /*
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
            const QUrl url = arguments->url(i);
            if (url.isValid())
                mainWindow->openDocument(url);
        }
        mainWindow->show();

        arguments->clear();
    }

    return programCore.exec();
    */


}


