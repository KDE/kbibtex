/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#ifndef BIBTEXVALUE_H
#define BIBTEXVALUE_H

#include <QVector>
#include <QRegExp>
#include <QVariant>
#include <QSharedPointer>

#include "kbibtexdata_export.h"

#include "notificationhub.h"

class File;

/**
  * Generic class of an information element in a @see Value object.
  * In BibTeX, ValueItems are concatenated by "#".
  */
class KBIBTEXDATA_EXPORT ValueItem
{
public:
    enum ReplaceMode {CompleteMatch, AnySubstring};

    ValueItem();
    virtual ~ValueItem();

    virtual void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode) = 0;

    /**
      * Check if this object contains text pattern @p pattern.
      * @param pattern Pattern to check for
      * @param caseSensitive Case sensitivity setting for check
      * @return TRUE if pattern is contained within this value, otherwise FALSE
      */
    virtual bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const = 0;

    /**
      * Compare to instance if they contain the same content.
      * Subclasses implement under which conditions two instances are equal.
      * Subclasses of different type are never equal.
      * @param other other instance to compare with
      * @return TRUE if both instances are equal
      */
    virtual bool operator==(const ValueItem &other) const = 0;

    /**
     * Unique numeric identifier for every ValueItem instance.
     * @return Unique numeric identifier
     */
    quint64 id() const;

protected:
    /// contains text fragments to be removed before performing a "contains pattern" operation
    /// includes among other "{" and "}"
    static const QRegExp ignoredInSorting;

private:
    /// Unique numeric identifier
    const quint64 internalId;
    /// Keeping track of next available unique numeric identifier
    static quint64 internalIdCounter;
};

class KBIBTEXDATA_EXPORT Keyword: public ValueItem
{
public:
    Keyword(const Keyword &other);
    explicit Keyword(const QString &text);

    void setText(const QString &text);
    QString text() const;

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;
    bool operator==(const ValueItem &other) const;

protected:
    QString m_text;
};

class KBIBTEXDATA_EXPORT Person: public ValueItem
{
public:
    /**
    * Create a representation for a person's name. In bibliographies,
    * a person is either an author or an editor. The four parameters
    * cover all common parts of a name. Only first and last name are
    * mandatory (each person should have those).
    @param firstName First name of a person. Example: "Peter"
    @param lastName Last name of a person. Example: "Smith"
    @param suffix Suffix after a name. Example: "jr."
    */
    Person(const QString &firstName, const QString &lastName, const QString &suffix = QString());
    Person(const Person &other);

    QString firstName() const;
    QString lastName() const;
    QString suffix() const;

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;
    bool operator==(const ValueItem &other) const;

    static QString transcribePersonName(const QString &formatting, const QString &firstName, const QString &lastName, const QString &suffix = QString());
    static QString transcribePersonName(const Person *person, const QString &formatting);

private:
    QString m_firstName;
    QString m_lastName;
    QString m_suffix;

};

class KBIBTEXDATA_EXPORT MacroKey: public ValueItem
{
public:
    MacroKey(const MacroKey &other);
    explicit MacroKey(const QString &text);

    void setText(const QString &text);
    QString text() const;
    bool isValid();

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;
    bool operator==(const ValueItem &other) const;

protected:
    QString m_text;
    static const QRegExp validMacroKey;
};

class KBIBTEXDATA_EXPORT PlainText: public ValueItem
{
public:
    PlainText(const PlainText &other);
    explicit PlainText(const QString &text);

    void setText(const QString &text);
    QString text() const;

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;
    bool operator==(const ValueItem &other) const;

protected:
    QString m_text;
};

class KBIBTEXDATA_EXPORT VerbatimText: public ValueItem
{
public:
    VerbatimText(const VerbatimText &other);
    explicit VerbatimText(const QString &text);

    void setText(const QString &text);
    QString text() const;

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode);
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;
    bool operator==(const ValueItem &other) const;

protected:
    QString m_text;

private:
    struct ColorLabelPair {
        QString hexColor;
        QString label;
    };

    static QList<ColorLabelPair> colorLabelPairs;
    static bool colorLabelPairsInitialized;
};

/**
 * Container class to hold values of BibTeX entry fields and similar value types in BibTeX file.
 * A Value object is built from a list of @see ValueItem objects.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT Value: public QVector<QSharedPointer<ValueItem> >
{
public:
    Value();
    Value(const Value &other);
    virtual ~Value();

    void merge(const Value &other);

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode);
    void replace(const QString &before, const QSharedPointer<ValueItem> &after);

    /**
      * Check if this value contains text pattern @p pattern.
      * @param pattern Pattern to check for
      * @param caseSensitive Case sensitivity setting for check
      * @return TRUE if pattern is contained within this value, otherwise FALSE
      */
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

    bool contains(const ValueItem &item) const;

    Value &operator=(const Value &rhs);

private:
    void mergeFrom(const Value &other);
};

class KBIBTEXDATA_EXPORT PlainTextValue: private NotificationListener
{
public:
    static QString text(const Value &value);
    static QString text(const ValueItem &valueItem);
    static QString text(const QSharedPointer<ValueItem> &valueItem);

    void notificationEvent(int eventId);

private:
    enum ValueItemType { VITOther = 0, VITPerson, VITKeyword} lastItem;

    PlainTextValue();
    void readConfiguration();
    static PlainTextValue *notificationListener;
    static QString personNameFormatting;

    static QString text(const ValueItem &valueItem, ValueItemType &vit);

};

Q_DECLARE_METATYPE(Value)

#endif
