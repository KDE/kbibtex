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
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include "bibtexfields.h"

using namespace KBibTeX::GUI::Config;

BibTeXFields *BibTeXFields::m_self = NULL;

BibTeXFields::BibTeXFields()
{
    m_config = new KConfig(KStandardDirs::locateLocal("data", "kbibtex/fieldtypes.rc"), KConfig::NoGlobals);
    load();
}

BibTeXFields::~BibTeXFields()
{
    delete m_config;
}

BibTeXFields* BibTeXFields::self()
{
    if (m_self == NULL)
        m_self = new BibTeXFields();
    return m_self;
}

void BibTeXFields::load()
{
    FieldDescription fd;

    fd.raw = QString("^type"); // FIXME QLatin1String
    fd.label = QString("Type"); // FIXME i18n
    fd.width = 5;
    append(fd);

    fd.raw = QString("^id"); // FIXME QLatin1String
    fd.label = QString("Identifier"); // FIXME i18n
    fd.width = 6;
    append(fd);

    QStringList groupList = m_config->groupList();

    for (QStringList::ConstIterator it = groupList.begin(); it != groupList.end(); ++it) {
        KConfigGroup group(m_config, *it);
        fd.raw = *it;
        fd.label = group.readEntry("Label", *it);
        fd.width = group.readEntry("Width", 4);
        append(fd);
    }
}

void BibTeXFields::save()
{
// TODO
}
