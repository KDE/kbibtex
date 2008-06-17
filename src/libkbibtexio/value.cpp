/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QDebug>

#include "value.h"

using namespace KBibTeX::IO;

ValueTextInterface::ValueTextInterface(const ValueTextInterface* other)
        : m_text(other->text())
{
// nothing
}

ValueTextInterface::ValueTextInterface(const QString& text) : m_text(text)
{
// nothing
}

void ValueTextInterface::setText(const QString& text)
{
    m_text = text;
}

QString ValueTextInterface::text() const
{
    return m_text;
}

QString ValueTextInterface::simplifiedText() const
{
    return text().replace(QRegExp("\\\\[A-Za-z0-9]+"), "").replace('{', "").replace('}', "");
}

void ValueTextInterface::replace(const QString &before, const QString &after)
{
    if (before == text() || before == simplifiedText())
        setText(after);
}

bool ValueTextInterface::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive)
{
    return text().contains(pattern, caseSensitive) || simplifiedText().contains(pattern, caseSensitive);
}

ValueItem::ValueItem(const QString& text)
        : ValueTextInterface(text)
{
// nothing
}

Keyword::Keyword(Keyword *other)
        : ValueTextInterface(other)
{
// nothing
}

Keyword::Keyword(const QString& text)
        : ValueTextInterface(text)
{
// nothing
}

Keyword *Keyword::clone() const
{
    return new Keyword(text());
}

KeywordContainer::KeywordContainer()
        : ValueItem("")
{
// nothing
}

KeywordContainer::KeywordContainer(const QString& text)
        : ValueItem(text)
{
    setText(text);
}

KeywordContainer::KeywordContainer(const KeywordContainer *other)
        : ValueItem(QString::null)
{
    for (QLinkedList<Keyword*>::ConstIterator it = other->keywords.constBegin(); it != other->keywords.constEnd(); ++it)
        keywords.append((*it)->clone());
}

KeywordContainer::KeywordContainer(const QStringList& list)
        : ValueItem(QString::null)
{
    setList(list);
}

ValueItem *KeywordContainer::clone() const
{
    return new KeywordContainer(this);
}

void KeywordContainer::setList(const QStringList& list)
{
    keywords.clear();
    for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
        keywords.append(new Keyword(*it));
}

void KeywordContainer::append(const QString& text)
{
    bool contains = FALSE;

    for (QLinkedList<Keyword*>::ConstIterator it = keywords.begin(); !contains && it != keywords.end(); ++it)
        contains = QString::compare((*it)->text(), text) == 0;

    if (contains == 0)
        keywords.append(new Keyword(text));
}

void KeywordContainer::remove(const QString& text)
{
    bool contains = FALSE;
    for (QLinkedList<Keyword*>::Iterator it = keywords.begin(); !contains && it != keywords.end(); ++it)
        if ((*it)->text() == text) {
            keywords.erase(it);
            break;
        }
}

void KeywordContainer::setText(const QString& text)
{
    ValueItem::setText(text);

    QRegExp splitRegExp;
    if (text.contains(";"))
        splitRegExp = QRegExp("\\s*;\\s*");
    else
        splitRegExp = QRegExp("\\s*,\\s*");

    keywords.clear();
    QStringList keywordList = text.split(splitRegExp, QString::SkipEmptyParts);
    for (QStringList::ConstIterator it = keywordList.begin(); it != keywordList.end(); ++it)
        keywords.append(new Keyword(*it));
}

QString KeywordContainer::text() const
{
    QString result;
    bool first = true;
    for (QLinkedList<Keyword*>::ConstIterator it = keywords.begin(); it != keywords.end(); ++it) {
        if (!first)
            result.append("; ");
        else first = false;
        result.append((*it)->text());
    }
    return result;
}

void KeywordContainer::replace(const QString &before, const QString &after)
{
    for (QLinkedList<Keyword*>::ConstIterator it = keywords.begin(); it != keywords.end(); ++it)
        (*it)->replace(before, after);
}

Person::Person(const QString& text)
        : ValueTextInterface(text)
{
    setText(text);
}

Person::Person(const QString& firstName, const QString& lastName)
        : ValueTextInterface(firstName + " " + lastName), m_firstName(firstName), m_lastName(lastName)
{
// nothing
}

Person *Person::clone() const
{
    return new Person(m_firstName, m_lastName);
}

void Person::setText(const QString& text)
{
    ValueTextInterface::setText(text);

    QStringList segments;
    bool containsComma = splitName(text, segments);
    m_firstName = "";
    m_lastName = "";

    if (segments.isEmpty())
        return;

    if (!containsComma) {
        QString lastItem = segments[segments.count() - 1];
        int singleCapitalLettersCounter = 0;
        int p = segments.count() - 1;
        while (segments[p].length() == 1 && segments[p].compare(segments[p].toUpper()) == 0) {
            --p;
            ++singleCapitalLettersCounter;
        }

        if (singleCapitalLettersCounter > 0) {
            // this is a special case for names from PubMed, which are formatted like "Fischer T"
            for (int i = 0; i < p; ++i)
                m_lastName.append(segments[i]).append(" ");
            m_lastName.append(segments[p]);
            for (int i = p + 1; i < segments.count() - 1; ++i)
                m_firstName.append(segments[i]).append(" ");
            m_firstName.append(segments[segments.count() - 1]);
        } else {
            int from = segments.count() - 1;
            m_lastName = segments[from];
            /** check for lower case parts of the last name such as "van", "von", "de", ... */
            while (from > 0) {
                if (segments[from - 1].compare(segments[from - 1].toLower()) != 0)
                    break;
                --from;
                m_lastName.prepend(" ");
                m_lastName.prepend(segments[from]);
            }

            if (from > 0) {
                /** there is something left for the first name */
                m_firstName = *segments.begin();
                for (QStringList::Iterator it = ++segments.begin(); from > 1; ++it, --from) {
                    m_firstName.append(" ");
                    m_firstName.append(*it);
                }
            }
        }
    } else {
        bool inLastName = TRUE;
        for (int i = 0; i < segments.count(); ++i) {
            if (segments[i] == ",")
                inLastName = FALSE;
            else if (inLastName) {
                if (!m_lastName.isEmpty()) m_lastName.append(" ");
                m_lastName.append(segments[i]);
            } else {
                if (!m_firstName.isEmpty()) m_firstName.append(" ");
                m_firstName.append(segments[i]);
            }
        }
    }
}

QString Person::text(bool firstNameFirst) const
{

    if (m_firstName.isEmpty())
        return m_lastName;
    else
        return firstNameFirst ? m_firstName + " " + m_lastName : m_lastName + ", " + m_firstName;
}

QString Person::firstName()
{
    return m_firstName;
}

QString Person::lastName()
{
    return m_lastName;
}

/** Splits a name into single words. If the name's text was reversed (Last, First), the result will be true and the comma will be added to segments. Otherwise the functions result will be false. This function respects protecting {...}. */
bool Person::splitName(const QString& text, QStringList& segments)
{
    int bracketCounter = 0;
    bool result = FALSE;
    QString buffer = "";

    for (int pos = 0; pos < text.length(); ++pos) {
        if (text[pos] == '{')
            ++bracketCounter;
        else if (text[pos] == '}')
            --bracketCounter;

        if (text[pos] == ' ' && bracketCounter == 0) {
            if (!buffer.isEmpty()) {
                segments.append(buffer);
                buffer = "";
            }
        } else if (text[pos] == ',' && bracketCounter == 0) {
            if (!buffer.isEmpty()) {
                segments.append(buffer);
                buffer = "";
            }
            segments.append(",");
            result = TRUE;
        } else
            buffer.append(text[pos]);
    }

    if (!buffer.isEmpty())
        segments.append(buffer);

    return result;
}

PersonContainer::PersonContainer()
        : ValueItem(QString::null)
{
    // nothing
}

PersonContainer::PersonContainer(const QString& text)
        : ValueItem(text)
{
    persons.append(new Person(text));
}

ValueItem *PersonContainer::clone() const
{
    PersonContainer *result = new PersonContainer();
    for (QLinkedList<Person*>::ConstIterator it = persons.begin(); it != persons.end(); ++it)
        result->persons.append((*it)->clone());

    return result;
}

void PersonContainer::setText(const QString& text)
{
    ValueTextInterface::setText(text);
    qWarning() << "You cannot set a text ('" << text << "') to a PersonContainer object";
}

QString PersonContainer::text() const
{
    QString result;
    bool first = TRUE;

    for (QLinkedList<Person*>::ConstIterator it = persons.begin(); it != persons.end(); ++it) {
        if (!first)
            result.append(" and ");
        else
            first = FALSE;
        result.append((*it)->text());
    }

    return result;
}

void PersonContainer::replace(const QString &before, const QString &after)
{
    for (QLinkedList<Person*>::ConstIterator it = persons.begin(); it != persons.end(); ++it)
        (*it)->replace(before, after);
}

MacroKey::MacroKey(const QString& text)
        : ValueItem(text)
{
    m_isValid = isValidInternal();
}

ValueItem *MacroKey::clone() const
{
    return new MacroKey(text());
}

void MacroKey::setText(const QString& text)
{
    ValueItem::setText(text);
    m_isValid = isValidInternal();
}

bool MacroKey::isValid()
{
    return m_isValid;
}

bool MacroKey::isValidInternal()
{
    return !text().contains(QRegExp("![-.:/+_a-zA-Z0-9]"));
}

PlainText::PlainText(const QString& text)
        : ValueItem(text)
{
    // nothing
}

ValueItem *PlainText::clone() const
{
    return new PlainText(text());
}

Value::Value()
        : ValueTextInterface(QString::null)
{
    // nothing
}

Value::Value(const Value *other)
        : ValueTextInterface(other)
{
    for (QLinkedList<ValueItem*>::ConstIterator it = other->items.begin(); it != other->items.end(); ++it)
        items.append((*it)->clone());
}

Value::Value(const QString& text, bool isMacroKey)
        : ValueTextInterface(text)
{
    ValueItem *item = NULL;
    if (isMacroKey)
        item = new MacroKey(text);
    else
        item = new PlainText(text);
    items.append(item);
}

void Value::setText(const QString& text)
{
    ValueTextInterface::setText(text);
    qWarning() << "You cannot set a text ('" << text << "') to a Value object";
}

QString Value::text() const
{
    QString result;

    for (QLinkedList<ValueItem*>::ConstIterator it = items.begin(); it != items.end(); ++it)
        result.append((*it)->text());

    return result;
}

void Value::replace(const QString &before, const QString &after)
{
    for (QLinkedList<ValueItem*>::ConstIterator it = items.begin(); it != items.end(); ++it)
        (*it)->replace(before, after);
}
