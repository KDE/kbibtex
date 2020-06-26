###########################################################################
#   SPDX-License-Identifier: GPL-2.0-or-later
#                                                                         #
#   SPDX-FileCopyrightText: 2019-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
#                                                                         #
#   This script is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This script is distributed in the hope that it will be useful,        #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this script; if not, see <https://www.gnu.org/licenses/>.  #
###########################################################################

import sys
import json
import datetime


def print_copyright_header(outputdevice=sys.stdout):
    """Print the default copyright statement to the output device."""

    print("""/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-{year} Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
 ***************************************************************************/"""
          .format(year=datetime.date.today().year), file=outputdevice)
    print(file=outputdevice)
    print("""/// This file has been automatically generated using the script 'preferences-generator.py'
/// based on configuration data from file 'preferences.json'. If there are any problems or
/// bugs, you need to fix those two files and re-generated both 'preferences.h' and
/// 'preferences.cpp'. Manual changes in this file will be overwritten the next time the
/// script will be run. You have been warned.""", file=outputdevice)
    print(file=outputdevice)


def needsReference(type):
    """Check if given Qt/C++ data type should be passed by reference rather than by value."""
    return type in ["QString", "QStringList"] \
        or type.startswith(('QPair<', 'QVector<', 'QSet<', 'QList<', 'QLinkedList<', 'QMap<', 'QVector<', 'QHash<'))


def rewriteobsoletecode(input, indent):
    inputlist = input if isinstance(input, list) else ([input] if isinstance(input, str) else None)
    if not isinstance(inputlist, list):
        return None

    result = []
    for line in inputlist:
        if "::KeepEmptyParts" in line or "::SkipEmptyParts" in line:
            result.append("#if QT_VERSION >= 0x050e00")
            result.append(indent + line.replace("QString::KeepEmptyParts", "Qt::KeepEmptyParts").replace("QString::SkipEmptyParts", "Qt::SkipEmptyParts"))
            result.append("#else // QT_VERSION < 0x050e00")
            result.append(indent + line.replace("Qt::KeepEmptyParts", "QString::KeepEmptyParts").replace("Qt::SkipEmptyParts", "QString::SkipEmptyParts"))
            result.append("#endif // QT_VERSION >= 0x050e00")
        else:
            result.append(indent + line)
    return result


def print_header(headerincludes, implementationincludes, enums, settings, outputdevice=sys.stdout):
    """Print the header file for the Preferences ('preferences.h')."""

    print_copyright_header(outputdevice)

    # Include guard definition
    print("#ifndef KBIBTEX_CONFIG_PREFERENCES_H", file=outputdevice)
    print("#define KBIBTEX_CONFIG_PREFERENCES_H", file=outputdevice)
    print(file=outputdevice)

    # Include other headers as necessary
    for includeline in headerincludes:
        if len(includeline) == 0:
            print(file=outputdevice)
        elif includeline[0] == '#':
            print(includeline, file=outputdevice)
        else:
            print("#include", includeline, file=outputdevice)
    if headerincludes:
        print(file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    print('#include "kbibtexconfig_export.h"', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)
    print(file=outputdevice)

    print("class KBIBTEXCONFIG_EXPORT Preferences {", file=outputdevice)
    print("public:", file=outputdevice)
    print("    static Preferences &instance();", file=outputdevice)
    print("    ~Preferences();", file=outputdevice)
    print(file=outputdevice)

    for enum in sorted(enums):
        print("    enum class " + enum + " {", end="", file=outputdevice)
        first = True
        for enumvaluepair in enums[enum]:
            if not first:
                print(",", end="", file=outputdevice)
            print(" ", end="", file=outputdevice)
            if isinstance(enumvaluepair, list):
                print(enumvaluepair[0], end="", file=outputdevice)
                if len(enumvaluepair) >= 2 and enumvaluepair[1] != None:
                    print(
                        " = " + str(enumvaluepair[1]), end="", file=outputdevice)
            elif isinstance(enumvaluepair, str):
                print(enumvaluepair, end="", file=outputdevice)
            first = False
        print(" };", file=outputdevice)
    if enums:
        print(file=outputdevice)

    for setting in settings:
        stem = setting['stem']
        type = setting['type']
        lowercasestart = stem[0].lower() + stem[1:]
        print(file=outputdevice)
        print("    /// ***", stem, "of type", type, "***", file=outputdevice)
        print(file=outputdevice)
        if 'predefined' in setting:
            for predefined in setting['predefined']:
                print("    static const " + type + " " +
                      lowercasestart + predefined[0] + ";", file=outputdevice)
        print("    static const " +
              type + " default" + stem + ";", file=outputdevice)
        if 'availabletype' in setting:
            print("    static const " +
                  setting['availabletype'] + " available" + stem + "s;", file=outputdevice)
        print(("    const " if needsReference(type) else "    ") + type + (" &" if needsReference(type) else " ") +
              lowercasestart + "();", file=outputdevice)
        print('#ifdef HAVE_KF5', file=outputdevice)
        print('    /*!', file=outputdevice)
        print('     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions', file=outputdevice)
        print('     */', file=outputdevice)
        print("    bool set" + stem + "(const " +
              type + (" &" if needsReference(type) else " ") + lowercasestart + ");", file=outputdevice)
        print('#endif // HAVE_KF5', file=outputdevice)
        print("", file=outputdevice)

    print("private:", file=outputdevice)
    print("    Q_DISABLE_COPY(Preferences)", file=outputdevice)
    print(file=outputdevice)
    print("    explicit Preferences();", file=outputdevice)
    print(file=outputdevice)
    print("    class Private;", file=outputdevice)
    print("    Private *const d;", file=outputdevice)
    print("};", file=outputdevice)
    print(file=outputdevice)
    print("#endif // KBIBTEX_CONFIG_PREFERENCES_H", file=outputdevice)


def print_implementation(headerincludes, implementationincludes, enums, settings, outputdevice=sys.stdout):
    """Print the implementatiom file for the Preferences ('preferences.cpp')."""

    print_copyright_header(outputdevice)

    # Include headers that will always be necessary
    print('#include <Preferences>', file=outputdevice)
    print(file=outputdevice)
    print('#include <QCoreApplication>', file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    print('#include <KLocalizedString>', file=outputdevice)
    print('#include <KSharedConfig>', file=outputdevice)
    print('#include <KConfigWatcher>', file=outputdevice)
    print('#include <KConfigGroup>', file=outputdevice)
    print('#else // HAVE_KF5', file=outputdevice)
    print('#define i18n(text) QStringLiteral(text)', file=outputdevice)
    print('#define i18nc(comment,text) QStringLiteral(text)', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)
    print(file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    print('#include <NotificationHub>', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)
    print(file=outputdevice)
    # Include other headers as necessary
    for includeline in implementationincludes:
        if len(includeline) == 0:
            print(file=outputdevice)
        elif includeline[0] == '#':
            print(includeline, file=outputdevice)
        else:
            print("#include", includeline, file=outputdevice)
    if implementationincludes:
        print(file=outputdevice)
    print('class Preferences::Private', file=outputdevice)
    print('{', file=outputdevice)
    print('public:', file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    print('    KSharedConfigPtr config;', file=outputdevice)
    print('    KConfigWatcher::Ptr watcher;', file=outputdevice)
    print(file=outputdevice)
    for setting in settings:
        stem = setting['stem']
        type = setting['type']
        if type in enums:
            type = "Preferences::" + type
        print('    bool dirtyFlag' + stem + ';', file=outputdevice)
        print('    ' + type + ' cached' + stem + ';', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)

    # Constructor for Preferences::Private
    print(file=outputdevice)
    print('    Private(Preferences *)', file=outputdevice)
    print('    {', file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    print('        config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));', file=outputdevice)
    print('        watcher = KConfigWatcher::create(config);', file=outputdevice)
    for setting in settings:
        stem = setting['stem']
        print('        dirtyFlag' + stem + ' = true;', file=outputdevice)
        print('        cached' + stem + ' = Preferences::default' +
              stem + ';', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)
    print('    }', file=outputdevice)

    print(file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    first = True
    for setting in settings:
        stem = setting['stem']
        type = setting['type']
        if type in enums:
            type = "Preferences::" + type
        if first:
            first = False
        else:
            print(file=outputdevice)
        print('    inline bool validateValueFor' + stem + '(const ' + type +
              (" &" if needsReference(type) else " ") + "valueToBeChecked) {", file=outputdevice)
        if 'validationcode' in setting:
            if isinstance(setting['validationcode'], list):
                for line in setting['validationcode']:
                    print('        ' + line, file=outputdevice)
                if not setting['validationcode'][-1].startswith("return "):
                    print('        return false;', file=outputdevice)
            elif isinstance(setting['validationcode'], str):
                print('        ' +
                      setting['validationcode'], file=outputdevice)
        elif 'availabletype' in setting and setting['availabletype'].startswith('QVector<QPair<' + type + ","):
            print('        for (' + setting['availabletype'] + '::ConstIterator it = Preferences::available' + stem +
                  's.constBegin(); it != Preferences::available' + stem + 's.constEnd(); ++it)', file=outputdevice)
            print(
                '            if (it->first == valueToBeChecked) return true;', file=outputdevice)
            print('        return false;', file=outputdevice)
        elif 'availabletype' in setting and setting['availabletype'] == "QStringList":
            print('        return Preferences::available' + stem +
                  's.contains(valueToBeChecked);', file=outputdevice)
        else:
            print('        Q_UNUSED(valueToBeChecked)', file=outputdevice)
            print('        return true;', file=outputdevice)
        print('    }', file=outputdevice)

        if 'readEntry' in setting:
            print(file=outputdevice)
            print('    ' + type + ' readEntry' + stem +
                  '(const KConfigGroup &configGroup, const QString &key) const', file=outputdevice)
            print('    {', file=outputdevice)
            readEntryList = rewriteobsoletecode(setting['readEntry'], '        ')
            if isinstance(readEntryList, list):
                for line in readEntryList:
                    print(line, file=outputdevice)
            print('    }', file=outputdevice)

        if 'writeEntry' in setting:
            print(file=outputdevice)
            print('    void writeEntry' + stem + '(KConfigGroup &configGroup, const QString &key, const ' +
                  type + (" &" if needsReference(type) else " ") + 'valueToBeWritten)', file=outputdevice)
            print('    {', file=outputdevice)
            if isinstance(setting['writeEntry'], list):
                for line in setting['writeEntry']:
                    print('        ' + line, file=outputdevice)
            elif isinstance(setting['writeEntry'], str):
                print('        ' + setting['writeEntry'], file=outputdevice)
            print('    }', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)
    print('};', file=outputdevice)

    # Singleton function Preferences::instance()
    print(file=outputdevice)
    print('Preferences &Preferences::instance()', file=outputdevice)
    print('{', file=outputdevice)
    print('    static Preferences singleton;', file=outputdevice)
    print('    return singleton;', file=outputdevice)
    print('}', file=outputdevice)
    print(file=outputdevice)
    # Constructor for class Preferences
    print('Preferences::Preferences()', file=outputdevice)
    print('        : d(new Preferences::Private(this))', file=outputdevice)
    print('{', file=outputdevice)
    print('#ifdef HAVE_KF5', file=outputdevice)
    print(
        '    QObject::connect(d->watcher.data(), &KConfigWatcher::configChanged, QCoreApplication::instance(), [this](const KConfigGroup &group, const QByteArrayList &names) {', file=outputdevice)
    print('        QSet<int> eventsToPublish;', file=outputdevice)
    for setting in settings:
        stem = setting['stem']
        configgroup = setting['configgroup'] if 'configgroup' in setting else 'General'
        print('        if (group.name() == QStringLiteral("' + configgroup +
              '") && names.contains("' + stem + '")) {', file=outputdevice)
        print('            /// Configuration setting ' + stem +
              ' got changed by another Preferences instance";', file=outputdevice)
        print('            d->dirtyFlag' +
              stem + ' = true;', file=outputdevice)
        if 'notificationevent' in setting:
            print('            eventsToPublish.insert(' +
                  setting['notificationevent'] + ');', file=outputdevice)
        print('        }', file=outputdevice)
    print(file=outputdevice)
    print('        for (const int eventId : eventsToPublish)', file=outputdevice)
    print('            NotificationHub::publishEvent(eventId);', file=outputdevice)
    print('    });', file=outputdevice)
    print('#endif // HAVE_KF5', file=outputdevice)
    print('}', file=outputdevice)
    print(file=outputdevice)
    # Destructor for Preferences
    print('Preferences::~Preferences()', file=outputdevice)
    print('{', file=outputdevice)
    print('    delete d;', file=outputdevice)
    print('}', file=outputdevice)

    for setting in settings:
        stem = setting['stem']
        configgroup = setting['configgroup'] if 'configgroup' in setting else 'General'
        type = setting['type']
        typeInConfig = "int" if type in enums.keys() else type
        if 'podtype' in setting:
            typeInConfig = setting['podtype']
        if type in enums:
            type = "Preferences::" + type
        lowercasestart = stem[0].lower() + stem[1:]

        print(file=outputdevice)

        if 'predefined' in setting:
            for predefined in setting['predefined']:
                print('const ' + type + ' Preferences::' + lowercasestart +
                      predefined[0] + " = " + predefined[1] + ";", file=outputdevice)

        if 'available' in setting:
            available = setting['available']
            print('const ' + setting['availabletype'] + ' Preferences::available' + stem +
                  ('s ' if available.startswith("{") else "s = ") + available + ";", file=outputdevice)

        default = "nullptr"
        if 'default' in setting:
            default = setting['default']
            if 'predefined' in setting and default in [pair[0] for pair in setting['predefined']]:
                default = "Preferences::" + lowercasestart + default
        elif 'availabletype' in setting:
            if setting['availabletype'].startswith("QVector<QPair<"):
                default = 'Preferences::available' + stem + 's.front().first'
            elif setting['availabletype'] == "QStringList":
                default = 'Preferences::available' + stem + 's.front()'
        print('const ' + type + ' Preferences::default' + stem +
              (' ' if default.startswith("{") else " = ") + default + ";", file=outputdevice)

        print(file=outputdevice)

        print(("const " if needsReference(type) else "") + type + (" &" if needsReference(type) else " ") +
              "Preferences::" + lowercasestart + "()", file=outputdevice)
        print("{", file=outputdevice)
        print('#ifdef HAVE_KF5', file=outputdevice)
        print('    if (d->dirtyFlag' + stem + ') {', file=outputdevice)
        print('        d->config->reparseConfiguration();', file=outputdevice)
        print('        static const KConfigGroup configGroup(d->config, QStringLiteral("' +
              configgroup + '"));', file=outputdevice)
        print('        const ' + type +
              ' valueFromConfig = ', end="", file=outputdevice)
        if 'readEntry' in setting:
            print('d->readEntry' + stem + '(configGroup, QStringLiteral("' +
                  stem + '"));', file=outputdevice)
        else:
            if typeInConfig != type:
                print('static_cast<' + type + '>(', end="", file=outputdevice)
            print('configGroup.readEntry(QStringLiteral("' +
                  stem + '"), ', end="", file=outputdevice)
            if typeInConfig != type:
                print('static_cast<' + typeInConfig +
                      '>(', end="", file=outputdevice)
            print('Preferences::default' + stem, end="", file=outputdevice)
            if typeInConfig != type:
                print(')', end="", file=outputdevice)
            print(')', end="", file=outputdevice)
            if typeInConfig != type:
                print(')', end="", file=outputdevice)
            print(';', file=outputdevice)
        print('        if (d->validateValueFor' + stem +
              '(valueFromConfig)) {', file=outputdevice)
        print('            d->cached' + stem +
              ' = valueFromConfig;', file=outputdevice)
        print('            d->dirtyFlag' +
              stem + ' = false;', file=outputdevice)
        print('        } else {', file=outputdevice)
        print('            /// Configuration file setting for ' + stem +
              ' has an invalid value, using default as fallback', file=outputdevice)
        print('            set' + stem +
              '(Preferences::default' + stem + ');', file=outputdevice)
        print('        }', file=outputdevice)
        print('    }', file=outputdevice)
        print('    return d->cached' + stem + ";", file=outputdevice)
        print('#else // HAVE_KF5', file=outputdevice)
        print('    return default' + stem + ";", file=outputdevice)
        print('#endif // HAVE_KF5', file=outputdevice)
        print("}", file=outputdevice)

        print(file=outputdevice)

        print('#ifdef HAVE_KF5', file=outputdevice)
        print("bool Preferences::set" + stem + '(const ' + type +
              (" &" if needsReference(type) else " ") + 'newValue)', file=outputdevice)
        print("{", file=outputdevice)

        if 'sanitizecode' in setting:
            newValueVariable = 'sanitizedNewValue'
            if isinstance(setting['sanitizecode'], str):
                print('    ' + setting['sanitizecode'], file=outputdevice)
            elif isinstance(setting['sanitizecode'], list):
                for line in setting['sanitizecode']:
                    print('    ' + line, file=outputdevice)
            else:
                newValueVariable = 'newValue'
        elif 'availabletype' in setting and setting['availabletype'] == 'QStringList':
            newValueVariable = 'sanitizedNewValue'
            print('    ' + type + ' sanitizedNewValue = newValue;',
                  file=outputdevice)
            print('    const ' + type +
                  ' lowerSanitizedNewValue = sanitizedNewValue.toLower();', file=outputdevice)
            print('    for (const QString &known' + stem +
                  ' : available' + stem + 's)', file=outputdevice)
            print('        if (known' + stem +
                  '.toLower() == lowerSanitizedNewValue) {', file=outputdevice)
            print('            sanitizedNewValue = known' +
                  stem + ';', file=outputdevice)
            print('            break;', file=outputdevice)
            print('        }', file=outputdevice)
        else:
            newValueVariable = 'newValue'

        print('    if (!d->validateValueFor' + stem +
              '(' + newValueVariable + ')) return false;', file=outputdevice)
        print('    d->dirtyFlag' + stem + ' = false;', file=outputdevice)
        print('    d->cached' + stem + ' = ' +
              newValueVariable + ';', file=outputdevice)
        print('    static KConfigGroup configGroup(d->config, QStringLiteral("' +
              configgroup + '"));', file=outputdevice)
        print('    const ' + type + ' valueFromConfig = ',
              end="", file=outputdevice)
        if 'readEntry' in setting:
            print('d->readEntry' + stem + '(configGroup, QStringLiteral("' +
                  stem + '"));', file=outputdevice)
        else:
            if typeInConfig != type:
                print('static_cast<' + type + '>(', end="", file=outputdevice)
            print('configGroup.readEntry(QStringLiteral("' +
                  stem + '"), ', end="", file=outputdevice)
            if typeInConfig != type:
                print('static_cast<' + typeInConfig +
                      '>(', end="", file=outputdevice)
            print('Preferences::default' + stem, end="", file=outputdevice)
            if typeInConfig != type:
                print(')', end="", file=outputdevice)
            print(')', end="", file=outputdevice)
            if typeInConfig != type:
                print(')', end="", file=outputdevice)
            print(';', file=outputdevice)
        print('    if (valueFromConfig == ' + newValueVariable +
              ') return false;', file=outputdevice)
        if 'writeEntry' in setting:
            print('    d->writeEntry' + stem + '(configGroup, QStringLiteral("' +
                  stem + '"), ' + newValueVariable + ');', file=outputdevice)
        else:
            print('    configGroup.writeEntry(QStringLiteral("' +
                  stem + '"), ', end="", file=outputdevice)
            if typeInConfig != type:
                print('static_cast<' + typeInConfig +
                      '>(', end="", file=outputdevice)
            print(newValueVariable, end="", file=outputdevice)
            if typeInConfig != type:
                print(')', end="", file=outputdevice)
            print(', KConfig::Notify);', file=outputdevice)
        print('    d->config->sync();', file=outputdevice)
        # NotificationHub::publishEvent(notificationEventId);
        print('    return true;', file=outputdevice)
        print("}", file=outputdevice)
        print('#endif // HAVE_KF5', file=outputdevice)


jsondata = {}
with open("preferences.json") as jsonfile:
    jsondata = json.load(jsonfile)
headerincludes = jsondata['headerincludes'] \
    if 'headerincludes' in jsondata else {}
implementationincludes = jsondata['implementationincludes'] \
    if 'implementationincludes' in jsondata else {}
enums = jsondata['enums'] \
    if 'enums' in jsondata else {}
settings = jsondata['settings'] \
    if 'settings' in jsondata else {}

for setting in settings:
    if 'headerincludes' in setting:
        headerincludes.extend(setting['headerincludes'])

with open("/tmp/preferences.h", "w") as headerfile:
    print_header(headerincludes, implementationincludes,
                 enums, settings, outputdevice=headerfile)
with open("/tmp/preferences.cpp", "w") as implementationfile:
    print_implementation(headerincludes, implementationincludes,
                         enums, settings, outputdevice=implementationfile)
