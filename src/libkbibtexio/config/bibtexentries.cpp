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

#include <KSharedConfig>
#include <KConfigGroup>
#include <KDebug>

#include <entry.h>
#include "bibtexentries.h"

bool operator==(const EntryDescription &a, const EntryDescription &b)
{
    return a.upperCamelCase == b.upperCamelCase;
}
uint qHash(const EntryDescription &a)
{
    return qHash(a.upperCamelCase);
}

static const int entryTypeMaxCount = 256;

class BibTeXEntries::BibTeXEntriesPrivate
{
public:
    BibTeXEntries *p;

    KSharedConfigPtr config;

    static BibTeXEntries *singleton;

    BibTeXEntriesPrivate(BibTeXEntries *parent)
            : p(parent), config(KSharedConfig::openConfig("kbibtexrc")) {
        // nothing
    }

    void load() {
        p->clear();

        EntryDescription ed;

        QString groupName = QLatin1String("EntryType");
        KConfigGroup configGroup(config, groupName);
        int typeCount = qMin(configGroup.readEntry("count", 0), entryTypeMaxCount);

        for (int col = 1; col <= typeCount; ++col) {
            QString groupName = QString("EntryType%1").arg(col);
            KConfigGroup configGroup(config, groupName);

            ed.upperCamelCase = configGroup.readEntry("UpperCamelCase", "");
            if (ed.upperCamelCase.isEmpty()) continue;
            ed.upperCamelCaseAlt = configGroup.readEntry("UpperCamelCaseAlt", "");
            ed.label = configGroup.readEntry("Label", ed.upperCamelCase);;
            p->append(ed);
        }

        if (p->isEmpty()) kWarning() << "List of entry descriptions is empty";
    }

    void save() {
        int typeCount = 0;
        foreach(EntryDescription ed, *p) {
            ++typeCount;
            QString groupName = QString("EntryType%1").arg(typeCount);
            KConfigGroup configGroup(config, groupName);

            configGroup.writeEntry("UpperCamelCase", ed.upperCamelCase);
            configGroup.writeEntry("UpperCamelCaseAlt", ed.upperCamelCaseAlt);
            configGroup.writeEntry("Label", ed.label);
        }

        QString groupName = QLatin1String("EntryType");
        KConfigGroup configGroup(config, groupName);
        configGroup.writeEntry("count", typeCount);

        config->sync();
    }

};

BibTeXEntries *BibTeXEntries::BibTeXEntriesPrivate::singleton = NULL;


BibTeXEntries::BibTeXEntries()
        : QList<EntryDescription>(), d(new BibTeXEntriesPrivate(this))
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
            if (itName == iName && !(*it).upperCamelCase.isEmpty()) {
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
            if (itName == iName && !(*it).upperCamelCase.isEmpty()) {
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
