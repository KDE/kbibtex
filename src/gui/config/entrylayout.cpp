/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#include <KSharedConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KStandardDirs>
#include <KDebug>

#include "entrylayout.h"

static const int entryLayoutMaxTabCount = 256;
static const int entryLayoutMaxFieldPerTabCount = 256;

class EntryLayout::EntryLayoutPrivate
{
public:
    EntryLayout *p;

    KConfig *systemDefaultsConfig;
    KSharedConfigPtr userConfig;

    static EntryLayout *singleton;

    EntryLayoutPrivate(EntryLayout *parent)
            : p(parent) {
        kDebug() << "looking for " << KStandardDirs::locate("appdata", "entrylayout.rc");
        systemDefaultsConfig = new KConfig(KStandardDirs::locate("appdata", "entrylayout.rc"), KConfig::SimpleConfig);
        kDebug() << "looking for " << KStandardDirs::locateLocal("appdata", "entrylayout.rc");
        userConfig = KSharedConfig::openConfig(KStandardDirs::locateLocal("appdata", "entrylayout.rc"), KConfig::SimpleConfig);
    }

    static QString convert(KBibTeX::FieldInputType fil) {
        switch (fil) {
        case KBibTeX::List : return QLatin1String("List");
        case KBibTeX::MultiLine : return QLatin1String("MultiLine");
        case KBibTeX::URL : return QLatin1String("URL");
        case KBibTeX::Month : return QLatin1String("Month");
        default: return QLatin1String("SingleLine");
        }
    }

    static KBibTeX::FieldInputType convert(const QString &text) {
        if (text == QLatin1String("List"))
            return KBibTeX::List;
        else if (text == QLatin1String("MultiLine"))
            return KBibTeX::MultiLine;
        else if (text == QLatin1String("URL"))
            return KBibTeX::URL;
        else if (text == QLatin1String("Month"))
            return KBibTeX::Month;
        else
            return KBibTeX::SingleLine;
    }

    ~EntryLayoutPrivate() {
        delete systemDefaultsConfig;
    }
};

EntryLayout *EntryLayout::EntryLayoutPrivate::singleton = NULL;

EntryLayout::EntryLayout()
        : d(new EntryLayoutPrivate(this))
{
    load();
}

EntryLayout::~EntryLayout()
{
    delete d;
}

EntryLayout* EntryLayout::self()
{
    if (EntryLayoutPrivate::singleton == NULL)
        EntryLayoutPrivate::singleton  = new EntryLayout();
    return EntryLayoutPrivate::singleton;
}

void EntryLayout::load()
{
    clear();
    for (int tab = 1; tab < entryLayoutMaxTabCount; ++tab) {
        QString groupName = QString("Tab%1").arg(tab);
        KConfigGroup usercg(d->userConfig, groupName);
        KConfigGroup systemcg(d->systemDefaultsConfig, groupName);

        EntryTabLayout etl;
        etl.uiCaption = systemcg.readEntry("uiCaption", "");
        etl.columns = systemcg.readEntry("columns", 1);
        if (etl.uiCaption.isEmpty())
            continue;

        for (int field = 1; field < entryLayoutMaxFieldPerTabCount; ++field) {
            SingleFieldLayout sfl;
            sfl.bibtexLabel = systemcg.readEntry(QString("bibtexLabel%1").arg(field), "");
            sfl.uiLabel = systemcg.readEntry(QString("uiLabel%1").arg(field), "");
            sfl.fieldInputLayout = EntryLayoutPrivate::convert(systemcg.readEntry(QString("fieldInputLayout%1").arg(field), "SingleLine"));
            if (sfl.bibtexLabel.isEmpty() || sfl.uiLabel.isEmpty())
                continue;

            etl.singleFieldLayouts.append(sfl);
        }
        append(etl);
    }
}

void EntryLayout::save()
{
    /*
    int col = 1;
    for (Iterator it = begin(); it != end(); ++it, ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(d->userConfig, groupName);
        FieldDescription &fd = *it;
        usercg.writeEntry("Width", fd.width);
        usercg.writeEntry("Visible", fd.visible);
    }

    d->userConfig->sync();
    */
}

void EntryLayout::resetToDefaults()
{
    /*
    for (int col = 1; col < bibTeXFieldsMaxColumnCount; ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(d->userConfig, groupName);
        usercg.deleteGroup();
    }
    */

    load();
}
