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

#include "bibtexfields.h"

static const int bibTeXFieldsMaxColumnCount = 256;

class BibTeXFields::BibTeXFieldsPrivate
{
public:
    BibTeXFields *p;

    KConfig *systemDefaultsConfig;
    KSharedConfigPtr userConfig;

    static BibTeXFields *singleton;

    BibTeXFieldsPrivate(BibTeXFields *parent)
            : p(parent) {
        kDebug() << "looking for " << KStandardDirs::locate("appdata", "fieldtypes.rc");
        systemDefaultsConfig = new KConfig(KStandardDirs::locate("appdata", "fieldtypes.rc"), KConfig::SimpleConfig);
        kDebug() << "looking for " << KStandardDirs::locateLocal("appdata", "fieldtypes.rc");
        userConfig = KSharedConfig::openConfig(KStandardDirs::locateLocal("appdata", "fieldtypes.rc"), KConfig::SimpleConfig);
    }

    ~BibTeXFieldsPrivate() {
        delete systemDefaultsConfig;
    }

    void load() {
        unsigned int sumWidth = 0;
        FieldDescription fd;

        p->clear();
        for (int col = 1; col < bibTeXFieldsMaxColumnCount; ++col) {
            QString groupName = QString("Column%1").arg(col);
            KConfigGroup usercg(userConfig, groupName);
            KConfigGroup systemcg(systemDefaultsConfig, groupName);

            fd.upperCamelCase = systemcg.readEntry("UpperCamelCase", "");
            fd.upperCamelCase = usercg.readEntry("UpperCamelCase", fd.upperCamelCase);
            if (fd.upperCamelCase.isEmpty()) continue;

            fd.upperCamelCaseAlt = systemcg.readEntry("UpperCamelCaseAlt", "");
            fd.upperCamelCaseAlt = usercg.readEntry("UpperCamelCaseAlt", fd.upperCamelCaseAlt);
            if (fd.upperCamelCaseAlt.isEmpty()) fd.upperCamelCaseAlt = QString::null;

            fd.label = systemcg.readEntry("Label", fd.upperCamelCase);
            fd.label = usercg.readEntry("Label", fd.label);

            fd.defaultWidth = systemcg.readEntry("DefaultWidth", sumWidth / col);
            fd.defaultWidth = usercg.readEntry("DefaultWidth", fd.defaultWidth);

            fd.width = systemcg.readEntry("Width", fd.defaultWidth);
            fd.width = usercg.readEntry("Width", fd.width);
            sumWidth += fd.width;

            fd.visible = systemcg.readEntry("Visible", true);
            fd.visible = usercg.readEntry("Visible", fd.visible);

            QString typeFlags = systemcg.readEntry("TypeFlags", "Source");
            typeFlags = usercg.readEntry("TypeFlags", typeFlags);

            fd.typeFlags = typeFlagsFromString(typeFlags);
            QString preferredTypeFlag = typeFlags.split(';').first();
            fd.preferredTypeFlag = typeFlagFromString(preferredTypeFlag);
            p->append(fd);
        }
    }
};

BibTeXFields *BibTeXFields::BibTeXFieldsPrivate::singleton = NULL;

BibTeXFields::BibTeXFields()
        : d(new BibTeXFieldsPrivate(this))
{
    d->load();
}

BibTeXFields::~BibTeXFields()
{
    delete d;
}

BibTeXFields* BibTeXFields::self()
{
    if (BibTeXFieldsPrivate::singleton == NULL)
        BibTeXFieldsPrivate::singleton = new BibTeXFields();
    return BibTeXFieldsPrivate::singleton;
}

void BibTeXFields::save()
{
    return; // FIXME avoid saving config for now ...
    int col = 1;
    for (Iterator it = begin(); it != end(); ++it, ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(d->userConfig, groupName);
        FieldDescription &fd = *it;
        usercg.writeEntry("Width", fd.width);
        usercg.writeEntry("Visible", fd.visible);
        QString typeFlagsString = fd.typeFlags == fd.preferredTypeFlag ? "" : ";" + typeFlagsToString(fd.typeFlags);
        typeFlagsString.prepend(typeFlagToString(fd.preferredTypeFlag));
        usercg.writeEntry("TypeFlags", typeFlagsString);
    }

    d->userConfig->sync();
}

void BibTeXFields::resetToDefaults()
{
    for (int col = 1; col < bibTeXFieldsMaxColumnCount; ++col) {
        QString groupName = QString("Column%1").arg(col);
        KConfigGroup usercg(d->userConfig, groupName);
        usercg.deleteGroup();
    }

    d->load();
}

QString BibTeXFields::format(const QString& name, KBibTeX::Casing casing) const
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
            if (itName == iName && (*it).upperCamelCaseAlt == QString::null) {
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
            if (itName == iName && (*it).upperCamelCaseAlt == QString::null) {
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

const FieldDescription* BibTeXFields::find(const QString &name) const
{
    const FieldDescription* result = NULL;

    const QString iName = name.toLower();
    for (ConstIterator it = constBegin(); it != constEnd(); ++it) {
        if ((*it).upperCamelCase.toLower() == iName && (result == NULL || (*it).upperCamelCaseAlt == QString::null))
            result = &(*it);
    }
    return result;
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
    return QString::null;
}
