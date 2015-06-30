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

#include "bibtexfields.h"

#include <QExplicitlySharedDataPointer>
#include <QDebug>
#include <QStandardPaths>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

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

    KSharedConfigPtr layoutConfig;

    static BibTeXFields *singleton;

    BibTeXFieldsPrivate(BibTeXFields *parent)
            : p(parent) {
        KSharedConfigPtr config(KSharedConfig::openConfig("kbibtexrc"));
        KConfigGroup configGroup(config, QString("User Interface"));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex").append(".kbstyle");
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::AppDataLocation);
    }

    void load() {
        p->clear();

        QString groupName = QLatin1String("Column");
        KConfigGroup configGroup(layoutConfig, groupName);
        int columnCount = configGroup.readEntry("count", bibTeXFieldsMaxColumnCount);
        const QStringList treeViewNames = QStringList() << QLatin1String("SearchResults") << QLatin1String("Main") << QLatin1String("MergeWidget") << QLatin1String("Zotero");

        for (int col = 1; col <= columnCount; ++col) {
            const QString groupName = QString("Column%1").arg(col);
            KConfigGroup configGroup(layoutConfig, groupName);
            if (!configGroup.exists()) break;

            FieldDescription *fd = new FieldDescription();

            fd->upperCamelCase = configGroup.readEntry("UpperCamelCase", QString());
            if (fd->upperCamelCase.isEmpty()) {
                delete fd;
                continue;
            }

            fd->upperCamelCaseAlt = configGroup.readEntry("UpperCamelCaseAlt", QString());

            fd->label = i18n(configGroup.readEntry("Label", fd->upperCamelCase).toUtf8().constData());

            fd->defaultWidth = configGroup.readEntry("DefaultWidth", 10);
            fd->defaultVisible = configGroup.readEntry("Visible", true);

            foreach(const QString &treeViewName, treeViewNames) {
                fd->width.insert(treeViewName, configGroup.readEntry("Width_" + treeViewName, fd->defaultWidth));
                fd->visible.insert(treeViewName, configGroup.readEntry("Visible_" + treeViewName,  fd->defaultVisible));
            }

            QString typeFlags = configGroup.readEntry("TypeFlags", "Source");
            fd->typeFlags = typeFlagsFromString(typeFlags);
            QString preferredTypeFlag = typeFlags.split(';').first();
            fd->preferredTypeFlag = typeFlagFromString(preferredTypeFlag);

            fd->typeIndependent = configGroup.readEntry("TypeIndependent", false);

            p->append(fd);
        }

        if (p->isEmpty()) qWarning() << "List of field descriptions is empty after load()";
    }

    void save() {
        if (p->isEmpty()) qWarning() << "List of field descriptions is empty before save()";

        QStringList treeViewNames;
        int columnCount = 0;
        foreach(const FieldDescription *fd, *p) {
            ++columnCount;
            QString groupName = QString("Column%1").arg(columnCount);
            KConfigGroup configGroup(layoutConfig, groupName);

            for (QMap<QString, int>::ConstIterator it = fd->width.constBegin(); it != fd->width.constEnd(); ++it) {
                configGroup.writeEntry("Width_" + it.key(), it.value());
                configGroup.writeEntry("Visible_" +  it.key(), it.value());
            }
            QString typeFlagsString = fd->typeFlags == fd->preferredTypeFlag ? QString() : QLatin1Char(';') + typeFlagsToString(fd->typeFlags);
            typeFlagsString.prepend(typeFlagToString(fd->preferredTypeFlag));
            configGroup.writeEntry("TypeFlags", typeFlagsString);
            configGroup.writeEntry("TypeIndependent", fd->typeIndependent);

            if (treeViewNames.isEmpty())
                treeViewNames.append(fd->width.keys());
        }

        QString groupName = QLatin1String("Column");
        KConfigGroup configGroup(layoutConfig, groupName);
        configGroup.writeEntry("count", columnCount);
        configGroup.writeEntry("treeViewNames", treeViewNames);

        layoutConfig->sync();
    }

    void resetToDefaults(const QString &treeViewName) {
        for (int col = 1; col < bibTeXFieldsMaxColumnCount; ++col) {
            QString groupName = QString("Column%1").arg(col);
            KConfigGroup configGroup(layoutConfig, groupName);
            configGroup.deleteEntry("Width_" + treeViewName);
            configGroup.deleteEntry("Visible_" + treeViewName);
        }
        load();
    }
};

BibTeXFields *BibTeXFields::BibTeXFieldsPrivate::singleton = NULL;

BibTeXFields::BibTeXFields()
        : QList<FieldDescription *>(), d(new BibTeXFieldsPrivate(this))
{
    d->load();
}

BibTeXFields::~BibTeXFields()
{
    delete d;
}

const BibTeXFields *BibTeXFields::self()
{
    if (BibTeXFieldsPrivate::singleton == NULL)
        BibTeXFieldsPrivate::singleton = new BibTeXFields();
    return BibTeXFieldsPrivate::singleton;
}

void BibTeXFields::save()
{
    d->save();
}

void BibTeXFields::resetToDefaults(const QString &treeViewName)
{
    d->resetToDefaults(treeViewName);
}

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
        for (QList<FieldDescription *>::ConstIterator it = constBegin(); it != constEnd(); ++it) {
            /// configuration file uses camel-case
            QString itName = (*it)->upperCamelCase.toLower();
            if (itName == iName && (*it)->upperCamelCaseAlt.isEmpty()) {
                iName = (*it)->upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toLower();
        return iName;
    }
    case KBibTeX::cUpperCamelCase: {
        for (QList<FieldDescription *>::ConstIterator it = constBegin(); it != constEnd(); ++it) {
            /// configuration file uses camel-case
            QString itName = (*it)->upperCamelCase.toLower();
            if (itName == iName && (*it)->upperCamelCaseAlt.isEmpty()) {
                iName = (*it)->upperCamelCase;
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

const FieldDescription *BibTeXFields::find(const QString &name) const
{
    const QString iName = name.toLower();
    for (QList<FieldDescription *>::ConstIterator it = constBegin(); it != constEnd(); ++it) {
        if ((*it)->upperCamelCase.toLower() == iName && (*it)->upperCamelCaseAlt.isEmpty())
            return (*it);
    }
    qWarning() << "No field description for " << name << "(" << iName << ")";
    return NULL;
}

KBibTeX::TypeFlag BibTeXFields::typeFlagFromString(const QString &typeFlagString)
{
    KBibTeX::TypeFlag result = (KBibTeX::TypeFlag)0;

    if (typeFlagString == QLatin1String("Text"))
        result = KBibTeX::tfPlainText;
    else if (typeFlagString == QLatin1String("Source"))
        result = KBibTeX::tfSource;
    else if (typeFlagString == QLatin1String("Person"))
        result = KBibTeX::tfPerson;
    else if (typeFlagString == QLatin1String("Keyword"))
        result = KBibTeX::tfKeyword;
    else if (typeFlagString == QLatin1String("Reference"))
        result = KBibTeX::tfReference;
    else if (typeFlagString == QLatin1String("Verbatim"))
        result = KBibTeX::tfVerbatim;

    return result;
}

KBibTeX::TypeFlags BibTeXFields::typeFlagsFromString(const QString &typeFlagsString)
{
    KBibTeX::TypeFlags result;

    QStringList list = typeFlagsString.split(';');
    for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
        result |= typeFlagFromString(*it);

    return result;
}

QString BibTeXFields::typeFlagsToString(KBibTeX::TypeFlags typeFlags)
{
    QStringList resultList;
    if (typeFlags & KBibTeX::tfPlainText) resultList << QLatin1String("Text");
    if (typeFlags & KBibTeX::tfSource) resultList << QLatin1String("Source");
    if (typeFlags & KBibTeX::tfPerson) resultList << QLatin1String("Person");
    if (typeFlags & KBibTeX::tfKeyword) resultList << QLatin1String("Keyword");
    if (typeFlags & KBibTeX::tfReference) resultList << QLatin1String("Reference");
    if (typeFlags & KBibTeX::tfVerbatim) resultList << QLatin1String("Verbatim");
    return resultList.join(QChar(';'));
}

QString BibTeXFields::typeFlagToString(KBibTeX::TypeFlag typeFlag)
{
    if (typeFlag == KBibTeX::tfPlainText) return QLatin1String("Text");
    if (typeFlag == KBibTeX::tfSource) return QLatin1String("Source");
    if (typeFlag == KBibTeX::tfPerson) return QLatin1String("Person");
    if (typeFlag == KBibTeX::tfKeyword) return QLatin1String("Keyword");
    if (typeFlag == KBibTeX::tfReference) return QLatin1String("Reference");
    if (typeFlag == KBibTeX::tfVerbatim) return QLatin1String("Verbatim");
    return QString();
}
