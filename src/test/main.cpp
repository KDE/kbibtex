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

#include <QPointer>
#include <QApplication>

#include <KAboutData>
#include <KLocalizedString>

#include "kbibtextest.h"
#include "kbibtex-version.h"
#include "kbibtex-git-info.h"

int main(int argc, char *argv[])
{
    QApplication programCore(argc, argv);

    KAboutData aboutData(QStringLiteral("kbibtextest"), i18n("KBibTeX Test"), strlen(KBIBTEX_GIT_INFO_STRING) > 0 ? QLatin1String(KBIBTEX_GIT_INFO_STRING ", near " KBIBTEX_VERSION_STRING) : QLatin1String(KBIBTEX_VERSION_STRING), i18n("A BibTeX editor by KDE"), KAboutLicense::GPL_V2, i18n("Copyright 2004-2018 Thomas Fischer"), QString(), QStringLiteral("https://userbase.kde.org/KBibTeX"));
    aboutData.addAuthor(i18n("Thomas Fischer"), i18n("Maintainer"), QStringLiteral("fischer@unix-ag.uni-kl.de"));
    KAboutData::setApplicationData(aboutData);

    QPointer<KBibTeXTest> test = new KBibTeXTest();
    test->exec();

    return programCore.exec();
}
