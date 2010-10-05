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
#ifndef BIBTEXVALUE_H
#define BIBTEXVALUE_H

#include <QList>
#include <QRegExp>
#include <QVariant>

#include "kbibtexio_export.h"

class File;

/**
  * Generic class of an information element in a @see Value object.
  * In BibTeX, ValueItems are concatenated by "#".
  */
class ValueItem
{
public:
    virtual ~ValueItem() { /* nothing */ };

    virtual void replace(const QString &before, const QString &after) = 0;

    /**
      * Check if this object contains text pattern @p pattern.
      * @param pattern Pattern to check for
      * @param caseSensitive Case sensitivity setting for check
      * @return TRUE if pattern is contained within this value, otherwise FALSE
      */
    virtual bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const = 0;
};

class KBIBTEXIO_EXPORT Keyword: public ValueItem
{
public:
    ~Keyword() { /* nothing */ };

    Keyword(const Keyword& other);
    Keyword(const QString& text);

    void setText(const QString& text);
    QString text() const;

    void replace(const QString &before, const QString &after);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

protected:
    QString m_text;
};

class KBIBTEXIO_EXPORT Person: public ValueItem
{
public:
    /**
    * Create a representation for a person's name. In bibliographies, a person is either an author or an editor.
    * The four parameters cover all common parts of a name. Only first and last name are mandatory (each person should have those).
    * Example: A name like "Dr. Wernher von Braun" would be split as follows: "Wernher" is assigned to @p firstName, "von Braun" to @p lastName, and "Dr." to @p prefix.
    @param firstName First name of a person. Example: "Peter"
    @param lastName Last name of a person. Example: "Smith"
    @param prefix Prefix in front of a name. Example: "Dr."
    @param suffix Suffix after a name. Example: "jr."
    */
    Person(const QString& firstName, const QString& lastName, const QString& prefix = QString::null, const QString& suffix = QString::null);
    Person(const Person& other);

    void setName(const QString& firstName, const QString& lastName, const QString& prefix = QString::null, const QString& suffix = QString::null);
    QString firstName() const;
    QString lastName() const;
    QString prefix() const;
    QString suffix() const;

    void replace(const QString &before, const QString &after);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

private:
    QString m_firstName;
    QString m_lastName;
    QString m_prefix;
    QString m_suffix;
};

class KBIBTEXIO_EXPORT MacroKey: public ValueItem
{
public:
    MacroKey(const MacroKey& other);
    MacroKey(const QString& text);

    void setText(const QString& text);
    QString text() const;
    bool isValid();

    void replace(const QString &before, const QString &after);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

protected:
    QString m_text;
    static const QRegExp validMacroKey;
};

class KBIBTEXIO_EXPORT PlainText: public ValueItem
{
public:
    PlainText(const PlainText& other);
    PlainText(const QString& text);

    void setText(const QString& text);
    QString text() const;

    void replace(const QString &before, const QString &after);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

protected:
    QString m_text;
};

class KBIBTEXIO_EXPORT VerbatimText: public ValueItem
{
public:
    VerbatimText(const VerbatimText& other);
    VerbatimText(const QString& text);

    void setText(const QString& text);
    QString text() const;

    void replace(const QString &before, const QString &after);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

protected:
    QString m_text;
};

/**
 * Container class to hold values of BibTeX entry fields and similar value types in BibTeX file.
 * A Value object is built from a list of @see ValueItem objects.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT Value: public QList<ValueItem*>
{
public:
    Value();
    Value(const Value& other);

    void replace(const QString &before, const QString &after);

    /**
      * Check if this value contains text pattern @p pattern.
      * @param pattern Pattern to check for
      * @param caseSensitive Case sensitivity setting for check
      * @return TRUE if pattern is contained within this value, otherwise FALSE
      */
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

    Value& operator=(const Value& rhs);

private:
    void copyFrom(const Value& other);
};

class KBIBTEXIO_EXPORT PlainTextValue
{
public:
    static QString text(const Value& value, const File* file = NULL, bool debug = false);
    static QString text(const ValueItem& valueItem, const File* file = NULL, bool debug = false);

private:
    enum ValueItemType { VITOther = 0, VITPerson, VITKeyword} lastItem;
    static QRegExp removeCurlyBrackets;

    static QString text(const ValueItem& valueItem, ValueItemType &vit, const File* file, bool debug);
};

Q_DECLARE_METATYPE(Value);

#endif
