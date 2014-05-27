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

#include "partfactory.h"

#include <KComponentData>
#include <KAboutData>
#include <KLocale>
#include <KDebug>

#include "part.h"
#include "version.h"
//const char *versionNumber = "some SVN revision";

static const char PartId[] = "kbibtexpart";
static const char PartName[] = I18N_NOOP("KBibTeXPart");
static const char PartDescription[] = I18N_NOOP("BibTeX Editor Component");
static const char PartCopyright[] = "Copyright 2004-2013 Thomas Fischer";
static const char *programHomepage = "http://home.gna.org/kbibtex/";

static KComponentData *_componentData = 0;
static KAboutData *_aboutData = 0;


KBibTeXPartFactory::KBibTeXPartFactory()
        : KParts::Factory()
{
    kDebug() << "Creating KBibTeXPart version" << versionNumber;
}


KBibTeXPartFactory::~KBibTeXPartFactory()
{
    delete _componentData;
    delete _aboutData;

    _componentData = 0;
}

KParts::Part *KBibTeXPartFactory::createPartObject(QWidget *parentWidget, QObject *parent, const char *cn, const QStringList &/*args*/)
{
    const QByteArray classname(cn);
    const bool browserViewWanted = (classname == "Browser/View");   // FIXME Editor?
    bool readOnlyWanted = (browserViewWanted || (classname == "KParts::ReadOnlyPart"));

    KBibTeXPart *part = new KBibTeXPart(parentWidget, parent, browserViewWanted);
    part->setReadWrite(!readOnlyWanted);

    return part;
}

const KComponentData &KBibTeXPartFactory::componentData()
{
    if (!_componentData) {
        _aboutData = new KAboutData(PartId, 0, ki18n(PartName), versionNumber,
                                    ki18n(PartDescription), KAboutData::License_GPL_V2,
                                    ki18n(PartCopyright), KLocalizedString(),
                                    programHomepage);
        _aboutData->addAuthor(ki18n("Thomas Fischer"), ki18n("Maintainer"), "fischer@unix-ag.uni-kl.de", "http://www.t-fischer.net/");
        _componentData = new KComponentData(_aboutData);
    }
    return *_componentData;
}


K_EXPORT_COMPONENT_FACTORY(kbibtexpart, KBibTeXPartFactory)

#include "partfactory.moc"
