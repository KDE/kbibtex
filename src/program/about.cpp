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

#include <KLocale>
#include <kdeversion.h>

#include "version.h"
#include "about.h"

const char *programVersion = versionNumber;
static const char programHomepage[] = "http://home.gna.org/kbibtex/";
static const char bugTrackerWebsite[] = "https://gna.org/bugs/?group=kbibtex";

KBibTeXAboutData::KBibTeXAboutData()
        : KAboutData("kbibtex", 0, ki18n("KBibTeX"), programVersion, ki18n("A BibTeX editor for KDE"), KAboutData::License_GPL_V2, ki18n("Copyright 2004-2011 Thomas Fischer"), ki18n("Edit bibliography files"), programHomepage)
{
    addAuthor(ki18n("Thomas Fischer"), ki18n("Author"), "fischer@unix-ag.uni-kl.de");
    setBugAddress(bugTrackerWebsite);
}
