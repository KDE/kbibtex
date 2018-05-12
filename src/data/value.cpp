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

#include "value.h"

#include <typeinfo>

#include <QSet>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>

#ifdef HAVE_KF5
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#endif // HAVE_KF5

#include "file.h"
#include "preferences.h"

quint64 ValueItem::internalIdCounter = 0;

uint qHash(const QSharedPointer<ValueItem> &valueItem)
{
    return qHash(valueItem->id());
}

const QRegularExpression ValueItem::ignoredInSorting(QStringLiteral("[{}\\\\]+"));

ValueItem::ValueItem()
        : internalId(++internalIdCounter)
{
    /// nothing
}

ValueItem::~ValueItem()
{
    /// nothing
}

quint64 ValueItem::id() const
{
    return internalId;
}

bool ValueItem::operator!=(const ValueItem &other) const
{
    return !operator ==(other);
}

Keyword::Keyword(const Keyword &other)
        : m_text(other.m_text)
{
    /// nothing
}

Keyword::Keyword(const QString &text)
        : m_text(text)
{
    /// nothing
}

void Keyword::setText(const QString &text)
{
    m_text = text;
}

QString Keyword::text() const
{
    return m_text;
}

void Keyword::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    if (replaceMode == ValueItem::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::CompleteMatch && m_text == before)
        m_text = after;
}

bool Keyword::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).remove(ignoredInSorting);
    return text.contains(pattern, caseSensitive);
}

bool Keyword::operator==(const ValueItem &other) const
{
    const Keyword *otherKeyword = dynamic_cast<const Keyword *>(&other);
    if (otherKeyword != nullptr) {
        return otherKeyword->text() == text();
    } else
        return false;
}

bool Keyword::isKeyword(const ValueItem &other) {
    return typeid(other) == typeid(Keyword);
}


Person::Person(const QString &firstName, const QString &lastName, const QString &suffix)
        : m_firstName(firstName), m_lastName(lastName), m_suffix(suffix)
{
    /// nothing
}

Person::Person(const Person &other)
        : m_firstName(other.firstName()), m_lastName(other.lastName()), m_suffix(other.suffix())
{
    /// nothing
}

QString Person::firstName() const
{
    return m_firstName;
}

QString Person::lastName() const
{
    return m_lastName;
}

QString Person::suffix() const
{
    return m_suffix;
}

void Person::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    if (replaceMode == ValueItem::AnySubstring) {
        m_firstName = m_firstName.replace(before, after);
        m_lastName = m_lastName.replace(before, after);
        m_suffix = m_suffix.replace(before, after);
    } else if (replaceMode == ValueItem::CompleteMatch) {
        if (m_firstName == before)
            m_firstName = after;
        if (m_lastName == before)
            m_lastName = after;
        if (m_suffix == before)
            m_suffix = after;
    }
}

bool Person::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString firstName = QString(m_firstName).remove(ignoredInSorting);
    const QString lastName = QString(m_lastName).remove(ignoredInSorting);
    const QString suffix = QString(m_suffix).remove(ignoredInSorting);

    return firstName.contains(pattern, caseSensitive) || lastName.contains(pattern, caseSensitive) || suffix.contains(pattern, caseSensitive) || QString(QStringLiteral("%1 %2|%2, %1")).arg(firstName, lastName).contains(pattern, caseSensitive);
}

bool Person::operator==(const ValueItem &other) const
{
    const Person *otherPerson = dynamic_cast<const Person *>(&other);
    if (otherPerson != nullptr) {
        return otherPerson->firstName() == firstName() && otherPerson->lastName() == lastName() && otherPerson->suffix() == suffix();
    } else
        return false;
}

QString Person::transcribePersonName(const Person *person, const QString &formatting)
{
    return transcribePersonName(formatting, person->firstName(), person->lastName(), person->suffix());
}

QString Person::transcribePersonName(const QString &formatting, const QString &firstName, const QString &lastName, const QString &suffix)
{
    QString result = formatting;
    int p1 = -1, p2 = -1, p3 = -1;
    while ((p1 = result.indexOf('<')) >= 0 && (p2 = result.indexOf('>', p1 + 1)) >= 0 && (p3 = result.indexOf('%', p1)) >= 0 && p3 < p2) {
        QString insert;
        switch (result[p3 + 1].toLatin1()) {
        case 'f':
            insert = firstName;
            break;
        case 'l':
            insert = lastName;
            break;
        case 's':
            insert = suffix;
            break;
        }

        if (!insert.isEmpty())
            insert = result.mid(p1 + 1, p3 - p1 - 1) + insert + result.mid(p3 + 2, p2 - p3 - 2);

        result = result.left(p1) + insert + result.mid(p2 + 1);
    }
    return result;
}

bool Person::isPerson(const ValueItem &other) {
    return typeid(other) == typeid(Person);
}

QDebug operator<<(QDebug dbg, const Person &person) {
    dbg.nospace() << "Person " << Person::transcribePersonName(&person, Preferences::defaultPersonNameFormatting);
    return dbg;
}


MacroKey::MacroKey(const MacroKey &other)
        : m_text(other.m_text)
{
    /// nothing
}

MacroKey::MacroKey(const QString &text)
        : m_text(text)
{
    /// nothing
}

void MacroKey::setText(const QString &text)
{
    m_text = text;
}

QString MacroKey::text() const
{
    return m_text;
}

bool MacroKey::isValid()
{
    const QString t = text();
    static const QRegularExpression validMacroKey(QStringLiteral("^[a-z][-.:/+_a-z0-9]*$|^[0-9]+$"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = validMacroKey.match(t);
    return match.hasMatch() && match.captured(0) == t;
}

void MacroKey::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    if (replaceMode == ValueItem::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::CompleteMatch && m_text == before)
        m_text = after;
}

bool MacroKey::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).remove(ignoredInSorting);
    return text.contains(pattern, caseSensitive);
}

bool MacroKey::operator==(const ValueItem &other) const
{
    const MacroKey *otherMacroKey = dynamic_cast<const MacroKey *>(&other);
    if (otherMacroKey != nullptr) {
        return otherMacroKey->text() == text();
    } else
        return false;
}

bool MacroKey::isMacroKey(const ValueItem &other) {
    return typeid(other) == typeid(MacroKey);
}

QDebug operator<<(QDebug dbg, const MacroKey &macrokey) {
    dbg.nospace() << "MacroKey " << macrokey.text();
    return dbg;
}


PlainText::PlainText(const PlainText &other)
        : m_text(other.text())
{
    /// nothing
}

PlainText::PlainText(const QString &text)
        : m_text(text)
{
    /// nothing
}

void PlainText::setText(const QString &text)
{
    m_text = text;
}

QString PlainText::text() const
{
    return m_text;
}

void PlainText::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    if (replaceMode == ValueItem::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::CompleteMatch && m_text == before)
        m_text = after;
}

bool PlainText::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).remove(ignoredInSorting);
    return text.contains(pattern, caseSensitive);
}

bool PlainText::operator==(const ValueItem &other) const
{
    const PlainText *otherPlainText = dynamic_cast<const PlainText *>(&other);
    if (otherPlainText != nullptr) {
        return otherPlainText->text() == text();
    } else
        return false;
}

bool PlainText::isPlainText(const ValueItem &other) {
    return typeid(other) == typeid(PlainText);
}

QDebug operator<<(QDebug dbg, const PlainText &plainText) {
    dbg.nospace() << "PlainText " << plainText.text();
    return dbg;
}


#ifdef HAVE_KF5
bool VerbatimText::colorLabelPairsInitialized = false;
QList<VerbatimText::ColorLabelPair> VerbatimText::colorLabelPairs = QList<VerbatimText::ColorLabelPair>();
#endif // HAVE_KF5

VerbatimText::VerbatimText(const VerbatimText &other)
        : m_text(other.text())
{
    /// nothing
}

VerbatimText::VerbatimText(const QString &text)
        : m_text(text)
{
    /// nothing
}

void VerbatimText::setText(const QString &text)
{
    m_text = text;
}

QString VerbatimText::text() const
{
    return m_text;
}

void VerbatimText::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    if (replaceMode == ValueItem::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::CompleteMatch && m_text == before)
        m_text = after;
}

bool VerbatimText::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).remove(ignoredInSorting);

#ifdef HAVE_KF5
    /// Initialize map of labels to color (hex string) only once
    // FIXME if user changes colors/labels later, it will not be updated here
    if (!colorLabelPairsInitialized) {
        colorLabelPairsInitialized = true;

        /// Read data from config file
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, Preferences::groupColor);
        const QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
        const QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultColorLabels);

        /// Translate data from config file into internal mapping
        for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
            ColorLabelPair clp;
            clp.hexColor = *itc;
            clp.label = i18n((*itl).toUtf8().constData());
            colorLabelPairs << clp;
        }
    }
#endif //  HAVE_KF5

    bool contained = text.contains(pattern, caseSensitive);
#ifdef HAVE_KF5
    if (!contained) {
        /// Only if simple text match failed, check color labels
        /// For a match, the user's pattern has to be the start of the color label
        /// and this verbatim text has to contain the color as hex string
        for (const auto &clp : const_cast<const QList<ColorLabelPair> &>(colorLabelPairs)) {
            contained = text.compare(clp.hexColor, Qt::CaseInsensitive) == 0 && clp.label.contains(pattern, Qt::CaseInsensitive);
            if (contained) break;
        }
    }
#endif // HAVE_KF5

    return contained;
}

bool VerbatimText::operator==(const ValueItem &other) const
{
    const VerbatimText *otherVerbatimText = dynamic_cast<const VerbatimText *>(&other);
    if (otherVerbatimText != nullptr) {
        return otherVerbatimText->text() == text();
    } else
        return false;
}

bool VerbatimText::isVerbatimText(const ValueItem &other) {
    return typeid(other) == typeid(VerbatimText);
}

QDebug operator<<(QDebug dbg, const VerbatimText &verbatimText) {
    dbg.nospace() << "VerbatimText " << verbatimText.text();
    return dbg;
}


Value::Value()
        : QVector<QSharedPointer<ValueItem> >()
{
    /// nothing
}

Value::Value(const Value &other)
        : QVector<QSharedPointer<ValueItem> >(other)
{
    /// nothing
}

Value::Value(Value &&other)
        : QVector<QSharedPointer<ValueItem> >(other)
{
    /// nothing
}

Value::~Value()
{
    clear();
}

void Value::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    QSet<QSharedPointer<ValueItem> > unique;
    /// Delegate the replace operation to each individual ValueItem
    /// contained in this Value object
    for (Value::Iterator it = begin(); it != end();) {
        (*it)->replace(before, after, replaceMode);

        bool containedInUnique = false;
        for (const auto &valueItem : const_cast<const QSet<QSharedPointer<ValueItem> > &>(unique)) {
            containedInUnique = *valueItem.data() == *(*it).data();
            if (containedInUnique) break;
        }

        if (containedInUnique)
            it = erase(it);
        else {
            unique.insert(*it);
            ++it;
        }
    }

    QSet<QString> uniqueValueItemTexts;
    for (int i = count() - 1; i >= 0; --i) {
        at(i)->replace(before, after, replaceMode);
        const QString valueItemText = PlainTextValue::text(*at(i).data());
        if (uniqueValueItemTexts.contains(valueItemText)) {
            /// Due to a replace/delete operation above, an old ValueItem's text
            /// matches the replaced text.
            /// Therefore, remove the replaced text to avoid duplicates
            remove(i);
            ++i; /// compensate for for-loop's --i
        } else
            uniqueValueItemTexts.insert(valueItemText);
    }
}

void Value::replace(const QString &before, const QSharedPointer<ValueItem> &after)
{
    const QString valueText = PlainTextValue::text(*this);
    if (valueText == before) {
        clear();
        append(after);
    } else {
        QSet<QString> uniqueValueItemTexts;
        for (int i = count() - 1; i >= 0; --i) {
            QString valueItemText = PlainTextValue::text(*at(i).data());
            if (valueItemText == before) {
                /// Perform replacement operation
                QVector<QSharedPointer<ValueItem> >::replace(i, after);
                valueItemText = PlainTextValue::text(*after.data());
                //  uniqueValueItemTexts.insert(PlainTextValue::text(*after.data()));
            }

            if (uniqueValueItemTexts.contains(valueItemText)) {
                /// Due to a previous replace operation, an existingValueItem's
                /// text matches a text which was inserted as an "after" ValueItem.
                /// Therefore, remove the old ValueItem to avoid duplicates.
                remove(i);
            } else {
                /// Neither a replacement, nor a duplicate. Keep this
                /// ValueItem (memorize as unique) and continue.
                uniqueValueItemTexts.insert(valueItemText);
            }
        }
    }
}

bool Value::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    for (const auto &valueItem : const_cast<const Value &>(*this)) {
        if (valueItem->containsPattern(pattern, caseSensitive))
            return true;
    }
    return false;
}

bool Value::contains(const ValueItem &item) const
{
    for (const auto &valueItem : const_cast<const Value &>(*this))
        if (valueItem->operator==(item))
            return true;
    return false;
}

Value &Value::operator=(const Value &rhs)
{
    return static_cast<Value &>(QVector<QSharedPointer<ValueItem> >::operator =((rhs)));
}

Value &Value::operator=(Value &&rhs)
{
    return static_cast<Value &>(QVector<QSharedPointer<ValueItem> >::operator =((rhs)));
}

Value &Value::operator<<(const QSharedPointer<ValueItem> &value)
{
    return static_cast<Value &>(QVector<QSharedPointer<ValueItem> >::operator<<((value)));
}

bool Value::operator==(const Value &rhs) const
{
    const Value &lhs = *this; ///< just for readability to have a 'lhs' matching 'rhs'

    /// Obviously, both Values must be of same size
    if (lhs.count() != rhs.count()) return false;

    /// Synchronously iterate over both Values' ValueItems
    for (Value::ConstIterator lhsIt = lhs.constBegin(), rhsIt = rhs.constBegin(); lhsIt != lhs.constEnd() && rhsIt != rhs.constEnd(); ++lhsIt, ++rhsIt) {
        /// Are both ValueItems PlainTexts and are both PlainTexts equal?
        const QSharedPointer<PlainText> lhsPlainText = lhsIt->dynamicCast<PlainText>();
        const QSharedPointer<PlainText> rhsPlainText = rhsIt->dynamicCast<PlainText>();
        if (!lhsPlainText.isNull() && !rhsPlainText.isNull() && *lhsPlainText.data() != *rhsPlainText.data())
            return false;
        else {
            /// Remainder of comparisons is like for PlainText above, just for other descendants of ValueItem
            const QSharedPointer<MacroKey> lhsMacroKey = lhsIt->dynamicCast<MacroKey>();
            const QSharedPointer<MacroKey> rhsMacroKey = rhsIt->dynamicCast<MacroKey>();
            if (!lhsMacroKey.isNull() && !rhsMacroKey.isNull() && *lhsMacroKey.data() != *rhsMacroKey.data())
                return false;
            else {
                const QSharedPointer<Person> lhsPerson = lhsIt->dynamicCast<Person>();
                const QSharedPointer<Person> rhsPerson = rhsIt->dynamicCast<Person>();
                if (!lhsPerson.isNull() && !rhsPerson.isNull() && *lhsPerson.data() != *rhsPerson.data())
                    return false;
                else {
                    const QSharedPointer<VerbatimText> lhsVerbatimText = lhsIt->dynamicCast<VerbatimText>();
                    const QSharedPointer<VerbatimText> rhsVerbatimText = rhsIt->dynamicCast<VerbatimText>();
                    if (!lhsVerbatimText.isNull() && !rhsVerbatimText.isNull() && *lhsVerbatimText.data() != *rhsVerbatimText.data())
                        return false;
                    else {
                        const QSharedPointer<Keyword> lhsKeyword = lhsIt->dynamicCast<Keyword>();
                        const QSharedPointer<Keyword> rhsKeyword = rhsIt->dynamicCast<Keyword>();
                        if (!lhsKeyword.isNull() && !rhsKeyword.isNull() && *lhsKeyword.data() != *rhsKeyword.data())
                            return false;
                        else {
                            /// If there are other descendants of ValueItem, add tests here ...
                        }
                    }
                }
            }
        }
    }

    /// No check failed, so equalness is proven
    return true;
}

bool Value::operator!=(const Value &rhs) const
{
    return !operator ==(rhs);
}

QDebug operator<<(QDebug dbg, const Value &value) {
    dbg.nospace() << "Value";
    if (value.isEmpty())
        dbg << " is empty";
    else
        dbg.nospace() << ": " << PlainTextValue::text(value);
    return dbg;
}


QString PlainTextValue::text(const Value &value)
{
    ValueItemType vit = VITOther;
    ValueItemType lastVit = VITOther;

    QString result;
    for (const auto &valueItem : value) {
        QString nextText = text(*valueItem, vit);
        if (!nextText.isEmpty()) {
            if (lastVit == VITPerson && vit == VITPerson)
                result.append(i18n(" and ")); // TODO proper list of authors/editors, not just joined by "and"
            else if (lastVit == VITPerson && vit == VITOther && nextText == QStringLiteral("others")) {
                /// "and others" case: replace text to be appended by translated variant
                nextText = i18n(" and others");
            } else if (lastVit == VITKeyword && vit == VITKeyword)
                result.append("; ");
            else if (!result.isEmpty())
                result.append(" ");
            result.append(nextText);

            lastVit = vit;
        }
    }
    return result;
}

QString PlainTextValue::text(const QSharedPointer<const ValueItem> &valueItem)
{
    const ValueItem *p = valueItem.data();
    return text(*p);
}

QString PlainTextValue::text(const ValueItem &valueItem)
{
    ValueItemType vit;
    return text(valueItem, vit);
}

QString PlainTextValue::text(const ValueItem &valueItem, ValueItemType &vit)
{
    QString result;
    vit = VITOther;

#ifdef HAVE_KF5
    if (notificationListener == nullptr)
        notificationListener = new PlainTextValue();
#endif // HAVE_KF5

    bool isVerbatim = false;
    const PlainText *plainText = dynamic_cast<const PlainText *>(&valueItem);
    if (plainText != nullptr) {
        result = plainText->text();
    } else {
        const MacroKey *macroKey = dynamic_cast<const MacroKey *>(&valueItem);
        if (macroKey != nullptr) {
            result = macroKey->text(); // TODO Use File to resolve key to full text
        } else {
            const Person *person = dynamic_cast<const Person *>(&valueItem);
            if (person != nullptr) {
                result = Person::transcribePersonName(person, personNameFormatting);
                vit = VITPerson;
            } else {
                const Keyword *keyword = dynamic_cast<const Keyword *>(&valueItem);
                if (keyword != nullptr) {
                    result = keyword->text();
                    vit = VITKeyword;
                } else {
                    const VerbatimText *verbatimText = dynamic_cast<const VerbatimText *>(&valueItem);
                    if (verbatimText != nullptr) {
                        result = verbatimText->text();
                        isVerbatim = true;
                    } else
                        qWarning() << "Cannot interpret ValueItem to one of its descendants";
                }
            }
        }
    }

    /// clean up result string
    const int len = result.length();
    int j = 0;
    static const QChar cbo = QLatin1Char('{'), cbc = QLatin1Char('}'), bs = QLatin1Char('\\'), mns = QLatin1Char('-'), comma = QLatin1Char(','), thinspace = QChar(0x2009), tilde = QLatin1Char('~'), nobreakspace = QChar(0x00a0);
    for (int i = 0; i < len; ++i) {
        if ((result[i] == cbo || result[i] == cbc) && (i < 1 || result[i - 1] != bs)) {
            /// hop over curly brackets
        } else if (i < len - 1 && result[i] == bs && result[i + 1] == mns) {
            /// hop over hyphenation commands
            ++i;
        } else if (i < len - 1 && result[i] == bs && result[i + 1] == comma) {
            /// place '\,' with a thin space
            result[j] = thinspace;
            ++i; ++j;
        } else if (!isVerbatim && result[i] == tilde && (i < 1 || result[i - 1] != bs))  {
            /// replace '~' with a non-breaking space,
            /// except if text was verbatim (e.g. a 'localfile' value
            /// like '~/document.pdf' or 'document.pdf~')
            result[j] = nobreakspace;
            ++j;
        } else {
            if (i > j) {
                /// move individual characters forward in result string
                result[j] = result[i];
            }
            ++j;
        }
    }
    result.resize(j);

    return result;
}

#ifdef HAVE_KF5
PlainTextValue::PlainTextValue()
{
    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    readConfiguration();
}

void PlainTextValue::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged)
        readConfiguration();
}

void PlainTextValue::readConfiguration()
{
    KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
    KConfigGroup configGroup(config, "General");
    personNameFormatting = configGroup.readEntry(Preferences::keyPersonNameFormatting, Preferences::defaultPersonNameFormatting);
}

PlainTextValue *PlainTextValue::notificationListener = nullptr;
QString PlainTextValue::personNameFormatting;
#else // HAVE_KF5
const QString PlainTextValue::personNameFormatting = QStringLiteral("<%l><, %s><, %f>");
#endif // HAVE_KF5
