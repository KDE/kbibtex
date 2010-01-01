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
#include <KGlobal>
#include <KStandardDirs>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KSharedPtr>

#include "bibtexfields.h"

using namespace KBibTeX::GUI::Config;

class BibTeXFields::BibTeXFieldsPrivate
{
public:
    BibTeXFields *p;

    KConfig *systemDefaultsConfig;
    KSharedPtr<KSharedConfig> userConfig;

    static BibTeXFields *singleton;
    const int maxColumns;

    BibTeXFieldsPrivate(BibTeXFields *parent)
            : p(parent), maxColumns(256) {
        systemDefaultsConfig = new KConfig(KStandardDirs::locate("appdata", "fieldtypes.rc"), KConfig::SimpleConfig);
        userConfig = KSharedConfig::openConfig(KStandardDirs::locateLocal("appdata", "ui.rc"), KConfig::SimpleConfig);
    }

    ~BibTeXFieldsPrivate() {
        delete systemDefaultsConfig;
    }
};

BibTeXFields *BibTeXFields::BibTeXFieldsPrivate::singleton = NULL;

BibTeXFields::BibTeXFields()
        : d(new BibTeXFieldsPrivate(this))
{
    load();
}

BibTeXFields::~BibTeXFields()
{
    delete d;
}

BibTeXFields* BibTeXFields::self()
{
    if (BibTeXFieldsPrivate::singleton == NULL)
        BibTeXFieldsPrivate::singleton  = new BibTeXFields();
    return BibTeXFieldsPrivate::singleton;
}

void BibTeXFields::load()
{
    unsigned int sumWidth = 0;
    FieldDescription fd;

    clear();
    for (int col = 1; col < d->maxColumns; ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(d->userConfig, groupName);
        KConfigGroup systemcg(d->systemDefaultsConfig, groupName);
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
        KConfigGroup usercg(d->userConfig, groupName);
        FieldDescription &fd = *it;
        usercg.writeEntry("Width", fd.width);
        usercg.writeEntry("Visible", fd.visible);
    }

    d->userConfig->sync();
}

void BibTeXFields::resetToDefaults()
{
    for (int col = 1; col < d->maxColumns; ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(d->userConfig, groupName);
        usercg.deleteGroup();
    }

    load();
}

