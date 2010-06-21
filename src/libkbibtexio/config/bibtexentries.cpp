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

#include <KGlobal>
#include <KStandardDirs>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KSharedPtr>

#include <entry.h>
#include "bibtexentries.h"

class BibTeXEntries::BibTeXEntriesPrivate
{
public:
    BibTeXEntries *p;

    KConfig *systemDefaultsConfig;
    KSharedPtr<KSharedConfig> userConfig;

    static BibTeXEntries *singleton;

    BibTeXEntriesPrivate(BibTeXEntries *parent)
            : p(parent) {
        systemDefaultsConfig = new KConfig(KStandardDirs::locate("appdata", "entrytypes.rc"), KConfig::SimpleConfig);
        userConfig = KSharedConfig::openConfig(KStandardDirs::locateLocal("appdata", "ui.rc"), KConfig::SimpleConfig);
    }

    ~BibTeXEntriesPrivate() {
        delete systemDefaultsConfig;
    }
};

BibTeXEntries *BibTeXEntries::BibTeXEntriesPrivate::singleton = NULL;


BibTeXEntries::BibTeXEntries()
        : d(new BibTeXEntriesPrivate(this))
{
    load();
}

BibTeXEntries::~BibTeXEntries()
{
    delete d;
}

BibTeXEntries* BibTeXEntries::self()
{
    if (BibTeXEntriesPrivate::singleton == NULL)
        BibTeXEntriesPrivate::singleton = new BibTeXEntries();
    return BibTeXEntriesPrivate::singleton;
}


void BibTeXEntries::load()
{
// TODO
    clear();

    // TODO: Dummy implementation
    EntryDescription ed;
    ed.label = "Article";
    ed.raw = "Article";
    append(ed);
}

QString BibTeXEntries::format(const QString& name, KBibTeX::Casing casing) const
{
    QString iName = name.toLower();

    switch (casing) {
    case KBibTeX::cLowerCase: return iName;
    case KBibTeX::cUpperCase: return name.toUpper();
    case KBibTeX::cInitialCapital:
        iName[0] = iName[0].toUpper();
        return iName;
    case KBibTeX::cCamelCase: {
        for (ConstIterator it = begin(); it != end(); ++it) {
            /// configuration file uses camel-case
            QString itName = (*it).raw.toLower();
            if (itName == iName)
                return (*it).raw;

            /// make an educated guess how camel-case would look like
            iName[0] = iName[0].toUpper();
            return iName;
        }
    }
    }
    return name;
}

void BibTeXEntries::format(File& file, KBibTeX::Casing casing) const
{
    for (File::Iterator it = file.begin(); it != file.end(); ++it) {
        Entry *entry = dynamic_cast<Entry*>(*it);
        if (entry != NULL) {
            QString key = format(entry->id(), casing);
            entry->setId(key);
        }
    }
}
