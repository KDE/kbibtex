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
#include <KDebug>

#include <entry.h>
#include "bibtexentries.h"

static const int BibTeXEntriesMax = 256;

class BibTeXEntries::BibTeXEntriesPrivate
{
public:
    BibTeXEntries *p;

    KConfig *systemDefaultsConfig;
    KSharedConfigPtr userConfig;

    static BibTeXEntries *singleton;

    BibTeXEntriesPrivate(BibTeXEntries *parent)
            : p(parent) {
        systemDefaultsConfig = new KConfig(KStandardDirs::locate("appdata", "entrytypes.rc"), KConfig::SimpleConfig);
        userConfig = KSharedConfig::openConfig(KStandardDirs::locateLocal("appdata", "entrytypes.rc"), KConfig::SimpleConfig);
    }

    ~BibTeXEntriesPrivate() {
        delete systemDefaultsConfig;
    }

    void load() {
// TODO
        p->clear();

        // TODO: Dummy implementation
        EntryDescription ed;

        for (int col = 1; col < BibTeXEntriesMax; ++col) {
            QString groupName = QString("EntryType%1").arg(col);
            KConfigGroup usercg(userConfig, groupName);
            KConfigGroup systemcg(systemDefaultsConfig, groupName);

            ed.upperCamelCase = systemcg.readEntry("UpperCamelCase", "");
            ed.upperCamelCase = usercg.readEntry("UpperCamelCase", ed.upperCamelCase);
            if (ed.upperCamelCase.isEmpty()) continue;
            ed.upperCamelCaseAlt = systemcg.readEntry("UpperCamelCaseAlt", "");
            ed.upperCamelCaseAlt = usercg.readEntry("UpperCamelCaseAlt", ed.upperCamelCaseAlt);
            ed.label = systemcg.readEntry("Label", ed.upperCamelCase);;
            ed.label = usercg.readEntry("Label", ed.label);;
            p->append(ed);
        }
    }
};

BibTeXEntries *BibTeXEntries::BibTeXEntriesPrivate::singleton = NULL;


BibTeXEntries::BibTeXEntries()
        : d(new BibTeXEntriesPrivate(this))
{
    d->load();
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

QString BibTeXEntries::format(const QString& name, KBibTeX::Casing casing) const
{
    QString iName = name.toLower();

    switch (casing) {
    case KBibTeX::cLowerCase: return iName;
    case KBibTeX::cUpperCase: return name.toUpper();
    case KBibTeX::cInitialCapital:
        iName[0] = iName[0].toUpper();
        return iName;
    case KBibTeX::cLowerCamelCase: {
        for (ConstIterator it = begin(); it != end(); ++it) {
            /// configuration file uses camel-case
            QString itName = (*it).upperCamelCase.toLower();
            if (itName == iName && (*it).upperCamelCase == QString::null) {
                iName = (*it).upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toLower();
        return iName;
    }
    case KBibTeX::cUpperCamelCase: {
        for (ConstIterator it = begin(); it != end(); ++it) {
            /// configuration file uses camel-case
            QString itName = (*it).upperCamelCase.toLower();
            if (itName == iName && (*it).upperCamelCase == QString::null) {
                iName = (*it).upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toUpper();
        return iName;
    }
    }
    return name;
}

QString BibTeXEntries::label(const QString& name) const
{
    const QString iName = name.toLower();

    for (ConstIterator it = begin(); it != end(); ++it) {
        /// configuration file uses camel-case
        QString itName = (*it).upperCamelCase.toLower();
        if (itName == iName || (!(itName = (*it).upperCamelCaseAlt.toLower()).isEmpty() && itName == iName))
            return (*it).label;
    }
    return QString::null;
}
