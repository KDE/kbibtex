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

#include <file.h>
#include "value.h"

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
    return m_text.contains(pattern, caseSensitive);
}


Person::Person(const QString& firstName, const QString& lastName, const QString& prefix, const QString& suffix)
        : m_firstName(firstName), m_lastName(lastName), m_prefix(prefix), m_suffix(suffix)
{
    // nothing
}

Person::Person(const Person& other)
        : m_firstName(other.firstName()), m_lastName(other.lastName()), m_prefix(other.prefix()), m_suffix(other.suffix())
{
    // nothing
}

void Person::setName(const QString& firstName, const QString& lastName, const QString& prefix, const QString& suffix)
{
    m_firstName = firstName;
    m_lastName = lastName;
    m_prefix = prefix;
    m_suffix = suffix;
}

QString Person::firstName() const
{
    return m_firstName;
}

QString Person::lastName() const
{
    return m_lastName;
}

QString Person::prefix() const
{
    return m_prefix;
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
    if (m_prefix == before)
        m_prefix = after;
    if (m_suffix == before)
        m_suffix = after;
}

bool Person::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    return m_firstName.contains(pattern, caseSensitive) || m_lastName.contains(pattern, caseSensitive) ||  m_prefix.contains(pattern, caseSensitive) ||  m_suffix.contains(pattern, caseSensitive) || QString("%1 %2|%2, %1").arg(m_firstName).arg(m_lastName).contains(pattern, caseSensitive);
}


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
    return m_text.contains(pattern, caseSensitive);
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
    return m_text.contains(pattern, caseSensitive);
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
    return m_text.contains(pattern, caseSensitive);
}


Value::Value()
        : QList<ValueItem*>()
{
    // nothing
}

Value::Value(const Value& other)
        : QList<ValueItem*>()
{
    copyFrom(other);
}

void Value::replace(const QString &before, const QString &after)
{
    for (QList<ValueItem*>::Iterator it = begin(); it != end(); ++it)
        (*it)->replace(before, after);
}

bool Value::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    bool result = false;
    for (QList<ValueItem*>::ConstIterator it = begin(); !result && it != end(); ++it) {
        result |= (*it)->containsPattern(pattern, caseSensitive);
    }
    return result;
}

Value& Value::operator=(const Value & rhs)
{
    copyFrom(rhs);
    return *this;
}

void Value::copyFrom(const Value& other)
{
    clear();
    for (QList<ValueItem*>::ConstIterator it = other.begin(); it != other.end(); ++it) {
        PlainText *plainText = dynamic_cast<PlainText*>(*it);
        if (plainText != NULL)
            append(new PlainText(*plainText));
        else {
            Person *person = dynamic_cast<Person*>(*it);
            if (person != NULL)
                append(new Person(*person));
            else {
                Keyword *keyword = dynamic_cast<Keyword*>(*it);
                if (keyword != NULL)
                    append(new Keyword(*keyword));
                else {
                    MacroKey *macroKey = dynamic_cast<MacroKey*>(*it);
                    if (macroKey != NULL)
                        append(new MacroKey(*macroKey));
                    else {
                        VerbatimText *verbatimText = dynamic_cast<VerbatimText*>(*it);
                        if (verbatimText != NULL)
                            append(new VerbatimText(*verbatimText));
                        else
                            kError() << "cannot copy from unknown data type" << endl;
                    }
                }
            }
        }
    }
}

QRegExp PlainTextValue::removeCurlyBrackets = QRegExp("(^|[^\\\\])[{}]");

QString PlainTextValue::text(const Value& value, const File* file, bool debug)
{
    ValueItemType vit = VITOther;
    ValueItemType lastVit = VITOther;

    QString result = "";
    for (QList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
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
                result = person->firstName() + " " + person->lastName();
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

    int i = -1;
    while ((i = result.indexOf(removeCurlyBrackets, i + 1)) >= 0) {
        result = result.replace(removeCurlyBrackets.cap(0), removeCurlyBrackets.cap(1));
    }

    if (debug) result = "[:" + result + ":Debug]";
    return result;
}
