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

#include "file.h"

#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QStringList>

#ifdef HAVE_KF5
#include <KSharedConfig>
#include <KConfigGroup>
#endif // HAVE_KF5

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
    quint64 validInvalidField;
    static const quint64 initialInternalIdCounter;
    static quint64 internalIdCounter;

#ifdef HAVE_KF5
    KSharedConfigPtr config;
    const QString configGroupName;
#endif // HAVE_KF5

public:
    const quint64 internalId;
    QHash<QString, QVariant> properties;

    explicit FilePrivate(File *parent)
            : validInvalidField(valid),
#ifdef HAVE_KF5
        config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("FileExporterBibTeX")),
#endif // HAVE_KF5
        internalId(++internalIdCounter) {
        Q_UNUSED(parent)
        const bool isValid = checkValidity();
        if (!isValid) qCDebug(LOG_KBIBTEX_DATA) << "Creating File instance" << internalId << "  Valid?" << isValid;
#ifdef HAVE_KF5
        loadConfiguration();
#endif // HAVE_KF5
    }

    ~FilePrivate() {
        const bool isValid = checkValidity();
        if (!isValid) qCDebug(LOG_KBIBTEX_DATA) << "Deleting File instance" << internalId << "  Valid?" << isValid;
        validInvalidField = invalid;
    }

    /// Copy-assignment operator
    FilePrivate &operator= (const FilePrivate &other) {
        if (this != &other) {
            validInvalidField = other.validInvalidField;
            properties = other.properties;
            const bool isValid = checkValidity();
            if (!isValid) qCDebug(LOG_KBIBTEX_DATA) << "Assigning File instance" << other.internalId << "to" << internalId << "  Is other valid?" << other.checkValidity() << "  Self valid?" << isValid;
        }
        return *this;
    }

    /// Move-assignment operator
    FilePrivate &operator= (FilePrivate &&other) {
        if (this != &other) {
            validInvalidField = std::move(other.validInvalidField);
            properties = std::move(other.properties);
            const bool isValid = checkValidity();
            if (!isValid) qCDebug(LOG_KBIBTEX_DATA) << "Assigning File instance" << other.internalId << "to" << internalId << "  Is other valid?" << other.checkValidity() << "  Self valid?" << isValid;
        }
        return *this;
    }

#ifdef HAVE_KF5
    void loadConfiguration() {
        /// Load and set configuration as stored in settings
        KConfigGroup configGroup(config, configGroupName);
        properties.insert(File::Encoding, configGroup.readEntry(Preferences::keyEncoding, Preferences::defaultEncoding));
        properties.insert(File::StringDelimiter, configGroup.readEntry(Preferences::keyStringDelimiter, Preferences::defaultStringDelimiter));
        properties.insert(File::QuoteComment, static_cast<Preferences::QuoteComment>(configGroup.readEntry(Preferences::keyQuoteComment, static_cast<int>(Preferences::defaultQuoteComment))));
        properties.insert(File::KeywordCasing, static_cast<KBibTeX::Casing>(configGroup.readEntry(Preferences::keyKeywordCasing, static_cast<int>(Preferences::defaultKeywordCasing))));
        properties.insert(File::NameFormatting, configGroup.readEntry(Preferences::keyPersonNameFormatting, QString()));
        properties.insert(File::ProtectCasing, configGroup.readEntry(Preferences::keyProtectCasing, static_cast<int>(Preferences::defaultProtectCasing)));
        properties.insert(File::ListSeparator, configGroup.readEntry(Preferences::keyListSeparator, Preferences::defaultListSeparator));
    }
#endif // HAVE_KF5

    bool checkValidity() const {
        if (validInvalidField != valid) {
            /// 'validInvalidField' must equal to the know 'valid' value
            qCWarning(LOG_KBIBTEX_DATA) << "Failed validity check: " << validInvalidField << "!=" << valid;
            return false;
        } else if (internalId <= initialInternalIdCounter) {
            /// Internal id counter starts at initialInternalIdCounter+1
            qCWarning(LOG_KBIBTEX_DATA) << "Failed validity check: " << internalId << "< " << (initialInternalIdCounter + 1);
            return false;
        } else if (internalId > 600000) {
            /// Reasonable assumption: not more that 500000 ids used
            qCWarning(LOG_KBIBTEX_DATA) << "Failed validity check: " << internalId << "> 600000";
            return false;
        }
        return true;
    }
};

const quint64 File::FilePrivate::initialInternalIdCounter = 99999;
quint64 File::FilePrivate::internalIdCounter = File::FilePrivate::initialInternalIdCounter;

File::File()
        : QList<QSharedPointer<Element> >(), d(new FilePrivate(this))
{
    /// nothing
}

File::File(const File &other)
        : QList<QSharedPointer<Element> >(other), d(new FilePrivate(this))
{
    d->operator =(*other.d);
}

File::File(File &&other)
        : QList<QSharedPointer<Element> >(std::move(other)), d(new FilePrivate(this))
{
    d->operator =(std::move(*other.d));
}


File::~File()
{
    Q_ASSERT_X(d->checkValidity(), "File::~File()", "This File object is not valid");
    delete d;
}

File &File::operator= (const File &other) {
    if (this != &other)
        d->operator =(*other.d);
    return *this;
}

File &File::operator= (File &&other) {
    if (this != &other)
        d->operator =(std::move(*other.d));
    return *this;
}

const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const" << "This File object is not valid";
    for (const auto &element : const_cast<const File &>(*this)) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(etEntry) ? element.dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull()) {
            if (entry->id() == key)
                return entry;
        } else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(etMacro) ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
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
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "QStringList File::allKeys(ElementTypes elementTypes) const" << "This File object is not valid";
    QStringList result;
    result.reserve(size());
    for (const auto &element : const_cast<const File &>(*this)) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(etEntry) ? element.dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull())
            result.append(entry->id());
        else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(etMacro) ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
            if (!macro.isNull())
                result.append(macro->key());
        }
    }

    return result;
}

QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const" << "This File object is not valid";
    QSet<QString> valueSet;
    const QString lcFieldName = fieldName.toLower();

    for (const auto &element : const_cast<const File &>(*this)) {
        const QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
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
#ifdef HAVE_KF5
                                personNameFormattingList << Preferences::personNameFormatLastFirst << Preferences::personNameFormatFirstLast;
                                /// Check configuration if user-specified formatting template is different
                                KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
                                KConfigGroup configGroup(config, "General");
                                QString personNameFormatting = configGroup.readEntry(Preferences::keyPersonNameFormatting, Preferences::defaultPersonNameFormatting);
                                /// Add user's template if it differs from the two specified above
                                if (!personNameFormattingList.contains(personNameFormatting))
                                    personNameFormattingList << personNameFormatting;
#else // HAVE_KF5
                                personNameFormattingList << QStringLiteral("<%l><, %s><, %f>") << QStringLiteral("<%f ><%l>< %s>");
#endif // HAVE_KF5
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
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "QStringList File::uniqueEntryValuesList(const QString &fieldName) const" << "This File object is not valid";
    QSet<QString> valueSet = uniqueEntryValuesSet(fieldName);
    QStringList list = valueSet.toList();
    list.sort();
    return list;
}

void File::setProperty(const QString &key, const QVariant &value)
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "void File::setProperty(const QString &key, const QVariant &value)" << "This File object is not valid";
    d->properties.insert(key, value);
}

QVariant File::property(const QString &key) const
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "QVariant File::property(const QString &key) const" << "This File object is not valid";
    return d->properties.contains(key) ? d->properties.value(key) : QVariant();
}

QVariant File::property(const QString &key, const QVariant &defaultValue) const
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "QVariant File::property(const QString &key, const QVariant &defaultValue) const" << "This File object is not valid";
    return d->properties.value(key, defaultValue);
}

bool File::hasProperty(const QString &key) const
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "bool File::hasProperty(const QString &key) const" << "This File object is not valid";
    return d->properties.contains(key);
}

#ifdef HAVE_KF5
void File::setPropertiesToDefault()
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "void File::setPropertiesToDefault()" << "This File object is not valid";
    d->loadConfiguration();
}
#endif // HAVE_KF5

bool File::checkValidity() const
{
    return d->checkValidity();
}

QDebug operator<<(QDebug dbg, const File &file) {
    dbg.nospace() << "File is " << (file.checkValidity() ? "" : "NOT ") << "valid and has " << file.count() << " members";
    return dbg;
}
