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

#ifndef BIBTEXVALUE_H
#define BIBTEXVALUE_H

#include <QVector>
#include <QVariant>
#include <QSharedPointer>

#include "kbibtexdata_export.h"

#ifdef HAVE_KF5
#include "notificationhub.h"
#endif // HAVE_KF5

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
    bool operator!=(const ValueItem &other) const;

    /**
     * Unique numeric identifier for every ValueItem instance.
     * @return Unique numeric identifier
     */
    quint64 id() const;

protected:
    /// contains text fragments to be removed before performing a "contains pattern" operation
    /// includes among other "{" and "}"
    static const QRegularExpression ignoredInSorting;

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

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode) override;
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const override;
    bool operator==(const ValueItem &other) const override;

    /**
     * Cheap and fast test if another ValueItem is a Keyword object.
     * @param other another ValueItem object to test
     * @return true if ValueItem is actually a Keyword
     */
    static bool isKeyword(const ValueItem &other);

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

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode) override;
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const override;
    bool operator==(const ValueItem &other) const override;

    static QString transcribePersonName(const QString &formatting, const QString &firstName, const QString &lastName, const QString &suffix = QString());
    static QString transcribePersonName(const Person *person, const QString &formatting);

    /**
     * Cheap and fast test if another ValueItem is a Person object.
     * @param other another ValueItem object to test
     * @return true if ValueItem is actually a Person
     */
    static bool isPerson(const ValueItem &other);

private:
    QString m_firstName;
    QString m_lastName;
    QString m_suffix;

};

QDebug operator<<(QDebug dbg, const Person &person);

Q_DECLARE_METATYPE(Person *)


class KBIBTEXDATA_EXPORT MacroKey: public ValueItem
{
public:
    MacroKey(const MacroKey &other);
    explicit MacroKey(const QString &text);

    void setText(const QString &text);
    QString text() const;
    bool isValid();

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode) override;
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const override;
    bool operator==(const ValueItem &other) const override;

    /**
     * Cheap and fast test if another ValueItem is a MacroKey object.
     * @param other another ValueItem object to test
     * @return true if ValueItem is actually a MacroKey
     */
    static bool isMacroKey(const ValueItem &other);

protected:
    QString m_text;
};

QDebug operator<<(QDebug dbg, const MacroKey &macrokey);


class KBIBTEXDATA_EXPORT PlainText: public ValueItem
{
public:
    PlainText(const PlainText &other);
    explicit PlainText(const QString &text);

    void setText(const QString &text);
    QString text() const;

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode) override;
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const override;
    bool operator==(const ValueItem &other) const override;

    /**
     * Cheap and fast test if another ValueItem is a PlainText object.
     * @param other another ValueItem object to test
     * @return true if ValueItem is actually a PlainText
     */
    static bool isPlainText(const ValueItem &other);

protected:
    QString m_text;
};

QDebug operator<<(QDebug dbg, const PlainText &plainText);


class KBIBTEXDATA_EXPORT VerbatimText: public ValueItem
{
public:
    VerbatimText(const VerbatimText &other);
    explicit VerbatimText(const QString &text);

    void setText(const QString &text);
    QString text() const;

    void replace(const QString &before, const QString &after, ValueItem::ReplaceMode replaceMode) override;
    bool containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const override;
    bool operator==(const ValueItem &other) const override;

    /**
     * Cheap and fast test if another ValueItem is a VerbatimText object.
     * @param other another ValueItem object to test
     * @return true if ValueItem is actually a VerbatimText
     */
    static bool isVerbatimText(const ValueItem &other);

protected:
    QString m_text;

private:
#ifdef HAVE_KF5
    struct ColorLabelPair {
        QString hexColor;
        QString label;
    };

    static QList<ColorLabelPair> colorLabelPairs;
    static bool colorLabelPairsInitialized;
#endif // HAVE_KF5
};

QDebug operator<<(QDebug dbg, const VerbatimText &verbatimText);


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
    Value(Value &&other);
    virtual ~Value();

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
    Value &operator=(Value &&rhs);
    Value &operator<<(const QSharedPointer<ValueItem> &value);
    bool operator==(const Value &rhs) const;
    bool operator!=(const Value &rhs) const;
};

QDebug operator<<(QDebug dbg, const Value &value);


class KBIBTEXDATA_EXPORT PlainTextValue
#ifdef HAVE_KF5
  : private NotificationListener
#endif // HAVE_KF5
{
public:
    static QString text(const Value &value);
    static QString text(const ValueItem &valueItem);
    static QString text(const QSharedPointer<const ValueItem> &valueItem);

#ifdef HAVE_KF5
    void notificationEvent(int eventId) override;
#endif // HAVE_KF5

private:
    enum ValueItemType { VITOther = 0, VITPerson, VITKeyword};

#ifdef HAVE_KF5
    PlainTextValue();
    void readConfiguration();
    static PlainTextValue *notificationListener;
    static QString personNameFormatting;
#else // HAVE_KF5
    static const QString personNameFormatting;
#endif // HAVE_KF5

    static QString text(const ValueItem &valueItem, ValueItemType &vit);

};

Q_DECLARE_METATYPE(Value)

#endif
