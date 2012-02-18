/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <kcomponentdata.h>
#include <KAboutData>
#include <klocale.h>

#include "part.h"
#include "partfactory.h"
#include "version.h"

static const char PartId[] =           "kbibtexpart";
static const char PartName[] =         I18N_NOOP("KBibTeXPart");
static const char PartDescription[] =  I18N_NOOP("BibTeX Editor Component");
static const char PartCopyright[] =    "Copyright 2004-2012 Thomas Fischer";
static const char *programHomepage = I18N_NOOP("http://home.gna.org/kbibtex/");
static const char *bugTrackerHomepage = "https://gna.org/bugs/?group=kbibtex";

static KComponentData *_componentData = 0;
static KAboutData* _aboutData = 0;


KBibTeXPartFactory::KBibTeXPartFactory()
        : KParts::Factory()
{
    // nothing
}


KBibTeXPartFactory::~KBibTeXPartFactory()
{
    delete _componentData;
    delete _aboutData;

    _componentData = 0;
}

KParts::Part* KBibTeXPartFactory::createPartObject(QWidget *parentWidget, QObject *parent, const char *cn, const QStringList &/*args*/)
{
    const QByteArray classname(cn);
    const bool browserViewWanted = (classname == "Browser/View");   // FIXME Editor?
    bool readOnlyWanted = (browserViewWanted || (classname == "KParts::ReadOnlyPart"));

    KBibTeXPart* part = new KBibTeXPart(parentWidget, parent, browserViewWanted);
    part->setReadWrite(!readOnlyWanted);

    return part;
}

const KComponentData &KBibTeXPartFactory::componentData()
{
    if (!_componentData) {
        _aboutData = new KAboutData(PartId, 0, ki18n(PartName), versionNumber,
                                    ki18n(PartDescription), KAboutData::License_GPL_V2,
                                    ki18n(PartCopyright), KLocalizedString(),
                                    programHomepage, bugTrackerHomepage);
        _aboutData->addAuthor(ki18n("Thomas Fischer"), ki18n("Maintainer"), "fischer@unix-ag.uni-kl.de", "http://www.t-fischer.net/");
        _aboutData->setCustomAuthorText(ki18n("Please use https://gna.org/bugs/?group=kbibtex to report bugs.\n"), ki18n("Please use <a href=\"https://gna.org/bugs/?group=kbibtex\">https://gna.org/bugs/?group=kbibtex</a> to report bugs.\n"));
        _componentData = new KComponentData(_aboutData);
    }
    return *_componentData;
}


K_EXPORT_COMPONENT_FACTORY(kbibtexpart, KBibTeXPartFactory)

#include "partfactory.moc"
