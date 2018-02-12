/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "bibtexfields.h"

#include <QExplicitlySharedDataPointer>
#include <QStandardPaths>

#ifdef HAVE_KF5
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#endif // HAVE KF5

#include "logging_config.h"

bool operator==(const FieldDescription &a, const FieldDescription &b)
{
    return a.upperCamelCase == b.upperCamelCase;
}
uint qHash(const FieldDescription &a)
{
    return qHash(a.upperCamelCase);
}

static const int bibTeXFieldsMaxColumnCount = 0x0fff;

class BibTeXFields::BibTeXFieldsPrivate
{
public:
    BibTeXFields *p;

#ifdef HAVE_KF5
    KSharedConfigPtr layoutConfig;
#endif // HAVE_KF5

    static BibTeXFields *singleton;

    BibTeXFieldsPrivate(BibTeXFields *parent)
            : p(parent) {
#ifdef HAVE_KF5
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, QStringLiteral("User Interface"));
        const QString stylefile = configGroup.readEntry(QStringLiteral("CurrentStyle"), QString(QStringLiteral("bibtex"))).append(QStringLiteral(".kbstyle"));
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::AppDataLocation);
        if (layoutConfig->groupList().isEmpty())
            qCWarning(LOG_KBIBTEX_CONFIG) << "The configuration file for BibTeX fields could not be located or is empty:" << stylefile;
#endif // HAVE_KF5
    }

#ifdef HAVE_KF5
    void load() {
        p->clear();

        const QString groupName = QStringLiteral("Column");
        KConfigGroup configGroup(layoutConfig, groupName);
        int columnCount = configGroup.readEntry(QStringLiteral("count"), bibTeXFieldsMaxColumnCount);
        static const QStringList treeViewNames = QStringList() << QStringLiteral("SearchResults") << QStringLiteral("Main") << QStringLiteral("MergeWidget") << QStringLiteral("Zotero");

        if (columnCount == 0)
            qCWarning(LOG_KBIBTEX_CONFIG) << "The configuration file for BibTeX fields does not contain columns:" << layoutConfig->name();

        for (int col = 1; col <= columnCount; ++col) {
            const QString groupName = QString(QStringLiteral("Column%1")).arg(col);
            KConfigGroup configGroup(layoutConfig, groupName);
            if (!configGroup.exists()) break;

            FieldDescription fd;

            fd.upperCamelCase = configGroup.readEntry("UpperCamelCase", QString());
            if (fd.upperCamelCase.isEmpty())
                continue;

            fd.upperCamelCaseAlt = configGroup.readEntry("UpperCamelCaseAlt", QString());

            fd.label = i18n(configGroup.readEntry("Label", fd.upperCamelCase).toUtf8().constData());

            fd.defaultWidth = configGroup.readEntry("DefaultWidth", 10);
            fd.defaultVisible = configGroup.readEntry("Visible", true);

            for (const QString &treeViewName : treeViewNames)
                fd.visible.insert(treeViewName, configGroup.readEntry("Visible_" + treeViewName,  fd.defaultVisible));

            QString typeFlags = configGroup.readEntry("TypeFlags", "Source");
            fd.typeFlags = typeFlagsFromString(typeFlags);
            QString preferredTypeFlag = typeFlags.split(';').first();
            fd.preferredTypeFlag = typeFlagFromString(preferredTypeFlag);

            fd.typeIndependent = configGroup.readEntry("TypeIndependent", false);

            p->append(fd);
        }

        if (p->isEmpty()) qCWarning(LOG_KBIBTEX_CONFIG) << "List of field descriptions is empty after load()";
    }

    void save() {
        if (p->isEmpty()) qCWarning(LOG_KBIBTEX_CONFIG) << "List of field descriptions is empty before save()";

        QStringList treeViewNames;
        int columnCount = 0;
        for (const auto &fd : const_cast<const BibTeXFields &>(*p)) {
            ++columnCount;
            QString groupName = QString(QStringLiteral("Column%1")).arg(columnCount);
            KConfigGroup configGroup(layoutConfig, groupName);

            const QList<QString> keys = fd.visible.keys();
            for (const QString &treeViewName : keys) {
                const QString key = QStringLiteral("Visible_") + treeViewName;
                configGroup.writeEntry(key, fd.visible.value(treeViewName, fd.defaultVisible));
            }
            QString typeFlagsString = fd.typeFlags == fd.preferredTypeFlag ? QString() : QLatin1Char(';') + typeFlagsToString(fd.typeFlags);
            typeFlagsString.prepend(typeFlagToString(fd.preferredTypeFlag));
            configGroup.writeEntry("TypeFlags", typeFlagsString);
            configGroup.writeEntry("TypeIndependent", fd.typeIndependent);

            if (treeViewNames.isEmpty())
                treeViewNames.append(fd.visible.keys());
        }

        QString groupName = QStringLiteral("Column");
        KConfigGroup configGroup(layoutConfig, groupName);
        configGroup.writeEntry("count", columnCount);
        configGroup.writeEntry("treeViewNames", treeViewNames);

        layoutConfig->sync();
    }

    void resetToDefaults(const QString &treeViewName) {
        for (int col = 1; col < bibTeXFieldsMaxColumnCount; ++col) {
            QString groupName = QString(QStringLiteral("Column%1")).arg(col);
            KConfigGroup configGroup(layoutConfig, groupName);
            configGroup.deleteEntry("Width_" + treeViewName);
            configGroup.deleteEntry("Visible_" + treeViewName);
        }
        load();
    }
#endif // HAVE_KF5
};

BibTeXFields *BibTeXFields::BibTeXFieldsPrivate::singleton = nullptr;

BibTeXFields::BibTeXFields()
        : QVector<FieldDescription>(), d(new BibTeXFieldsPrivate(this))
{
#ifdef HAVE_KF5
    d->load();
#endif // HAVE_KF5
}

BibTeXFields::~BibTeXFields()
{
    delete d;
}

BibTeXFields *BibTeXFields::self()
{
    if (BibTeXFieldsPrivate::singleton == nullptr)
        BibTeXFieldsPrivate::singleton = new BibTeXFields();
    return BibTeXFieldsPrivate::singleton;
}

#ifdef HAVE_KF5
void BibTeXFields::save()
{
    d->save();
}

void BibTeXFields::resetToDefaults(const QString &treeViewName)
{
    d->resetToDefaults(treeViewName);
}
#endif // HAVE_KF5

QString BibTeXFields::format(const QString &name, KBibTeX::Casing casing) const
{
    QString iName = name.toLower();

    switch (casing) {
    case KBibTeX::cLowerCase: return iName;
    case KBibTeX::cUpperCase: return name.toUpper();
    case KBibTeX::cInitialCapital:
        iName[0] = iName[0].toUpper();
        return iName;
    case KBibTeX::cLowerCamelCase: {
        for (const auto &fd : const_cast<const BibTeXFields &>(*this)) {
            /// configuration file uses camel-case
            QString itName = fd.upperCamelCase.toLower();
            if (itName == iName && fd.upperCamelCaseAlt.isEmpty()) {
                iName = fd.upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toLower();
        return iName;
    }
    case KBibTeX::cUpperCamelCase: {
        for (const auto &fd : const_cast<const BibTeXFields &>(*this)) {
            /// configuration file uses camel-case
            QString itName = fd.upperCamelCase.toLower();
            if (itName == iName && fd.upperCamelCaseAlt.isEmpty()) {
                iName = fd.upperCamelCase;
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

const FieldDescription BibTeXFields::find(const QString &name) const
{
    const QString iName = name.toLower();
    for (const auto &fd : const_cast<const BibTeXFields &>(*this)) {
        if (fd.upperCamelCase.toLower() == iName && fd.upperCamelCaseAlt.isEmpty())
            return fd;
    }
    qCWarning(LOG_KBIBTEX_CONFIG) << "No field description for " << name << "(" << iName << ")";
    return FieldDescription {QString(), QString(), QString(), KBibTeX::tfSource, KBibTeX::tfSource, 0, {}, false, false};
}

KBibTeX::TypeFlag BibTeXFields::typeFlagFromString(const QString &typeFlagString)
{
    KBibTeX::TypeFlag result = (KBibTeX::TypeFlag)0;

    if (typeFlagString == QStringLiteral("Text"))
        result = KBibTeX::tfPlainText;
    else if (typeFlagString == QStringLiteral("Source"))
        result = KBibTeX::tfSource;
    else if (typeFlagString == QStringLiteral("Person"))
        result = KBibTeX::tfPerson;
    else if (typeFlagString == QStringLiteral("Keyword"))
        result = KBibTeX::tfKeyword;
    else if (typeFlagString == QStringLiteral("Reference"))
        result = KBibTeX::tfReference;
    else if (typeFlagString == QStringLiteral("Verbatim"))
        result = KBibTeX::tfVerbatim;

    return result;
}

KBibTeX::TypeFlags BibTeXFields::typeFlagsFromString(const QString &typeFlagsString)
{
    KBibTeX::TypeFlags result;

    const QStringList list = typeFlagsString.split(';');
    for (const QString &s : list)
        result |= typeFlagFromString(s);

    return result;
}

QString BibTeXFields::typeFlagsToString(KBibTeX::TypeFlags typeFlags)
{
    QStringList resultList;
    if (typeFlags & KBibTeX::tfPlainText) resultList << QStringLiteral("Text");
    if (typeFlags & KBibTeX::tfSource) resultList << QStringLiteral("Source");
    if (typeFlags & KBibTeX::tfPerson) resultList << QStringLiteral("Person");
    if (typeFlags & KBibTeX::tfKeyword) resultList << QStringLiteral("Keyword");
    if (typeFlags & KBibTeX::tfReference) resultList << QStringLiteral("Reference");
    if (typeFlags & KBibTeX::tfVerbatim) resultList << QStringLiteral("Verbatim");
    return resultList.join(QChar(';'));
}

QString BibTeXFields::typeFlagToString(KBibTeX::TypeFlag typeFlag)
{
    if (typeFlag == KBibTeX::tfPlainText) return QStringLiteral("Text");
    if (typeFlag == KBibTeX::tfSource) return QStringLiteral("Source");
    if (typeFlag == KBibTeX::tfPerson) return QStringLiteral("Person");
    if (typeFlag == KBibTeX::tfKeyword) return QStringLiteral("Keyword");
    if (typeFlag == KBibTeX::tfReference) return QStringLiteral("Reference");
    if (typeFlag == KBibTeX::tfVerbatim) return QStringLiteral("Verbatim");
    return QString();
}
