/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#ifndef KBIBTEX_IO_FILEIMPORTERBIBTEX_H
#define KBIBTEX_IO_FILEIMPORTERBIBTEX_H

#include "kbibtexio_export.h"

#include <QTextStream>
#include <QSharedPointer>

#include "kbibtexnamespace.h"
#include "fileimporter.h"

class Element;
class Comment;
class Preamble;
class Macro;
class Entry;
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
public:
    /**
     * Creates an importer class to read a BibTeX file.
     * @param encoding the file's encoding.
     *     Supports all of iconv's encodings plus "latex",
     *     which performs no encoding but relies on that
     *     this file is pure ASCII only.
     * @param ignoreComments ignore comments in file.
     *     Useful if you for example read from an HTML file,
     *     as all HTML content you be treated as comments otherwise.
     */
    FileImporterBibTeX(bool ignoreComments = true, KBibTeX::Casing keywordCasing = KBibTeX::cLowerCase);
    ~FileImporterBibTeX();

    /**
     * Read data from the given device and construct a File object holding
     * the bibliographic data.
     * @param iodevice opened QIODevice instance ready to read from
     * @return @c valid File object with elements, @c NULL if reading failed for some reason
     */
    File *load(QIODevice *iodevice);

    /** TODO
     */
    static bool guessCanDecode(const QString &text);

    /**
     * Split a list of keyword separated by ";" or "," into single Keyword objects.
     * @param text Text containing the keyword list
     * @return A list of Keyword object containing the keywords
      * @see Keyword
     */
    static QList<Keyword *> splitKeywords(const QString &text);

    /**
     * Split a list of names into single Person objects.
     * Examples: "Smith, John, Fulkerson, Ford, and Johnson, Tim"
     * or "John Smith and Tim Johnson"
     * @param text Text containing the persons' names
     * @return A list of Person object containing the names
     * @see Person
     */
    static QList<QSharedPointer<Person> > splitNames(const QString &text);

    /**
     * Split a person's name into its parts and construct a Person object from them.
     * This is a functions specialized on the properties of (La)TeX code considering
     * e.g. curly brackets.
     * @param name The persons name
     * @return A Person object containing the name
     * @see Person
     */
    static QSharedPointer<Person> personFromString(const QString &name);

    static void parsePersonList(const QString &text, Value &value);


    /**
      * As not always only @c Entry::ftAuthor and @c Entry::ftEditor should be interpreted as Persons, this allows to expand it
      *
      * Any key in this list will also be checked for a Person entry. Thus the splitPerson (xxx and yyy) take place
      * as well as the nameing split in first, last, suffix.
      * @param keylist list of additional keys beside ftAuthor and ftEditor. Must be lowercase
      */
    void setKeysForPersonDetection(const QStringList &keylist);

public slots:
    void cancel();

private:
    enum Token {
        tAt = 1, tBracketOpen = 2, tBracketClose = 3, tAlphaNumText = 4, tComma = 5, tAssign = 6, tDoublecross = 7, tEOF = 0xffff, tUnknown = -1
    };
    enum CommaContainment { ccNoComma = 0, ccContainsComma = 1 };

    struct {
        int countCurlyBrackets, countQuotationMarks;
        int countFirstNameFirst, countLastNameFirst;
        int countNoCommentQuote, countCommentPercent, countCommentCommand;
    } m_statistics;

    bool m_cancelFlag;
    unsigned int m_lineNo;
    QString m_prevLine, m_currentLine;
    QTextStream *m_textStream;
    int m_nextDuePos;
    QChar m_currentChar;
    bool m_ignoreComments;
    KBibTeX::Casing m_keywordCasing;
    QStringList m_keysForPersonDetection;

    Comment *readCommentElement();
    Comment *readPlainCommentElement();
    Macro *readMacroElement();
    Preamble *readPreambleElement();
    Entry *readEntryElement(const QString &typeString);
    Element *nextElement();
    Token nextToken();
    QString readString(bool &isStringKey);
    QString readSimpleString(QChar until = QChar('\0'));
    QString readQuotedString();
    QString readLine();
    QString readBracketString(const QChar openingBracket); ///< do not use reference on QChar here!
    Token readValue(Value &value, const QString &fieldType);

    static QSharedPointer<Person> personFromString(const QString &name, CommaContainment *comma);
    static QSharedPointer<Person> personFromTokenList(const QStringList &tokens, CommaContainment *comma = NULL);
    static void parsePersonList(const QString &text, Value &value, CommaContainment *comma);

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

    bool evaluateParameterComments(QTextStream *textStream, const QString &line, File *file);
    QString tokenidToString(Token token);
};

#endif // KBIBTEX_IO_FILEIMPORTERBIBTEX_H
