/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <QString>
#include <QStringList>

#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>

#include <file.h>
#include "value.h"

const QRegExp ValueItem::ignoredInSorting = QRegExp("[{}\\\\]+");

ValueItem::~ValueItem()
{
    // nothing
}

Keyword::Keyword(const Keyword& other)
        : m_text(other.m_text)
{
    // nothing
}

Keyword::Keyword(const QString& text)
        : m_text(text)
{
    // nothing
}

void Keyword::setText(const QString& text)
{
    m_text = text;
}

QString Keyword::text() const
{
    return m_text;
}

void Keyword::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool Keyword::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).replace(ignoredInSorting, "");
    return text.contains(pattern, caseSensitive);
}

bool Keyword::operator==(const ValueItem &other) const
{
    const Keyword *otherKeyword = dynamic_cast<const Keyword*>(&other);
    if (otherKeyword != NULL) {
        return otherKeyword->text() == text();
    } else
        return false;
}


Person::Person(const QString& firstName, const QString& lastName, const QString& suffix)
        : m_firstName(firstName), m_lastName(lastName), m_suffix(suffix)
{
    // nothing
}

Person::Person(const Person& other)
        : m_firstName(other.firstName()), m_lastName(other.lastName()), m_suffix(other.suffix())
{
    // nothing
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

void Person::replace(const QString &before, const QString &after)
{
    if (m_firstName == before)
        m_firstName = after;
    if (m_lastName == before)
        m_lastName = after;
    if (m_suffix == before)
        m_suffix = after;
}

bool Person::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString firstName = QString(m_firstName).replace(ignoredInSorting, "");
    const QString lastName = QString(m_lastName).replace(ignoredInSorting, "");
    const QString suffix = QString(m_suffix).replace(ignoredInSorting, "");

    return firstName.contains(pattern, caseSensitive) || lastName.contains(pattern, caseSensitive) || suffix.contains(pattern, caseSensitive) || QString("%1 %2|%2, %1").arg(firstName).arg(lastName).contains(pattern, caseSensitive);
}

bool Person::operator==(const ValueItem &other) const
{
    const Person *otherPerson = dynamic_cast<const Person*>(&other);
    if (otherPerson != NULL) {
        return otherPerson->firstName() == firstName() && otherPerson->lastName() == lastName();
    } else
        return false;
}

QString Person::transcribePersonName(const Person *person, const QString &formatting)
{
    return transcribePersonName(formatting, person->firstName(), person->lastName(), person->suffix());
}

QString Person::transcribePersonName(const QString &formatting, const QString& firstName, const QString& lastName, const QString& suffix)
{
    QString result = formatting;
    int p1 = -1, p2 = -1, p3 = -1;
    while ((p1 = result.indexOf('<')) >= 0 && (p2 = result.indexOf('>', p1 + 1)) >= 0 && (p3 = result.indexOf('%', p1)) >= 0 && p3 < p2) {
        QString insert;
        switch (result[p3+1].toAscii()) {
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

const QString Person::keyPersonNameFormatting = QLatin1String("personNameFormatting");
const QString Person::defaultPersonNameFormatting = QLatin1String("<%l><, %s><, %f>"); // "<%f ><%l>< %s>"


const QRegExp MacroKey::validMacroKey = QRegExp("^[a-z][-.:/+_a-z0-9]*$|^[0-9]+$", Qt::CaseInsensitive);

MacroKey::MacroKey(const MacroKey& other)
        : m_text(other.m_text)
{
    // nothing
}

MacroKey::MacroKey(const QString& text)
        : m_text(text)
{
    // nothing
}

void MacroKey::setText(const QString& text)
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
    int idx = validMacroKey.indexIn(t);
    return idx > -1 && validMacroKey.cap(0) == t;
}

void MacroKey::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool MacroKey::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).replace(ignoredInSorting, "");
    return text.contains(pattern, caseSensitive);
}

bool MacroKey::operator==(const ValueItem &other) const
{
    const MacroKey *otherMacroKey = dynamic_cast<const MacroKey*>(&other);
    if (otherMacroKey != NULL) {
        return otherMacroKey->text() == text();
    } else
        return false;
}


PlainText::PlainText(const PlainText& other)
        : m_text(other.text())
{
    // nothing
}

PlainText::PlainText(const QString& text)
        : m_text(text)
{
    // nothing
}

void PlainText::setText(const QString& text)
{
    m_text = text;
}

QString PlainText::text() const
{
    return m_text;
}

void PlainText::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool PlainText::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).replace(ignoredInSorting, "");
    return text.contains(pattern, caseSensitive);
}

bool PlainText::operator==(const ValueItem &other) const
{
    const PlainText *otherPlainText = dynamic_cast<const PlainText*>(&other);
    if (otherPlainText != NULL) {
        return otherPlainText->text() == text();
    } else
        return false;
}


VerbatimText::VerbatimText(const VerbatimText& other)
        : m_text(other.text())
{
    // nothing
}

VerbatimText::VerbatimText(const QString& text)
        : m_text(text)
{
    // nothing
}

void VerbatimText::setText(const QString& text)
{
    m_text = text;
}

QString VerbatimText::text() const
{
    return m_text;
}

void VerbatimText::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool VerbatimText::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    const QString text = QString(m_text).replace(ignoredInSorting, "");
    return text.contains(pattern, caseSensitive);
}

bool VerbatimText::operator==(const ValueItem &other) const
{
    const VerbatimText *otherVerbatimText = dynamic_cast<const VerbatimText*>(&other);
    if (otherVerbatimText != NULL) {
        return otherVerbatimText->text() == text();
    } else
        return false;
}


Value::Value()
        : QVector<QSharedPointer<ValueItem> >()
{
    // nothing
}

Value::Value(const Value& other)
        : QVector<QSharedPointer<ValueItem> >()
{
    clear();
    mergeFrom(other);
}

Value::~Value()
{
    clear();
}

void Value::merge(const Value& other)
{
    mergeFrom(other);
}

void Value::replace(const QString &before, const QString &after)
{
    for (Value::Iterator it = begin(); it != end(); ++it)
        (*it)->replace(before, after);
}

bool Value::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    bool result = false;
    for (Value::ConstIterator it = constBegin(); !result && it != constEnd(); ++it) {
        result |= (*it)->containsPattern(pattern, caseSensitive);
    }
    return result;
}

bool Value::contains(const ValueItem& item) const
{
    for (Value::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if ((*it)->operator==(item))
            return true;
    return false;
}

Value& Value::operator=(const Value & rhs)
{
    clear();
    mergeFrom(rhs);
    return *this;
}

void Value::mergeFrom(const Value& other)
{
    for (Value::ConstIterator it = other.constBegin(); it != other.constEnd(); ++it)
        append(*it);
}

QString PlainTextValue::personNameFormatting = QString::null;

QString PlainTextValue::text(const Value& value, const File* file, bool debug)
{
    ValueItemType vit = VITOther;
    ValueItemType lastVit = VITOther;

    QString result = "";
    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        QString nextText = text(**it, vit, file, debug);
        if (!nextText.isNull()) {
            if (lastVit == VITPerson && vit == VITPerson)
                result.append(" and ");
            else if (lastVit == VITKeyword && vit == VITKeyword)
                result.append("; ");
            else if (!result.isEmpty())
                result.append(" ");
            result.append(nextText);

            lastVit = vit;
        }
    }
    return result;
}

QString PlainTextValue::text(const ValueItem& valueItem, const File* file, bool debug)
{
    ValueItemType vit;
    return text(valueItem, vit, file, debug);
}

QString PlainTextValue::text(const ValueItem& valueItem, ValueItemType &vit, const File* /*file*/, bool debug)
{
    QString result = QString::null;
    vit = VITOther;

    const PlainText *plainText = dynamic_cast<const PlainText*>(&valueItem);
    if (plainText != NULL) {
        result = plainText->text();
        if (debug) result = "[:" + result + ":PlainText]";
    } else {
        const MacroKey *macroKey = dynamic_cast<const MacroKey*>(&valueItem);
        if (macroKey != NULL) {
            result = macroKey->text(); // TODO Use File to resolve key to full text
            if (debug) result = "[:" + result + ":MacroKey]";
        } else {
            const Person *person = dynamic_cast<const Person*>(&valueItem);
            if (person != NULL) {
                if (personNameFormatting.isNull()) {
                    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
                    KConfigGroup configGroup(config, "General");
                    personNameFormatting = configGroup.readEntry(Person::keyPersonNameFormatting, Person::defaultPersonNameFormatting);
                }
                result = Person::transcribePersonName(person, personNameFormatting);
                vit = VITPerson;
                if (debug) result = "[:" + result + ":Person]";
            } else {
                const Keyword *keyword = dynamic_cast<const Keyword*>(&valueItem);
                if (keyword != NULL) {
                    result = keyword->text();
                    vit = VITKeyword;
                    if (debug) result = "[:" + result + ":Keyword]";
                } else {
                    const VerbatimText *verbatimText = dynamic_cast<const VerbatimText*>(&valueItem);
                    if (verbatimText != NULL) {
                        result = verbatimText->text();
                        if (debug) result = "[:" + result + ":VerbatimText]";
                    }
                }
            }
        }
    }

    /// clean up result string
    const int len = result.length();
    int j = 0;
    static const QChar cbo = QChar('{'), cbc = QChar('}'), bs = QChar('\\'), mns = QChar('-');
    for (int i = 0; i < len; ++i) {
        if ((result[i] == cbo || result[i] == cbc) && (i < 1 || result[i-1] != bs)) {
            /// hop over curly brackets
        } else if (i < len - 1 && result[i] == bs && result[i+1] == mns) {
            /// hop over hyphenation commands
            ++i;
        } else {
            if (i > j) {
                /// move individual characters forward in result string
                result[j] = result[i];
            }
            ++j;
        }
    }
    result.resize(j);

    if (debug) result = "[:" + result + ":Debug]";
    return result;
}
