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
#include <KSharedConfig>
#include <kconfiggroup.h>

#include "bibtexfields.h"

using namespace KBibTeX::GUI::Config;

BibTeXFields *BibTeXFields::m_self = NULL;
const int maxColumns = 256;

BibTeXFields::BibTeXFields()
{
    m_systemDefaultsConfig = new KConfig(KStandardDirs::locate("appdata", "fieldtypes.rc"), KConfig::SimpleConfig);
    m_userConfig = KSharedConfig::openConfig(KStandardDirs::locateLocal("appdata", "ui.rc"), KConfig::SimpleConfig);
    load();
}

BibTeXFields::~BibTeXFields()
{
    delete m_systemDefaultsConfig;
}

BibTeXFields* BibTeXFields::self()
{
    if (m_self == NULL)
        m_self = new BibTeXFields();
    return m_self;
}

void BibTeXFields::load()
{
    unsigned int sumWidth = 5;
    FieldDescription fd;

    clear();
    for (int col = 1; col < maxColumns; ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(m_userConfig, groupName);
        KConfigGroup systemcg(m_systemDefaultsConfig, groupName);
        fd.raw = systemcg.readEntry("Raw", "");
        if (fd.raw.isEmpty()) continue;
        fd.rawAlt = systemcg.readEntry("RawAlt", "");
        if (fd.rawAlt.isEmpty()) fd.rawAlt = QString::null;
        fd.label = systemcg.readEntry("Label", fd.raw);
        fd.defaultWidth = systemcg.readEntry("DefaultWidth", sumWidth / col);
        fd.width = usercg.readEntry("Width", fd.defaultWidth);
        sumWidth += fd.width;
        fd.visible = systemcg.readEntry("Visible", true);
        fd.visible = usercg.readEntry("Visible", fd.visible);
        append(fd);
    }
}

void BibTeXFields::save()
{
    int col = 1;
    for (Iterator it = begin(); it != end(); ++it, ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(m_userConfig, groupName);
        FieldDescription &fd = *it;
        usercg.writeEntry("Width", fd.width);
        usercg.writeEntry("Visible", fd.visible);
    }

    m_userConfig->sync();
}

void BibTeXFields::resetToDefaults()
{
    for (int col = 1; col < maxColumns; ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(m_userConfig, groupName);
        usercg.deleteGroup();
    }

    load();
}

