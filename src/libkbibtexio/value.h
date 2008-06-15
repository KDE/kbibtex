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
#ifndef BIBTEXVALUE_H
#define BIBTEXVALUE_H

#include <QLinkedList>

namespace KBibTeX
{
namespace IO {

class ValueTextInterface
{
public:
    ValueTextInterface(const ValueTextInterface* other);
    ValueTextInterface(const QString& text);
    virtual ~ValueTextInterface() {};

    virtual void setText(const QString& text);
    virtual QString text() const;
    QString simplifiedText() const;
    virtual void replace(const QString &before, const QString &after);
    virtual bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive);

private:
    QString m_text;
};


class ValueItem: public ValueTextInterface
{
public:
    ValueItem(const QString& text);

    virtual ValueItem *clone() const {
        return NULL;
    };
};


class Keyword: public ValueTextInterface
{
public:
    Keyword(Keyword *other);
    Keyword(const QString& text);

    Keyword *clone() const;
};

class KeywordContainer: public ValueItem
{
public:
    KeywordContainer();
    KeywordContainer(const QString& text);
    KeywordContainer(const KeywordContainer *other);
    KeywordContainer(const QStringList& list);

    ValueItem *clone() const;
    void setList(const QStringList& list);
    void append(const QString& text);
    void remove(const QString& text);
    void setText(const QString& text);
    QString text() const;
    void replace(const QString &before, const QString &after);

    QLinkedList<Keyword*> keywords;
};

class Person: public ValueTextInterface
{
public:
    Person(const QString& text);
    Person(const QString& firstName, const QString& lastName);

    Person *clone() const;
    void setText(const QString& text);
    QString text(bool firstNameFirst = false) const;

    QString firstName();
    QString lastName();

protected:
    QString m_firstName;
    QString m_lastName;

    bool splitName(const QString& text, QStringList& segments);
};

class PersonContainer: public ValueItem
{
public:
    PersonContainer();
    PersonContainer(const QString& text);

    ValueItem *clone() const;
    void setText(const QString& text);
    QString text() const;
    void replace(const QString &before, const QString &after);

    QLinkedList<Person*> persons;
};

class MacroKey: public ValueItem
{
private:
    bool m_isValid;
    bool isValidInternal();

public:
    MacroKey(const QString& text);

    ValueItem *clone() const;

    void setText(const QString& text);
    bool isValid();
};

class PlainText: public ValueItem
{
public:
    PlainText(const QString& text);

    ValueItem *clone() const;
};

class Value: public ValueTextInterface
{
public:
    Value();
    Value(const Value *other);
    Value(const QString& text, bool isMacroKey = false);

    void setText(const QString& text);
    QString text() const;
    void replace(const QString &before, const QString &after);

    QLinkedList<ValueItem*> items;
};

}
}

#endif
