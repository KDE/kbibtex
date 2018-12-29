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

#ifndef KBIBTEX_IO_FILEIMPORTERBIBTEX_H
#define KBIBTEX_IO_FILEIMPORTERBIBTEX_H

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

#include <QTextStream>
#include <QSharedPointer>
#include <QStringList>
#include <QSet>

#include "kbibtex.h"
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
    Q_OBJECT

public:
    static const char *defaultCodecName;

    enum CommentHandling {IgnoreComments = 0, KeepComments = 1};

    /**
     * Creates an importer class to read a BibTeX file.
     */
    explicit FileImporterBibTeX(QObject *parent);

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
     * @return A list of Keyword object containing the keywords
      * @see Keyword
     */
    static QList<QSharedPointer<Keyword> > splitKeywords(const QString &text, char *usedSplitChar = nullptr);

    /**
     * Split a list of names into single Person objects.
     * Examples: "Smith, John, Fulkerson, Ford, and Johnson, Tim"
     * or "John Smith and Tim Johnson"
     * @param text Text containing the persons' names
     * @return A list of Person object containing the names
     * @see Person
     */
    static QList<QSharedPointer<Person> > splitNames(const QString &text, const int line_number = 1, QObject *parent = nullptr);

    /**
     * Split a person's name into its parts and construct a Person object from them.
     * This is a functions specialized on the properties of (La)TeX code considering
     * e.g. curly brackets.
     * @param name The persons name
     * @return A Person object containing the name
     * @see Person
     */
    static QSharedPointer<Person> personFromString(const QString &name, const int line_number = 1, QObject *parent = nullptr);

    static void parsePersonList(const QString &text, Value &value, const int line_number = 1, QObject *parent = nullptr);

    void setCommentHandling(CommentHandling commentHandling);

public slots:
    void cancel() override;

private:
    enum Token {
        tAt = 1, tBracketOpen = 2, tBracketClose = 3, tAlphaNumText = 4, tComma = 5, tAssign = 6, tDoublecross = 7, tEOF = 0xffff, tUnknown = -1
    };
    enum CommaContainment { ccNoComma = 0, ccContainsComma = 1 };

    struct {
        int countCurlyBrackets, countQuotationMarks;
        int countFirstNameFirst, countLastNameFirst;
        int countNoCommentQuote, countCommentPercent, countCommentCommand;
        int countProtectedTitle, countUnprotectedTitle;
        QString mostRecentListSeparator;
    } m_statistics;

    bool m_cancelFlag;
    QTextStream *m_textStream;
    CommentHandling m_commentHandling;
    KBibTeX::Casing m_keywordCasing;
    QStringList m_keysForPersonDetection;
    QSet<QString> m_knownElementIds;

    /// low-level character operations
    QChar m_prevChar, m_nextChar;
    unsigned int m_lineNo;
    QString m_prevLine, m_currentLine;
    bool readChar();
    bool readCharUntil(const QString &until);
    bool skipWhiteChar();
    QString readLine();

    /// high-level parsing functions
    Comment *readCommentElement();
    Comment *readPlainCommentElement(const QString &prefix);
    Macro *readMacroElement();
    Preamble *readPreambleElement();
    Entry *readEntryElement(const QString &typeString);
    Element *nextElement();
    Token nextToken();
    QString readString(bool &isStringKey);
    QString readSimpleString(const QString &until = QString());
    QString readQuotedString();
    QString readBracketString();
    Token readValue(Value &value, const QString &fieldType);

    static QSharedPointer<Person> personFromString(const QString &name, CommaContainment *comma, const int line_number, QObject *parent);
    static QSharedPointer<Person> personFromTokenList(const QStringList &tokens, CommaContainment *comma, const int line_number, QObject *parent);
    static void parsePersonList(const QString &text, Value &value, CommaContainment *comma, const int line_number, QObject *parent);

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

    bool evaluateParameterComments(QTextStream *textStream, const QString &line, File *file);
    QString tokenidToString(Token token);
};

#endif // KBIBTEX_IO_FILEIMPORTERBIBTEX_H
