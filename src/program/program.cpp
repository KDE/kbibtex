/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <cstdlib>

#include <QtDebug>
#include <QApplication>
#include <QCommandLineParser>
#include <QRegularExpression>

#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>
#include <KCrash>

#include "mainwindow.h"
#include "kbibtex-version.h"
#include "kbibtex-git-info.h"
#include "logging_program.h"

int main(int argc, char *argv[])
{
    if (strlen(KBIBTEX_GIT_INFO_STRING) > 0) {
        /// In Git versions, enable debugging by default
        QLoggingCategory::setFilterRules(QStringLiteral("kbibtex.*.debug = true"));
    }

    QApplication programCore(argc, argv);

    KCrash::initialize();

    KLocalizedString::setApplicationDomain("kbibtex");

    KAboutData aboutData(QStringLiteral("kbibtex"), i18n("KBibTeX"), strlen(KBIBTEX_GIT_INFO_STRING) > 0 ? QLatin1String(KBIBTEX_GIT_INFO_STRING ", near " KBIBTEX_VERSION_STRING) : QLatin1String(KBIBTEX_VERSION_STRING), i18n("A BibTeX editor by KDE"), KAboutLicense::GPL_V2, i18n("Copyright 2004-2018 Thomas Fischer"), QString(), QStringLiteral("https://userbase.kde.org/KBibTeX"));

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kbibtex"));

    aboutData.addAuthor(i18n("Thomas Fischer"), i18n("Maintainer"), QStringLiteral("fischer@unix-ag.uni-kl.de"));

    KAboutData::setApplicationData(aboutData);

    programCore.setApplicationName(aboutData.componentName());
    programCore.setOrganizationDomain(aboutData.organizationDomain());
    programCore.setApplicationVersion(aboutData.version());
    programCore.setApplicationDisplayName(aboutData.displayName());
    programCore.setWindowIcon(QIcon::fromTheme(QStringLiteral("kbibtex")));

    qCInfo(LOG_KBIBTEX_PROGRAM) << "Starting KBibTeX version" << (strlen(KBIBTEX_GIT_INFO_STRING) > 0 ? QLatin1String(KBIBTEX_GIT_INFO_STRING) : QLatin1String(KBIBTEX_VERSION_STRING));

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();
    cmdLineParser.addPositionalArgument(QStringLiteral("urls"), i18n("File(s) to load."), QStringLiteral("[urls...]"));

    cmdLineParser.process(programCore);
    aboutData.processCommandLine(&cmdLineParser);

    KBibTeXMainWindow *mainWindow = new KBibTeXMainWindow();

    const QStringList urls = cmdLineParser.positionalArguments();
    /// Process arguments
    if (!urls.isEmpty())
    {
        static const QRegularExpression protocolCheckerRegExp(QStringLiteral("^[a-zA-Z]+:"));
        for (const QString &url : urls) {
            const QUrl u = protocolCheckerRegExp.match(url).hasMatch() ? QUrl::fromUserInput(url) : QUrl::fromLocalFile(url);
            mainWindow->openDocument(u);
        }
    }

    mainWindow->show();

    KService::Ptr service = KService::serviceByStorageId(QStringLiteral("kbibtexpart.desktop"));
    if (service.data() == nullptr) {
        /// Dump some environment variables that may be helpful
        /// in tracing back why the part's .desktop file was not found
        qCDebug(LOG_KBIBTEX_PROGRAM) << "PATH=" << getenv("PATH");
        qCDebug(LOG_KBIBTEX_PROGRAM) << "LD_LIBRARY_PATH=" << getenv("LD_LIBRARY_PATH");
        qCDebug(LOG_KBIBTEX_PROGRAM) << "XDG_DATA_DIRS=" << getenv("XDG_DATA_DIRS");
        qCDebug(LOG_KBIBTEX_PROGRAM) << "QT_PLUGIN_PATH=" << getenv("QT_PLUGIN_PATH");
        qCDebug(LOG_KBIBTEX_PROGRAM) << "KDEDIRS=" << getenv("KDEDIRS");
        qCDebug(LOG_KBIBTEX_PROGRAM) << "QCoreApplication::libraryPaths=" << programCore.libraryPaths().join(QLatin1Char(':'));
        KMessageBox::error(mainWindow, i18n("KBibTeX seems to be not installed completely. KBibTeX could not locate its own KPart.\n\nOnly limited functionality will be available."), i18n("Incomplete KBibTeX Installation"));
    } else {
        qCInfo(LOG_KBIBTEX_PROGRAM) << "Located KPart service:" << service->library() << "with description" << service->comment() << "from library" << service->library();
    }

    /*
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
    */

    return programCore.exec();
}
