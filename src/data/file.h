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

#ifndef KBIBTEX_DATA_FILE_H
#define KBIBTEX_DATA_FILE_H

#include <QList>
#include <QStringList>
#include <QSharedPointer>

#ifdef HAVE_KF
#include "kbibtexdata_export.h"
#endif // HAVE_KF

class Element;

/**
 * This class represents a bibliographic file such as a BibTeX file
 * (or any other format after proper conversion). The file's content
 * can be accessed using the inherited QList interface (for example
 * list iterators).
 * @see Element
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT File : public QList<QSharedPointer<Element> >
{
public:
    /// enum and flags to differ between entries, macros etc
    /// used for @see #allKeys() and @see #containsKey()
    enum class ElementType {
        Entry = 0x1, Macro = 0x2, All = Entry | Macro
    };
    Q_DECLARE_FLAGS(ElementTypes, ElementType)

    /// used for property map
    const static QString Url;
    const static QString Encoding;
    const static QString StringDelimiter;
    const static QString CommentContext;
    const static QString CommentPrefix;
    const static QString KeywordCasing;
    const static QString ProtectCasing;
    const static QString NameFormatting;
    const static QString ListSeparator;
    const static QString SortedByIdentifier;

    explicit File();
    explicit File(const File &other);
    explicit File(File &&other);
    ~File();

    /// Copy-assignment operator.
    File &operator= (const File &other);
    /// Move-assignment operator.
    File &operator= (File &&other);

    bool operator== (const File &other) const;
    bool operator!= (const File &other) const;

    /**
     * Check if a given key (e.g. a key for a macro or an id for an entry)
     * is contained in the file object.
     * @see allKeys
     * @return the object addressed by the key, @c nullptr if no such file has been found
     */
    const QSharedPointer<Element> containsKey(const QString &key, ElementTypes elementTypes = ElementType::All) const;

    /**
     * Retrieves a list of all keys for example from macros or entries.
     * @see containsKey
     * @return list of keys
     */
    QStringList allKeys(ElementTypes elementTypes = ElementType::All) const;

    /**
     * Retrieves a set of all unique values (as text) for a specified
     * field from all entries
     * @param fieldName field name to scan, e.g. "volume"
     * @return list of unique values
     */
    QSet<QString> uniqueEntryValuesSet(const QString &fieldName) const;

    /**
     * Retrieves a list of all unique values (as text) for a specified
     * field from all entries
     * @param fieldName field name to scan, e.g. "volume"
     * @return list of unique values
     */
    QStringList uniqueEntryValuesList(const QString &fieldName) const;

    void setProperty(const QString &key, const QVariant &value);
    QVariant property(const QString &key) const;
    QVariant property(const QString &key, const QVariant &defaultValue) const;
    bool hasProperty(const QString &key) const;
    void setPropertiesToDefault();

    /**
     * Check if this File object is valid by its own assessment.
     * No high-level checks, such as on the File instance's content,
     * are performed.
     * @return True if validity checks succeed, false otherwise
     */
    bool checkValidity() const;

    /**
     * Sort a BibTeX file by identifier if comparing two entries
     * @param bibtexfile The original File object, will not be modified
     * @return Sorted copy of the original File object
     */
    static const File *sortByIdentifier(const File *bibtexfile);

private:
    class FilePrivate;
    FilePrivate *d;
};

Q_DECLARE_METATYPE(File*)

KBIBTEXDATA_EXPORT QDebug operator<<(QDebug dbg, const File *file);

#endif // KBIBTEX_DATA_FILE_H
