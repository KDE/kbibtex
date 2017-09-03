/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "bibtexentries.h"

#include <QStandardPaths>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#endif // HAVE_KF5

#include "logging_config.h"

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

#ifdef HAVE_KF5
    KSharedConfigPtr layoutConfig;
#endif // HAVE_KF5

    static BibTeXEntries *singleton;

    BibTeXEntriesPrivate(BibTeXEntries *parent)
            : p(parent) {
#ifdef HAVE_KF5
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, QStringLiteral("User Interface"));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex").append(".kbstyle");
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::AppDataLocation);
        if (layoutConfig->groupList().isEmpty())
            qCWarning(LOG_KBIBTEX_CONFIG) << "The configuration file for BibTeX fields could not be located or is empty:" << stylefile;
#endif // HAVE_KF5
    }

#ifdef HAVE_KF5
    void load() {
        p->clear();

        EntryDescription ed;

        /// Field "count" is deprecated, invalid values are a common origin of bugs
        // QString groupName = QStringLiteral("EntryType");
        // KConfigGroup configGroup(layoutConfig, groupName);
        // int typeCount = configGroup.readEntry("count", entryTypeMaxCount);

        for (int col = 1; col <= entryTypeMaxCount; ++col) {
            const QString groupName = QString(QStringLiteral("EntryType%1")).arg(col);
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

        if (p->isEmpty()) qCWarning(LOG_KBIBTEX_CONFIG) << "List of entry descriptions is empty";
    }

    void save() {
        int typeCount = 0;
        for (const EntryDescription &ed : const_cast<const BibTeXEntries &>(*p)) {
            ++typeCount;
            QString groupName = QString(QStringLiteral("EntryType%1")).arg(typeCount);
            KConfigGroup configGroup(layoutConfig, groupName);

            configGroup.writeEntry("UpperCamelCase", ed.upperCamelCase);
            configGroup.writeEntry("UpperCamelCaseAlt", ed.upperCamelCaseAlt);
            configGroup.writeEntry("Label", ed.label);
            configGroup.writeEntry("RequiredItems", ed.requiredItems);
            configGroup.writeEntry("OptionalItems", ed.optionalItems);
        }

        /// Although field "count" is deprecated, it is written for backwards compatibility
        QString groupName = QStringLiteral("EntryType");
        KConfigGroup configGroup(layoutConfig, groupName);
        configGroup.writeEntry("count", typeCount);

        layoutConfig->sync();
    }
#endif // HAVE_KF5
};

BibTeXEntries *BibTeXEntries::BibTeXEntriesPrivate::singleton = nullptr;


BibTeXEntries::BibTeXEntries()
        : QList<EntryDescription>(), d(new BibTeXEntriesPrivate(this))
{
#ifdef HAVE_KF5
    d->load();
#endif // HAVE_KF5
}

BibTeXEntries::~BibTeXEntries()
{
    delete d;
}

const BibTeXEntries *BibTeXEntries::self()
{
    if (BibTeXEntriesPrivate::singleton == nullptr)
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
        for (const auto &ed : const_cast<const BibTeXEntries &>(*this)) {
            /// configuration file uses camel-case
            QString itName = ed.upperCamelCase.toLower();
            if (itName == iName) {
                iName = ed.upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toLower();
        return iName;
    }
    case KBibTeX::cUpperCamelCase: {
        for (const auto &ed : const_cast<const BibTeXEntries &>(*this)) {
            /// configuration file uses camel-case
            QString itName = ed.upperCamelCase.toLower();
            if (itName == iName) {
                iName = ed.upperCamelCase;
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

    for (const auto &ed : const_cast<const BibTeXEntries &>(*this)) {
        /// Configuration file uses camel-case, convert this to lower case for faster comparison
        QString itName = ed.upperCamelCase.toLower();
        if (itName == iName || (!(itName = ed.upperCamelCaseAlt.toLower()).isEmpty() && itName == iName))
            return ed.label;
    }
    return QString();
}
