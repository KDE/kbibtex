/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <kbibtexnamespace.h>
#include <fileimporter.h>

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
    FileImporterBibTeX(const QString& encoding = "latex", bool ignoreComments = true, KBibTeX::Casing keywordCasing = KBibTeX::cLowerCase);
    ~FileImporterBibTeX();

    /**
     * Read data from the given device and construct a File object holding
     * the bibliographic data.
     * @param iodevice opened QIODevice instance ready to read from
     * @return @c valid File object with elements, @c NULL if reading failed for some reason
     */
    File* load(QIODevice *iodevice);

    /** TODO
     */
    static bool guessCanDecode(const QString & text);

    /**
     * Split a list of keyword separated by ";" or "," into single Keyword objects.
     * @param name The persons name
     * @return A Person object containing the name
     * @see Person
     */
    static QList<Keyword*> splitKeywords(const QString& text);

    /**
     * Split a person's name into its parts and construct a Person object from them.
     * This is a functions specialized on the properties of (La)TeX code considering
     * e.g. curly brackets.
     * @param name The persons name
     * @return A Person object containing the name
     * @see Person
     */
    static Person *splitName(const QString& name);

public slots:
    void cancel();

private:
    enum Token {
        tAt = 1, tBracketOpen = 2, tBracketClose = 3, tAlphaNumText = 4, tComma = 5, tAssign = 6, tDoublecross = 7, tEOF = 0xffff, tUnknown = -1
    };

    bool m_cancelFlag;
    unsigned int m_lineNo;
    QTextStream *m_textStream;
    QChar m_currentChar;
    bool m_ignoreComments;
    QString m_encoding;
    KBibTeX::Casing m_keywordCasing;

    Comment *readCommentElement();
    Comment *readPlainCommentElement();
    Macro *readMacroElement();
    Preamble *readPreambleElement();
    Entry *readEntryElement(const QString& typeString);
    Element *nextElement();
    Token nextToken();
    QString readString(bool &isStringKey);
    QString readSimpleString(QChar until = '\0');
    QString readQuotedString();
    QString readLine();
    QString readBracketString(const QChar openingBracket);
    Token readValue(Value& value, const QString& fieldType);

    void unescapeLaTeXChars(QString &text);

    static void splitPersonList(const QString& name, QStringList &resultList);
    static bool splitName(const QString& name, QStringList& segments);

    bool evaluateParameterComments(QTextStream *textStream, const QString &line);
    QString tokenidToString(Token token);
};

#endif // KBIBTEX_IO_FILEIMPORTERBIBTEX_H
