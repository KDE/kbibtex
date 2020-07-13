/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_IO_FILEIMPORTERBIBTEX_H
#define KBIBTEX_IO_FILEIMPORTERBIBTEX_H

#include <QTextStream>
#include <QSharedPointer>
#include <QStringList>
#include <QSet>

#include <KBibTeX>
#include <FileImporter>

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

class Element;
class Comment;
class Preamble;
class Macro;
class Value;
class Keyword;

/**
 * This class reads a BibTeX file from a QIODevice (such as a QFile) and
 * creates a File object which can be used to access the BibTeX elements.
 * @see File
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileImporterBibTeX : public FileImporter
{
    Q_OBJECT

public:
    enum class CommentHandling {Ignore, Keep};

    /**
     * Creates an importer class to read a BibTeX file.
     */
    explicit FileImporterBibTeX(QObject *parent);
    ~FileImporterBibTeX();

    virtual File *fromString(const QString &text) override;

    /**
     * Read data from the given device and construct a File object holding
     * the bibliographic data.
     * @param iodevice opened QIODevice instance ready to read from
     * @return @c valid File object with elements, @c NULL if reading failed for some reason
     */
    File *load(QIODevice *iodevice) override;

    /** TODO
     */
    static bool guessCanDecode(const QString &text);

    /**
     * Split a list of keyword separated by ";" or "," into single Keyword objects.
     * @param text Text containing the keyword list
     * @param usedSplitChar The split char that is used to separate the keywords
     * @return A list of Keyword object containing the keywords
     * @see Keyword
     */
    static QList<QSharedPointer<Keyword> > splitKeywords(const QString &text, char *usedSplitChar = nullptr);

    /**
     * Split a list of names into single Person objects.
     * Examples: "Smith, John, Fulkerson, Ford, and Johnson, Tim"
     * or "John Smith and Tim Johnson"
     * @param text Text containing the persons' names
     * @param line_number Line number to use in error message if splitting failed
     * @param parent The parent object
     * @return A list of Person object containing the names
     * @see Person
     */
    static QList<QSharedPointer<Person> > splitNames(const QString &text);

    /**
     * Split a person's name into its parts and construct a Person object from them.
     * This is a functions specialized on the properties of (La)TeX code considering
     * e.g. curly brackets.
     * @param name The persons name
     * @param line_number Line number to use in error message if splitting failed
     * @param parent The parent object
     * @return A Person object containing the name
     * @see Person
     */
    static QSharedPointer<Person> personFromString(const QString &name, const int line_number = 1, QObject *parent = nullptr);

    static void parsePersonList(const QString &text, Value &value, const int line_number = 1, QObject *parent = nullptr);

    /**
     * Convert a textual representation of an edition string into a number.
     * Examples for supported string patterns include '4', '4th', or 'fourth'.
     * Success of the conversion is returned via the @c ok variable, where the
     * function caller has to provide a pointer to a boolean variable.
     * In case of success, the function's result is the edition, in case
     * of failure, i.e. @c *ok==false, the result is undefined.
     * @param[in] editionString A string representing an edition number
     * @param[out] ok Pointer to a boolean variable used to return the success (@c true) or failure (@c false) state of the conversion; must not be @c nullptr
     * @return In case of success, the edition as a positive int, else undefined
     */
    static int editionStringToNumber(const QString &editionString, bool *ok);

    void setCommentHandling(CommentHandling commentHandling);

public slots:
    void cancel() override;

private:
    class Private;
    Private *d;

    bool m_cancelFlag;


    /// high-level parsing functions
    Comment *readCommentElement();
    Comment *readPlainCommentElement(const QString &prefix);

    /**
     * Split a string into white-space separated chunks,
     * but keep parts intact which are protected by {...}.
     * Example: "aa bb ccc    {dd ee    ff}"
     * will be split into "aa", "bb", "ccc", "{dd ee    ff}"
     *
     * @param text input string to be split
     * @param segments list where chunks will be added to
     */
    static void contextSensitiveSplit(const QString &text, QStringList &segments);

    static QString bibtexAwareSimplify(const QString &text);
    static QString rstrip(const QString &text);
};

#endif // KBIBTEX_IO_FILEIMPORTERBIBTEX_H
