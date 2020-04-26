/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <Preferences>
#include "entry.h"
#include "element.h"
#include "macro.h"
#include "comment.h"
#include "preamble.h"
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

public:
    const quint64 internalId;
    QHash<QString, QVariant> properties;

    explicit FilePrivate(File *parent)
            : validInvalidField(valid), internalId(++internalIdCounter)
    {
        Q_UNUSED(parent)
        const bool isValid = checkValidity();
        if (!isValid) qCDebug(LOG_KBIBTEX_DATA) << "Creating File instance" << internalId << "  Valid?" << isValid;
        loadConfiguration();
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

    void loadConfiguration() {
        /// Load and set configuration as stored in settings
        properties.insert(File::Encoding, Preferences::instance().bibTeXEncoding());
        properties.insert(File::StringDelimiter, Preferences::instance().bibTeXStringDelimiter());
        properties.insert(File::QuoteComment,  static_cast<int>(Preferences::instance().bibTeXQuoteComment()));
        properties.insert(File::KeywordCasing, static_cast<int>(Preferences::instance().bibTeXKeywordCasing()));
        properties.insert(File::NameFormatting, Preferences::instance().personNameFormat());
        properties.insert(File::ProtectCasing, static_cast<int>(Preferences::instance().bibTeXProtectCasing() ? Qt::Checked : Qt::Unchecked));
        properties.insert(File::ListSeparator,  Preferences::instance().bibTeXListSeparator());
    }

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

bool File::operator==(const File &other) const {
    if (size() != other.size()) return false;

    for (File::ConstIterator myIt = constBegin(), otherIt = other.constBegin(); myIt != constEnd() && otherIt != constEnd(); ++myIt, ++otherIt) {
        QSharedPointer<const Entry> myEntry = myIt->dynamicCast<const Entry>();
        QSharedPointer<const Entry> otherEntry = otherIt->dynamicCast<const Entry>();
        if ((myEntry.isNull() && !otherEntry.isNull()) || (!myEntry.isNull() && otherEntry.isNull())) return false;
        if (!myEntry.isNull() && !otherEntry.isNull()) {
            if (myEntry->operator !=(*otherEntry.data()))
                return false;
        } else {
            QSharedPointer<const Macro> myMacro = myIt->dynamicCast<const Macro>();
            QSharedPointer<const Macro> otherMacro = otherIt->dynamicCast<const Macro>();
            if ((myMacro.isNull() && !otherMacro.isNull()) || (!myMacro.isNull() && otherMacro.isNull())) return false;
            if (!myMacro.isNull() && !otherMacro.isNull()) {
                if (myMacro->operator !=(*otherMacro.data()))
                    return false;
            } else {
                QSharedPointer<const Preamble> myPreamble = myIt->dynamicCast<const Preamble>();
                QSharedPointer<const Preamble> otherPreamble = otherIt->dynamicCast<const Preamble>();
                if ((myPreamble.isNull() && !otherPreamble.isNull()) || (!myPreamble.isNull() && otherPreamble.isNull())) return false;
                if (!myPreamble.isNull() && !otherPreamble.isNull()) {
                    if (myPreamble->operator !=(*otherPreamble.data()))
                        return false;
                } else {
                    QSharedPointer<const Comment> myComment = myIt->dynamicCast<const Comment>();
                    QSharedPointer<const Comment> otherComment = otherIt->dynamicCast<const Comment>();
                    if ((myComment.isNull() && !otherComment.isNull()) || (!myComment.isNull() && otherComment.isNull())) return false;
                    if (!myComment.isNull() && !otherComment.isNull()) {
                        // TODO right now, don't care if comments are equal
                        qCDebug(LOG_KBIBTEX_DATA) << "File objects being compared contain comments, ignoring those";
                    } else {
                        /// This case should never be reached
                        qCWarning(LOG_KBIBTEX_DATA) << "Met unhandled case while comparing two File objects";
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

bool File::operator!=(const File &other) const {
    return !operator ==(other);
}

const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const" << "This File object is not valid";
    for (const auto &element : const_cast<const File &>(*this)) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(ElementType::Entry) ? element.dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull()) {
            if (entry->id() == key)
                return entry;
        } else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(ElementType::Macro) ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
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
        const QSharedPointer<Entry> entry = elementTypes.testFlag(ElementType::Entry) ? element.dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull())
            result.append(entry->id());
        else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(ElementType::Macro) ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
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
                            QSet<QString> personNameFormattingSet {Preferences::personNameFormatLastFirst, Preferences::personNameFormatFirstLast};
                            personNameFormattingSet.insert(Preferences::instance().personNameFormat());
                            /// Add person's name formatted using each of the templates assembled above
                            for (const QString &personNameFormatting : const_cast<const QSet<QString> &>(personNameFormattingSet))
                                valueSet.insert(Person::transcribePersonName(person.data(), personNameFormatting));
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
#if QT_VERSION >= 0x050e00
    QStringList list(valueSet.constBegin(), valueSet.constEnd()); ///< This function was introduced in Qt 5.14
#else // QT_VERSION < 0x050e00
    QStringList list = valueSet.toList();
#endif // QT_VERSION >= 0x050e00
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

void File::setPropertiesToDefault()
{
    if (!d->checkValidity())
        qCCritical(LOG_KBIBTEX_DATA) << "void File::setPropertiesToDefault()" << "This File object is not valid";
    d->loadConfiguration();
}

bool File::checkValidity() const
{
    return d->checkValidity();
}

QDebug operator<<(QDebug dbg, const File &file) {
    dbg.nospace() << "File is " << (file.checkValidity() ? "" : "NOT ") << "valid and has " << file.count() << " members";
    return dbg;
}
