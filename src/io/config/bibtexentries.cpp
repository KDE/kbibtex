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

#include "bibtexentries.h"

#include <QDebug>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include "entry.h"

bool operator==(const EntryDescription &a, const EntryDescription &b)
{
    return a.upperCamelCase == b.upperCamelCase;
}
uint qHash(const EntryDescription &a)
{
    return qHash(a.upperCamelCase);
}

static const int entryTypeMaxCount = 0x0fff;

class BibTeXEntries::BibTeXEntriesPrivate
{
public:
    BibTeXEntries *p;

    KSharedConfigPtr layoutConfig;

    static BibTeXEntries *singleton;

    BibTeXEntriesPrivate(BibTeXEntries *parent)
            : p(parent) {
        KSharedConfigPtr config(KSharedConfig::openConfig("kbibtexrc"));
        KConfigGroup configGroup(config, QString("User Interface"));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex").append(".kbstyle").prepend("kbibtex/");
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::AppDataLocation);
    }

    void load() {
        p->clear();

        EntryDescription ed;

        /// Field "count" is deprecated, invalid values are a common origin of bugs
        // QString groupName = QLatin1String("EntryType");
        // KConfigGroup configGroup(layoutConfig, groupName);
        // int typeCount = configGroup.readEntry("count", entryTypeMaxCount);

        for (int col = 1; col <= entryTypeMaxCount; ++col) {
            const QString groupName = QString("EntryType%1").arg(col);
            KConfigGroup configGroup(layoutConfig, groupName);
            if (!configGroup.exists()) break;

            ed.upperCamelCase = configGroup.readEntry("UpperCamelCase", QString());
            if (ed.upperCamelCase.isEmpty()) continue;
            ed.upperCamelCaseAlt = configGroup.readEntry("UpperCamelCaseAlt", QString());
            ed.label = i18n(configGroup.readEntry("Label", ed.upperCamelCase).toUtf8().constData());
            ed.requiredItems = configGroup.readEntry("RequiredItems", QStringList());
            ed.optionalItems = configGroup.readEntry("OptionalItems", QStringList());
            p->append(ed);
        }

        if (p->isEmpty()) qWarning() << "List of entry descriptions is empty";
    }

    void save() {
        int typeCount = 0;
        foreach(const EntryDescription &ed, *p) {
            ++typeCount;
            QString groupName = QString("EntryType%1").arg(typeCount);
            KConfigGroup configGroup(layoutConfig, groupName);

            configGroup.writeEntry("UpperCamelCase", ed.upperCamelCase);
            configGroup.writeEntry("UpperCamelCaseAlt", ed.upperCamelCaseAlt);
            configGroup.writeEntry("Label", ed.label);
            configGroup.writeEntry("RequiredItems", ed.requiredItems);
            configGroup.writeEntry("OptionalItems", ed.optionalItems);
        }

        /// Although field "count" is deprecated, it is written for backwards compatibility
        QString groupName = QLatin1String("EntryType");
        KConfigGroup configGroup(layoutConfig, groupName);
        configGroup.writeEntry("count", typeCount);

        layoutConfig->sync();
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

BibTeXEntries *BibTeXEntries::self()
{
    if (BibTeXEntriesPrivate::singleton == NULL)
        BibTeXEntriesPrivate::singleton = new BibTeXEntries();
    return BibTeXEntriesPrivate::singleton;
}

QString BibTeXEntries::format(const QString &name, KBibTeX::Casing casing) const
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
            if (itName == iName) {
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
            if (itName == iName) {
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

QString BibTeXEntries::label(const QString &name) const
{
    const QString iName = name.toLower();

    for (ConstIterator it = begin(); it != end(); ++it) {
        /// Configuration file uses camel-case, convert this to lower case for faster comparison
        QString itName = (*it).upperCamelCase.toLower();
        if (itName == iName || (!(itName = (*it).upperCamelCaseAlt.toLower()).isEmpty() && itName == iName))
            return (*it).label;
    }
    return QString();
}
