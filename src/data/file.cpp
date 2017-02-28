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

#include "file.h"

#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QStringList>
#include <QRegExp>

#include <KSharedConfig>
#include <KConfigGroup>

#include "preferences.h"
#include "entry.h"
#include "element.h"
#include "macro.h"
#include "comment.h"
#include "logging_data.h"

const QString File::Url = QStringLiteral("Url");
const QString File::Encoding = QStringLiteral("Encoding");
const QString File::StringDelimiter = QStringLiteral("StringDelimiter");
const QString File::QuoteComment = QStringLiteral("QuoteComment");
const QString File::KeywordCasing = QStringLiteral("KeywordCasing");
const QString File::ProtectCasing = QStringLiteral("ProtectCasing");
const QString File::NameFormatting = QStringLiteral("NameFormatting");
const QString File::ListSeparator = QStringLiteral("ListSeparator");

const quint64 valid = 0x08090a0b0c0d0e0f;
const quint64 invalid = 0x0102030405060708;

class File::FilePrivate
{
private:
    // UNUSED File *p;
    quint64 validInvalidField;
    static quint64 internalIdCounter;

    KSharedConfigPtr config;
    const QString configGroupName;

public:
    const quint64 internalId;
    QHash<QString, QVariant> properties;

    FilePrivate(File */* UNUSED parent*/)
        : /* UNUSED p(parent),*/ validInvalidField(valid), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("FileExporterBibTeX")), internalId(++internalIdCounter) {
        const bool isValid = checkValidity();
        qCDebug(LOG_KBIBTEX_DATA) << "Creating File instance" << internalId << "  Valid?" << isValid;
        loadConfiguration();
    }

    FilePrivate(File */* UNUSED parent*/, const File &other)
        : /* UNUSED p(parent),*/ validInvalidField(valid), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("FileExporterBibTeX")), internalId(++internalIdCounter), properties(other.d->properties) {
        const bool isValid = checkValidity();
        qCDebug(LOG_KBIBTEX_DATA) << "Creating File instance" << internalId << "  Valid?" << isValid;
        loadConfiguration();
    }

    ~FilePrivate() {
        const bool isValid = checkValidity();
        qCDebug(LOG_KBIBTEX_DATA) << "Deleting File instance" << internalId << "  Valid?" << isValid;
        validInvalidField = invalid;
    }

    FilePrivate &operator= (const FilePrivate &other) {
        validInvalidField = other.validInvalidField;
        properties = other.properties;
        const bool isValid = checkValidity();
        qCDebug(LOG_KBIBTEX_DATA) << "Assigning File instance" << other.internalId << "to" << internalId << "  Is other valid?" << other.checkValidity() << "  Self valid?" << isValid;
        return *this;
    }

    void loadConfiguration() {
        /// Load and set configuration as stored in settings
        KConfigGroup configGroup(config, configGroupName);
        properties.insert(File::Encoding, configGroup.readEntry(Preferences::keyEncoding, Preferences::defaultEncoding));
        properties.insert(File::StringDelimiter, configGroup.readEntry(Preferences::keyStringDelimiter, Preferences::defaultStringDelimiter));
        properties.insert(File::QuoteComment, (Preferences::QuoteComment)configGroup.readEntry(Preferences::keyQuoteComment, (int)Preferences::defaultQuoteComment));
        properties.insert(File::KeywordCasing, (KBibTeX::Casing)configGroup.readEntry(Preferences::keyKeywordCasing, (int)Preferences::defaultKeywordCasing));
        properties.insert(File::NameFormatting, configGroup.readEntry(Preferences::keyPersonNameFormatting, QString()));
        properties.insert(File::ProtectCasing, configGroup.readEntry(Preferences::keyProtectCasing, (int)Preferences::defaultProtectCasing));
        properties.insert(File::ListSeparator, configGroup.readEntry(Preferences::keyListSeparator, Preferences::defaultListSeparator));
    }

    bool checkValidity() const {
        return validInvalidField == valid ///< 'validInvalidField' must contain the know 'valid' value
                && internalId >= 100000 ///< internal id counter starts at 100000
                && internalId < 200000; ///< reasonable assumption: not more that 100000 ids used
    }
};

quint64 File::FilePrivate::internalIdCounter = 99999;

File::File()
        : QList<QSharedPointer<Element> >(), d(new FilePrivate(this))
{
    // nothing
}

File::~File()
{
    Q_ASSERT_X(d->checkValidity(), "File::~File()", "This File object is not valid");
    delete d;
}

File &File::operator= (const File &other) {
    d->operator =(*other.d);
    return *this;
}

const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const
{
    Q_ASSERT_X(d->checkValidity(), "const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const", "This File object is not valid");
    for (QList<QSharedPointer<Element> >::ConstIterator it = constBegin(); it != constEnd(); ++it) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(etEntry) ? (*it).dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull()) {
            if (entry->id() == key)
                return entry;
        } else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(etMacro) ? (*it).dynamicCast<Macro>() : QSharedPointer<Macro>();
            if (!macro.isNull()) {
                if (macro->key() == key)
                    return macro;
            }
        }
    }

    return QSharedPointer<Element>();
}

QStringList File::allKeys(ElementTypes elementTypes) const
{
    Q_ASSERT_X(d->checkValidity(), "QStringList File::allKeys(ElementTypes elementTypes) const", "This File object is not valid");
    QStringList result;

    for (QList<QSharedPointer<Element> >::ConstIterator it = constBegin(); it != constEnd(); ++it) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(etEntry) ? (*it).dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull())
            result.append(entry->id());
        else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(etMacro) ? (*it).dynamicCast<Macro>() : QSharedPointer<Macro>();
            if (!macro.isNull())
                result.append(macro->key());
        }
    }

    return result;
}

QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const
{
    Q_ASSERT_X(d->checkValidity(), "QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const", "This File object is not valid");
    QSet<QString> valueSet;
    const QString lcFieldName = fieldName.toLower();

    for (QList<QSharedPointer<Element> >::ConstIterator it = constBegin(); it != constEnd(); ++it) {
        const QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
        if (!entry.isNull())
            for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
                if (it.key().toLower() == lcFieldName) {
                    const auto itValue = it.value();
                    for (const QSharedPointer<ValueItem> &valueItem : itValue) {
                        /// Check if ValueItem to process points to a person
                        const QSharedPointer<Person> person = valueItem.dynamicCast<Person>();
                        if (!person.isNull()) {
                            /// Assemble a list of formatting templates for a person's name
                            static QStringList personNameFormattingList; ///< use static to do pattern assembly only once
                            if (personNameFormattingList.isEmpty()) {
                                /// Use the two default patterns last-name-first and first-name-first
                                personNameFormattingList << Preferences::personNameFormatLastFirst << Preferences::personNameFormatFirstLast;
                                /// Check configuration if user-specified formatting template is different
                                KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
                                KConfigGroup configGroup(config, "General");
                                QString personNameFormatting = configGroup.readEntry(Preferences::keyPersonNameFormatting, Preferences::defaultPersonNameFormatting);
                                /// Add user's template if it differs from the two specified above
                                if (!personNameFormattingList.contains(personNameFormatting))
                                    personNameFormattingList << personNameFormatting;
                            }
                            /// Add person's name formatted using each of the templates assembled above
                            for (const QString &personNameFormatting : const_cast<const QStringList &>(personNameFormattingList)) {
                                valueSet.insert(Person::transcribePersonName(person.data(), personNameFormatting));
                            }
                        } else {
                            /// Default case: use PlainTextValue::text to translate ValueItem
                            /// to a human-readable text
                            valueSet.insert(PlainTextValue::text(*valueItem));
                        }
                    }
                }
    }

    return valueSet;
}

QStringList File::uniqueEntryValuesList(const QString &fieldName) const
{
    Q_ASSERT_X(d->checkValidity(), "QStringList File::uniqueEntryValuesList(const QString &fieldName) const", "This File object is not valid");
    QSet<QString> valueSet = uniqueEntryValuesSet(fieldName);
    QStringList list = valueSet.toList();
    list.sort();
    return list;
}

void File::setProperty(const QString &key, const QVariant &value)
{
    Q_ASSERT_X(d->checkValidity(), "void File::setProperty(const QString &key, const QVariant &value)", "This File object is not valid");
    d->properties.insert(key, value);
}

QVariant File::property(const QString &key) const
{
    Q_ASSERT_X(d->checkValidity(), "QVariant File::property(const QString &key) const", "This File object is not valid");
    return d->properties.contains(key) ? d->properties.value(key) : QVariant();
}

QVariant File::property(const QString &key, const QVariant &defaultValue) const
{
    Q_ASSERT_X(d->checkValidity(), "QVariant File::property(const QString &key, const QVariant &defaultValue) const", "This File object is not valid");
    return d->properties.value(key, defaultValue);
}

bool File::hasProperty(const QString &key) const
{
    Q_ASSERT_X(d->checkValidity(), "bool File::hasProperty(const QString &key) const", "This File object is not valid");
    return d->properties.contains(key);
}

void File::setPropertiesToDefault()
{
    Q_ASSERT_X(d->checkValidity(), "void File::setPropertiesToDefault()", "This File object is not valid");
    d->loadConfiguration();
}

quint64 File::id() const
{
    return d->internalId;
}

bool File::isMemoryValid() const
{
    return d->checkValidity();
}
