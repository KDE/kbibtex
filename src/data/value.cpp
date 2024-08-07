/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QRegularExpression>

#ifdef HAVE_KF
#include <KSharedConfig>
#include <KConfigGroup>
#endif // HAVE_KF
#ifdef HAVE_KFI18N
#include <KLocalizedString>
#else // HAVE_KFI18N
#include <QObject>
#define i18n(text) QObject::tr(text)
#endif // HAVE_KFI18N

#include <Preferences>
#include "logging_data.h"

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
    if (replaceMode == ValueItem::ReplaceMode::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::ReplaceMode::CompleteMatch && m_text == before)
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
    if (replaceMode == ValueItem::ReplaceMode::AnySubstring) {
        m_firstName = m_firstName.replace(before, after);
        m_lastName = m_lastName.replace(before, after);
        m_suffix = m_suffix.replace(before, after);
    } else if (replaceMode == ValueItem::ReplaceMode::CompleteMatch) {
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
    while ((p1 = result.indexOf(QLatin1Char('<'))) >= 0 && (p2 = result.indexOf(QLatin1Char('>'), p1 + 1)) >= 0 && (p3 = result.indexOf(QLatin1Char('%'), p1)) >= 0 && p3 < p2) {
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

QDebug operator<<(QDebug dbg, const Person *person) {
    dbg.nospace() << "Person " << Person::transcribePersonName(person, Preferences::defaultPersonNameFormat);
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
    if (replaceMode == ValueItem::ReplaceMode::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::ReplaceMode::CompleteMatch && m_text == before)
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
    if (replaceMode == ValueItem::ReplaceMode::AnySubstring)
        m_text = m_text.replace(before, after);
    else if (replaceMode == ValueItem::ReplaceMode::CompleteMatch && m_text == before)
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


VerbatimText::VerbatimText(const VerbatimText &other)
        : m_hasComment(other.hasComment()), m_text(other.text()), m_comment(other.comment())
{
    /// nothing
}

VerbatimText::VerbatimText(const QString &text)
        : m_hasComment(false), m_text(text)
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

bool VerbatimText::hasComment() const
{
    return m_hasComment;
}

void VerbatimText::removeComment()
{
    m_hasComment = false;
    m_comment.clear();
}

void VerbatimText::setComment(const QString &comment)
{
    m_hasComment = true;
    m_comment = comment;
}

QString VerbatimText::comment() const
{
    return m_comment;
}

void VerbatimText::replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode)
{
    if (replaceMode == ValueItem::ReplaceMode::AnySubstring) {
        m_text = m_text.replace(before, after);
        m_comment = m_comment.replace(before, after);
    } else if (replaceMode == ValueItem::ReplaceMode::CompleteMatch) {
        if (m_text == before)
            m_text = after;
        if (m_hasComment && m_comment == before)
            m_comment = after;
    }
}

bool VerbatimText::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).remove(ignoredInSorting);

    bool contained = text.contains(pattern, caseSensitive);
    if (!contained) {
        /// Only if simple text match failed, check color labels
        /// For a match, the user's pattern has to be the start of the color label
        /// and this verbatim text has to contain the color as hex string
        for (QVector<QPair<QString, QString>>::ConstIterator it = Preferences::instance().colorCodes().constBegin(); !contained && it != Preferences::instance().colorCodes().constEnd(); ++it)
            contained = text.compare(it->first, Qt::CaseInsensitive) == 0 && it->second.contains(pattern, Qt::CaseInsensitive);
    }

    if (!contained && m_hasComment) {
        const QString comment = QString(m_comment).remove(ignoredInSorting);
        contained = comment.contains(pattern, caseSensitive);
        /// Do not test for colors in comment
    }

    return contained;
}

bool VerbatimText::operator==(const ValueItem &other) const
{
    const VerbatimText *otherVerbatimText = dynamic_cast<const VerbatimText *>(&other);
    if (otherVerbatimText != nullptr) {
        return otherVerbatimText->text() == text() && (!m_hasComment || otherVerbatimText->comment() == comment());
    } else
        return false;
}

bool VerbatimText::isVerbatimText(const ValueItem &other) {
    return typeid(other) == typeid(VerbatimText);
}

QDebug operator<<(QDebug dbg, const VerbatimText &verbatimText) {
    dbg.nospace() << "VerbatimText " << verbatimText.text() << " (" << (verbatimText.hasComment() ? "" : "NO ") << "comment: " << verbatimText.comment() << ")";
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
        if ((lhsPlainText.isNull() && !rhsPlainText.isNull()) || (!lhsPlainText.isNull() && rhsPlainText.isNull())) return false;
        if (!lhsPlainText.isNull() && !rhsPlainText.isNull()) {
            if (*lhsPlainText.data() != *rhsPlainText.data())
                return false;
        } else {
            /// Remainder of comparisons is like for PlainText above, just for other descendants of ValueItem
            const QSharedPointer<MacroKey> lhsMacroKey = lhsIt->dynamicCast<MacroKey>();
            const QSharedPointer<MacroKey> rhsMacroKey = rhsIt->dynamicCast<MacroKey>();
            if ((lhsMacroKey.isNull() && !rhsMacroKey.isNull()) || (!lhsMacroKey.isNull() && rhsMacroKey.isNull())) return false;
            if (!lhsMacroKey.isNull() && !rhsMacroKey.isNull()) {
                if (*lhsMacroKey.data() != *rhsMacroKey.data())
                    return false;
            } else {
                const QSharedPointer<Person> lhsPerson = lhsIt->dynamicCast<Person>();
                const QSharedPointer<Person> rhsPerson = rhsIt->dynamicCast<Person>();
                if ((lhsPerson.isNull() && !rhsPerson.isNull()) || (!lhsPerson.isNull() && rhsPerson.isNull())) return false;
                if (!lhsPerson.isNull() && !rhsPerson.isNull()) {
                    if (*lhsPerson.data() != *rhsPerson.data())
                        return false;
                } else {
                    const QSharedPointer<VerbatimText> lhsVerbatimText = lhsIt->dynamicCast<VerbatimText>();
                    const QSharedPointer<VerbatimText> rhsVerbatimText = rhsIt->dynamicCast<VerbatimText>();
                    if ((lhsVerbatimText.isNull() && !rhsVerbatimText.isNull()) || (!lhsVerbatimText.isNull() && rhsVerbatimText.isNull())) return false;
                    if (!lhsVerbatimText.isNull() && !rhsVerbatimText.isNull()) {
                        if (*lhsVerbatimText.data() != *rhsVerbatimText.data())
                            return false;
                    } else {
                        const QSharedPointer<Keyword> lhsKeyword = lhsIt->dynamicCast<Keyword>();
                        const QSharedPointer<Keyword> rhsKeyword = rhsIt->dynamicCast<Keyword>();
                        if ((lhsKeyword.isNull() && !rhsKeyword.isNull()) || (!lhsKeyword.isNull() && rhsKeyword.isNull())) return false;
                        if (!lhsKeyword.isNull() && !rhsKeyword.isNull()) {
                            if (*lhsKeyword.data() != *rhsKeyword.data())
                                return false;
                        } else {
                            /// If there are other descendants of ValueItem, add tests here ...
                            return false;
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


QString PlainTextValue::text(const Value &value, const FormattingOptions formattingOptions)
{
    ValueItemType vit = ValueItemType::Other;
    ValueItemType lastVit = ValueItemType::Other;

    if (formattingOptions.testFlag(FormattingOption::BeautifyMonth) && value.length() >= 1 && value.length() <= 3) {
        int firstIndex = -1, lastIndex = -1;
        if (MacroKey::isMacroKey(*value.first()) && MacroKey::isMacroKey(*value.last())) {
            // Probably one macro key like  jan  or a triple like  jan "#" feb   or just  jan feb
            const QString firstText = value.first().dynamicCast<MacroKey>()->text().toLower();
            const QString lastText = value.length() > 1 ? value.last().dynamicCast<MacroKey>()->text().toLower() : QString();
            firstIndex = std::distance(KBibTeX::MonthsTriple, std::find(KBibTeX::MonthsTriple, KBibTeX::MonthsTriple + 12, firstText)) + 1;
            lastIndex = value.length() > 1 ? std::distance(KBibTeX::MonthsTriple, std::find(KBibTeX::MonthsTriple, KBibTeX::MonthsTriple + 12, lastText)) + 1 : firstIndex;
        } else {
            // Some text that may contain a number like  5
            bool ok = false;
            firstIndex = PlainTextValue::text(value.first()).toInt(&ok);
            if (!ok || (firstIndex < 1) || (firstIndex > 12))
                firstIndex = -1;
            ok = true;
            lastIndex = value.length() > 1 ? PlainTextValue::text(value.last()).toInt(&ok) : firstIndex;
            if (!ok || (lastIndex < 1) || (lastIndex > 12))
                lastIndex = -1;
            static const QSet<QString> centerOfThree{QStringLiteral("#"), QStringLiteral("-"), QStringLiteral("--"), QStringLiteral("/"), QChar(0x2013)};
            if (value.length() == 3 && !centerOfThree.contains(PlainTextValue::text(value.at(1)))) {
                // If value contains three elements, the center one must be one of the allowed strings in the set
                lastIndex = firstIndex = -1;
            }
        }
        if (firstIndex >= 1 && lastIndex <= 12 && lastIndex >= 1 && lastIndex <= 12) {
            if (value.length() == 1) {
                // Detour via format string necessary due to bug QT-113415
                return QString(QStringLiteral("%1")).arg(QLocale::system().monthName(firstIndex));
            }
            else
                return QString(QStringLiteral("%1%2%3")).arg(QLocale::system().monthName(firstIndex), QChar(0x2013), QLocale::system().monthName(lastIndex));
        } else {
            qCDebug(LOG_KBIBTEX_DATA) << "Got asked to beautify month, but could not determine index";
        }
    }

    QString result;
    for (const auto &valueItem : value) {
        QString nextText = text(*valueItem, vit);
        if (!nextText.isEmpty()) {
            if (lastVit == ValueItemType::Person && vit == ValueItemType::Person)
                result.append(i18n(" and ")); // TODO proper list of authors/editors, not just joined by "and"
            else if (lastVit == ValueItemType::Person && vit == ValueItemType::Other && nextText == QStringLiteral("others")) {
                /// "and others" case: replace text to be appended by translated variant
                nextText = i18n(" and others");
            } else if (lastVit == ValueItemType::Keyword && vit == ValueItemType::Keyword)
                result.append(QStringLiteral("; "));
            else if (!result.isEmpty())
                result.append(QStringLiteral(" "));
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
    vit = ValueItemType::Other;

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
                result = Person::transcribePersonName(person, Preferences::instance().personNameFormat());
                vit = ValueItemType::Person;
            } else {
                const Keyword *keyword = dynamic_cast<const Keyword *>(&valueItem);
                if (keyword != nullptr) {
                    result = keyword->text();
                    vit = ValueItemType::Keyword;
                } else {
                    const VerbatimText *verbatimText = dynamic_cast<const VerbatimText *>(&valueItem);
                    if (verbatimText != nullptr) {
                        result = verbatimText->text();
                        isVerbatim = true;
                    } else
                        qCWarning(LOG_KBIBTEX_DATA) << "Cannot interpret ValueItem to one of its descendants";
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
