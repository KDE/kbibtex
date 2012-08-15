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

#include <typeinfo>

#include <QTextCodec>
#include <QIODevice>
#include <QRegExp>
#include <QCoreApplication>
#include <QStringList>

#include <KDebug>

#include <file.h>
#include <comment.h>
#include <macro.h>
#include <preamble.h>
#include <entry.h>
#include <element.h>
#include <encoderlatex.h>
#include <value.h>
#include <bibtexentries.h>
#include <bibtexfields.h>
#include <fileexporterbibtex.h>

#include "fileimporterbibtex.h"

const QString extraAlphaNumChars = QString("?'`-_:.+/$\\\"&");
const QRegExp htmlRegExp = QRegExp("</?(a|pre|p|br|span|i|b|italic)\\b[^>]*>", Qt::CaseInsensitive);

FileImporterBibTeX::FileImporterBibTeX(bool ignoreComments, KBibTeX::Casing keywordCasing)
        : FileImporter(), m_cancelFlag(false), m_lineNo(1), m_currentLine(), m_textStream(NULL), m_currentChar(' '), m_ignoreComments(ignoreComments), m_keywordCasing(keywordCasing)
{
    // nothing
}

FileImporterBibTeX::~FileImporterBibTeX()
{
}

File* FileImporterBibTeX::load(QIODevice *iodevice)
{
    m_cancelFlag = false;

    File *result = new File();

    m_textStreamLastPos = 0;
    m_textStream = new QTextStream(iodevice);
    m_textStream->setCodec("us-ascii"); ///< unless we learn something else, assume 7-bit US-ASCII
    QString rawText = "";
    while (!m_textStream->atEnd()) {
        QString line = m_textStream->readLine();
        bool skipline = evaluateParameterComments(m_textStream, line.toLower(), result);
        if (line.startsWith(QLatin1String("<?xml")) && line.endsWith("?>"))
            /// Hop over XML declarations
            skipline = true;
        if (!skipline)
            rawText.append(line).append("\n");
    }

    delete m_textStream;

    /** Remove HTML code from the input source */
    rawText = rawText.replace(htmlRegExp, "");

    rawText = EncoderLaTeX::currentEncoderLaTeX() ->decode(rawText);

    unescapeLaTeXChars(rawText);

    m_textStreamLastPos = 0;
    m_textStream = new QTextStream(&rawText, QIODevice::ReadOnly);
    m_textStream->setCodec("UTF-8");
    m_lineNo = 1;
    m_prevLine = m_currentLine = QString();

    while (!m_cancelFlag && !m_textStream->atEnd()) {
        emit progress(m_textStream->pos(), rawText.length());
        Element * element = nextElement();

        if (element != NULL) {
            if (!m_ignoreComments || typeid(*element) != typeid(Comment))
                result->append(element);
            else
                delete element;
        }
    }
    emit progress(100, 100);

    if (m_cancelFlag) {
        kWarning() << "Loading file has been canceled";
        delete result;
        result = NULL;
    }

    delete m_textStream;

    return result;
}

bool FileImporterBibTeX::guessCanDecode(const QString & rawText)
{
    static const QRegExp bibtexLikeText("@\\w+\\{.+\\}");
    QString text = EncoderLaTeX::currentEncoderLaTeX()->decode(rawText);
    return text.indexOf(bibtexLikeText) >= 0;
}

void FileImporterBibTeX::cancel()
{
    m_cancelFlag = true;
}

Element *FileImporterBibTeX::nextElement()
{
    Token token = nextToken();

    if (token == tAt) {
        QString elementType = readSimpleString();
        if (elementType.toLower() == "comment")
            return readCommentElement();
        else if (elementType.toLower() == "string")
            return readMacroElement();
        else if (elementType.toLower() == "preamble")
            return readPreambleElement();
        else if (elementType.toLower() == QLatin1String("import")) {
            kDebug() << "Skipping potential HTML/JavaScript @import statement";
            return NULL;
        } else if (!elementType.isEmpty())
            return readEntryElement(elementType);
        else {
            kWarning() << "ElementType is empty";
            return NULL;
        }
    } else if (token == tUnknown && m_currentChar == QChar('%')) {
        /// do not complain about LaTeX-like comments, just eat them
        return readPlainCommentElement();
    } else if (token == tUnknown) {
        kDebug() << "Unknown token \"" << m_currentChar << "(" << m_currentChar.unicode() << ")" << "\" near line " << m_lineNo << ", treating as comment";
        return readPlainCommentElement();
    }

    if (token != tEOF)
        kWarning() << "Don't know how to parse next token of type " << tokenidToString(token) << " in line " << m_lineNo << "(" << m_prevLine << endl << m_currentLine << ")" << endl;

    return NULL;
}

Comment *FileImporterBibTeX::readCommentElement()
{
    while (m_currentChar != '{' && m_currentChar != '(' && !m_textStream->atEnd()) {
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    return new Comment(readBracketString(m_currentChar));
}

Comment *FileImporterBibTeX::readPlainCommentElement()
{
    QString result = readLine();
    if (m_currentChar == '\n') {
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine = QLatin1String("");
    } else
        m_currentLine.append(m_currentChar);
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd() && m_currentChar != '@' && !m_currentChar.isSpace()) {
        result.append('\n').append(m_currentChar);
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
        result.append(readLine());
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    if (result.startsWith(QLatin1String("x-kbibtex"))) {
        /// ignore special comments
        return NULL;
    }

    return new Comment(result);
}

Macro *FileImporterBibTeX::readMacroElement()
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            kWarning() << "Error in parsing unknown macro' (near line " << m_lineNo << ":" << m_prevLine << endl << m_currentLine <<  "): Opening curly brace ({) expected";
            return NULL;
        }
        token = nextToken();
    }

    QString key = readSimpleString();
    if (nextToken() != tAssign) {
        kError() << "Error in parsing macro '" << key << "'' (near line " << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Assign symbol (=) expected";
        return NULL;
    }

    Macro *macro = new Macro(key);
    do {
        bool isStringKey = false;
        QString text = readString(isStringKey).simplified();
        if (isStringKey)
            macro->value().append(new MacroKey(text));
        else
            macro->value().append(new PlainText(text));

        token = nextToken();
    } while (token == tDoublecross);

    return macro;
}

Preamble *FileImporterBibTeX::readPreambleElement()
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            kWarning() << "Error in parsing unknown preamble' (near line " << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Opening curly brace ({) expected";
            return NULL;
        }
        token = nextToken();
    }

    Preamble *preamble = new Preamble();
    do {
        bool isStringKey = false;
        /// Remember: strings from preamble do not get encoded,
        /// may contain raw LaTeX commands and code
        QString text = readString(isStringKey).simplified();
        if (isStringKey)
            preamble->value().append(new MacroKey(text));
        else
            preamble->value().append(new PlainText(text));

        token = nextToken();
    } while (token == tDoublecross);

    return preamble;
}

Entry *FileImporterBibTeX::readEntryElement(const QString& typeString)
{
    BibTeXEntries *be = BibTeXEntries::self();
    BibTeXFields *bf = BibTeXFields::self();
    EncoderLaTeX *encoder = EncoderLaTeX::currentEncoderLaTeX();

    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            kWarning() << "Error in parsing unknown entry (near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Opening curly brace '{' expected";
            return NULL;
        }
        token = nextToken();
    }

    QString id = readSimpleString(QChar(','));
    /// try to avoid non-ascii characters in ids
    encoder->convertToPlainAscii(id);

    Entry *entry = new Entry(be->format(typeString, m_keywordCasing), id);

    token = nextToken();
    do {
        if (token == tBracketClose || token == tEOF)
            break;
        else if (token != tComma) {
            if (m_currentChar.isLetter())
                kWarning() << "Error in parsing entry" << id << "(near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Comma symbol (,) expected but got letter" << m_currentChar << "(token" << tokenidToString(token) << ")";
            else
                kWarning() << "Error in parsing entry" << id << "(near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Comma symbol (,) expected but got character 0x" << QString::number(m_currentChar.unicode(), 16) << "(token" << tokenidToString(token) << ")";
            delete entry;
            return NULL;
        }

        QString keyName = bf->format(readSimpleString(), m_keywordCasing);
        /// try to avoid non-ascii characters in keys
        encoder->convertToPlainAscii(keyName);

        token = nextToken();
        if (keyName.isEmpty() || token == tBracketClose) {
            // entry is buggy, but we still accept it
            break;
        } else if (token != tAssign) {
            kError() << "Error in parsing entry" << id << " (near line " << m_lineNo  << ":" << m_prevLine << endl << m_currentLine << "): Assign symbol (=) expected after field name" << keyName;
            delete entry;
            return NULL;
        }

        Value value;

        /** check for duplicate fields */
        if (entry->contains(keyName)) {
            if (keyName.toLower() == Entry::ftKeywords || keyName.toLower() == Entry::ftUrl) {
                /// Special handling of keywords and URLs: instead of using fallback names
                /// like "keywords2", "keywords3", ..., append new keywords to
                /// already existing keyword value
                value = entry->value(keyName);
            } else {
                int i = 2;
                QString appendix = QString::number(i);
                while (entry->contains(keyName + appendix)) {
                    ++i;
                    appendix = QString::number(i);
                }
                kDebug() << "Entry already contains a key" << keyName << "(near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "), using" << (keyName + appendix);
                keyName += appendix;
            }
        }

        token = readValue(value, keyName);

        entry->insert(keyName, value);
    } while (true);

    return entry;
}

FileImporterBibTeX::Token FileImporterBibTeX::nextToken()
{
    if (m_textStream->atEnd())
        return tEOF;
    if (m_textStream->pos() == m_textStreamLastPos)
        *m_textStream >> m_currentChar;

    Token curToken = tUnknown;

    while (!m_textStream->atEnd() && (m_currentChar.isSpace() || m_currentChar == '\t')) {
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    switch (m_currentChar.toAscii()) {
    case '@':
        curToken = tAt;
        break;
    case '{':
    case '(':
        curToken = tBracketOpen;
        break;
    case '}':
    case ')':
        curToken = tBracketClose;
        break;
    case ',':
        curToken = tComma;
        break;
    case '=':
        curToken = tAssign;
        break;
    case '#':
        curToken = tDoublecross;
        break;
    default:
        if (m_textStream->atEnd())
            curToken = tEOF;
    }

    if (curToken != tUnknown && curToken != tEOF) {
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    m_textStreamLastPos = m_textStream->pos();
    return curToken;
}

QString FileImporterBibTeX::readString(bool &isStringKey)
{
    if (m_currentChar.isSpace()) {
        m_textStream->skipWhiteSpace();
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    isStringKey = false;
    switch (m_currentChar.toAscii()) {
    case '{':
    case '(':
        return readBracketString(m_currentChar);
    case '"':
        return readQuotedString();
    default:
        isStringKey = true;
        return readSimpleString();
    }
}

QString FileImporterBibTeX::readSimpleString(QChar until)
{
    QString result;

    if (m_currentChar == '\n') {
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine = QLatin1String("");
    }
    if (m_currentChar.isSpace()) {
        m_textStream->skipWhiteSpace();
        *m_textStream >> m_currentChar;
    }
    /*
        if (m_currentChar.isLetterOrNumber() || extraAlphaNumChars.contains(m_currentChar)) {
            result.append(m_currentChar);
            if (m_currentChar == '\n') {
                ++m_lineNo;
                m_prevLine = m_currentLine;
                m_currentLine = QLatin1String("");
            } else
                m_currentLine.append(m_currentChar);

            *m_textStream >> m_currentChar;
        }
    */
    while (!m_textStream->atEnd()) {
        if (until != '\0') {
            if (m_currentChar != until)
                result.append(m_currentChar);
            else
                break;
        } else
            if (m_currentChar.isLetterOrNumber() || extraAlphaNumChars.contains(m_currentChar))
                result.append(m_currentChar);
            else
                break;
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }
    return result;
}

QString FileImporterBibTeX::readQuotedString()
{
    QString result;
    QChar lastChar = m_currentChar;
    if (m_currentChar == '\n') {
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine = QLatin1String("");
    } else
        m_currentLine.append(m_currentChar);
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd()) {
        if (m_currentChar != '"' || lastChar == '\\')
            result.append(m_currentChar);
        else
            break;
        lastChar = m_currentChar;
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    /** read character after closing " */
    if (m_currentChar == '\n') {
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine = QLatin1String("");
    } else
        m_currentLine.append(m_currentChar);
    *m_textStream >> m_currentChar;

    return result;
}

QString FileImporterBibTeX::readLine()
{
    QString result;
    while (!m_textStream->atEnd() && m_currentChar != '\n') {
        result.append(m_currentChar);
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }
    return result;
}

QString FileImporterBibTeX::readBracketString(const QChar openingBracket) ///< do not use reference on QChar here!
{
    QString result;
    QChar closingBracket = '}';
    if (openingBracket == '(')
        closingBracket = ')';
    int counter = 1;
    if (m_currentChar == '\n') {
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine = QLatin1String("");
    } else
        m_currentLine.append(m_currentChar);

    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd()) {
        if (m_currentChar == openingBracket)
            counter++;
        else if (m_currentChar == closingBracket)
            counter--;

        if (counter == 0)
            break;
        else
            result.append(m_currentChar);
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }
    if (m_currentChar == '\n') {
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine = QLatin1String("");
    } else
        m_currentLine.append(m_currentChar);
    *m_textStream >> m_currentChar;
    return result;
}

FileImporterBibTeX::Token FileImporterBibTeX::readValue(Value& value, const QString& key)
{
    Token token = tUnknown;
    QString iKey = key.toLower();

    do {
        bool isStringKey = false;
        QString text = readString(isStringKey).simplified();
        /// for all entries except for abstracts and URLs/filenames ...
        if (iKey != Entry::ftAbstract && !(iKey.startsWith(Entry::ftUrl) && !iKey.startsWith(Entry::ftUrlDate)) && !iKey.startsWith(Entry::ftLocalFile)) {
            /// ... remove redundant spaces including newlines
            text = text.simplified();
        }
        /// abstracts will keep their formatting (regarding line breaks)
        /// as requested by Thomas Jensch via mail (20 October 2010)

        if (iKey == Entry::ftAuthor || iKey == Entry::ftEditor) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else {
                QStringList persons;

                /// handle "et al." i.e. "and others"
                bool hasOthers = false;
                if (text.endsWith(QLatin1String("and others"))) {
                    hasOthers = true;
                    text = text.left(text.length() - 10);
                }

                splitPersonList(text, persons);
                for (QStringList::ConstIterator pit = persons.constBegin(); pit != persons.constEnd(); ++pit) {
                    Person *person = splitName(*pit);
                    if (person != NULL)
                        value.append(person);
                }

                if (hasOthers)
                    value.append(new PlainText(QLatin1String("others")));
            }
        } else if (iKey == Entry::ftPages) {
            static const QRegExp rangeInAscii("\\s*--?\\s*");
            text.replace(rangeInAscii, QChar(0x2013));
            if (isStringKey)
                value.append(new MacroKey(text));
            else
                value.append(new PlainText(text));
        } else if ((iKey.startsWith(Entry::ftUrl) && !iKey.startsWith(Entry::ftUrlDate)) || iKey.startsWith(Entry::ftLocalFile) || iKey.compare(QLatin1String("ee"), Qt::CaseInsensitive) == 0 || iKey.compare(QLatin1String("biburl"), Qt::CaseInsensitive) == 0) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else {
                /// Assumption: in fields like Url or LocalFile, file names are separated by ; or ,
                static const QRegExp semicolonSpace = QRegExp("[;]\\s*");
                QStringList fileList = text.split(semicolonSpace, QString::SkipEmptyParts);
                foreach(const QString &filename, fileList) {
                    value.append(new VerbatimText(filename));
                }
            }
        } else if (iKey == Entry::ftMonth) {
            if (isStringKey) {
                static const QRegExp monthThreeChars("^[a-z]{3}", Qt::CaseInsensitive);
                if (monthThreeChars.indexIn(text) == 0)
                    text = text.left(3).toLower();
                value.append(new MacroKey(text));
            } else
                value.append(new PlainText(text));
        } else if (iKey.startsWith(Entry::ftDOI)) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else {
                int p = -5;
                while ((p = KBibTeX::doiRegExp.indexIn(text, p + 5)) >= 0)
                    value.append(new VerbatimText(KBibTeX::doiRegExp.cap(0)));
            }
        } else if (iKey == Entry::ftColor) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else
                value.append(new VerbatimText(text));
        } else if (iKey == Entry::ftCrossRef) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else
                value.append(new VerbatimText(text));
        } else if (iKey == Entry::ftKeywords) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else {
                QList<Keyword*> keywords = splitKeywords(text);
                for (QList<Keyword*>::Iterator it = keywords.begin(); it != keywords.end(); ++it)
                    value.append(*it);
            }
        } else {
            if (isStringKey)
                value.append(new MacroKey(text));
            else
                value.append(new PlainText(text));
        }

        token = nextToken();
    } while (token == tDoublecross);

    return token;
}

void FileImporterBibTeX::unescapeLaTeXChars(QString &text)
{
    text.replace("\\&", "&");
}

QList<Keyword*> FileImporterBibTeX::splitKeywords(const QString& text)
{
    QList<Keyword*> result;
    /// define a list of characters where keywords will be split along
    /// finalize list with null character
    static char splitChars[] = ";,\0";
    static const QRegExp splitAlong[] = {QRegExp(QString("\\s*%1\\s*").arg(splitChars[0])), QRegExp(QString("\\s*%1\\s*").arg(splitChars[1])), QRegExp()};
    char *curSplitChar = splitChars;
    int index = 0;

    /// for each char in list ...
    while (*curSplitChar != '\0') {
        /// check if character is contained in text (should be cheap to test)
        if (text.contains(*curSplitChar)) {
            /// split text along a pattern like spaces-splitchar-spaces
            /// extract keywords
            const QStringList keywords = text.split(splitAlong[index], QString::SkipEmptyParts);
            /// build QList of Keyword objects from keywords
            foreach(const QString &keyword, keywords) {
                result.append(new Keyword(keyword));
            }
            /// no more splits neccessary
            break;
        }
        /// no success so far, test next splitting character
        ++curSplitChar;
        ++index;
    }

    /// no split was performed, so whole text must be a single keyword
    if (result.isEmpty())
        result.append(new Keyword(text));

    return result;
}

void FileImporterBibTeX::splitPersonList(const QString& text, QStringList &resultList)
{
    QStringList wordList;
    QString word;
    int bracketCounter = 0;
    resultList.clear();

    for (int pos = 0; pos < text.length(); ++pos) {
        if (text[pos] == '{')
            ++bracketCounter;
        else if (text[pos] == '}')
            --bracketCounter;

        if (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n' || text[pos] == '\r') {
            if (word == "and" && bracketCounter == 0) {
                resultList.append(wordList.join(" "));
                wordList.clear();
            } else if (!word.isEmpty())
                wordList.append(word);

            word = "";
        } else
            word.append(text[pos]);
    }

    if (!word.isEmpty())
        wordList.append(word);
    if (!wordList.isEmpty())
        resultList.append(wordList.join(" "));
}

Person *FileImporterBibTeX::splitName(const QString& text)
{
    QStringList segments;
    CommaContainment commaContainment = splitName(text, segments);
    QString firstName = "";
    QString lastName = "";

    if (segments.isEmpty())
        return NULL;

    if (commaContainment == ccNoComma) {
        /** PubMed uses a special writing style for names, where the last name is followed by single capital letter,
          * each being the first letter of each first name
          * So, check how many single capital letters are at the end of the given segment list */
        int singleCapitalLettersCounter = 0;
        int p = segments.count() - 1;
        while (p > 0 && segments[p].length() == 1 && segments[p][0].isUpper()) {
            --p;
            ++singleCapitalLettersCounter;
        }

        if (singleCapitalLettersCounter > 0) {
            /** this is a special case for names from PubMed, which are formatted like "Fischer T"
              * all segment values until the first single letter segment are last name parts */
            for (int i = 0; i < p; ++i)
                lastName.append(segments[i]).append(" ");
            lastName.append(segments[p]);
            /** single letter segments are first name parts */
            for (int i = p + 1; i < segments.count() - 1; ++i)
                firstName.append(segments[i]).append(" ");
            firstName.append(segments[segments.count() - 1]);
        } else {
            int from = segments.count() - 1;
            lastName = segments[from];
            /** check for lower case parts of the last name such as "van", "von", "de", ... */
            while (from > 0) {
                if (segments[from - 1].compare(segments[from - 1].toLower()) != 0)
                    break;
                --from;
                lastName.prepend(" ");
                lastName.prepend(segments[from]);
            }

            if (from > 0) {
                /** there are segments left for the first name */
                firstName = *segments.begin();
                for (QStringList::Iterator it = ++segments.begin(); from > 1; ++it, --from) {
                    firstName.append(" ");
                    firstName.append(*it);
                }
            }
        }
    } else {
        bool inLastName = TRUE;
        for (int i = 0; i < segments.count(); ++i) {
            if (segments[i] == ",")
                inLastName = FALSE;
            else if (inLastName) {
                if (!lastName.isEmpty()) lastName.append(" ");
                lastName.append(segments[i]);
            } else {
                if (!firstName.isEmpty()) firstName.append(" ");
                firstName.append(segments[i]);
            }
        }
    }

    return new Person(firstName, lastName);
}

/** Splits a name into single words. If the name's text was reversed (Last, First), the result will be true and the comma will be added to segments. Otherwise the functions result will be false. This function respects protecting {...}. */
FileImporterBibTeX::CommaContainment FileImporterBibTeX::splitName(const QString& text, QStringList& segments)
{
    int bracketCounter = 0;
    CommaContainment result = ccNoComma;
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
            result = ccContainsComma;
        } else
            buffer.append(text[pos]);
    }

    if (!buffer.isEmpty())
        segments.append(buffer);

    return result;
}

bool FileImporterBibTeX::evaluateParameterComments(QTextStream *textStream, const QString &line, File *file)
{
    /// Assertion: variable "line" is all lower-case

    /** check if this file requests a special encoding */
    if (line.startsWith("@comment{x-kbibtex-encoding=") && line.endsWith("}")) {
        QString encoding = line.mid(28, line.length() - 29);
        textStream->setCodec(encoding == "latex" ? "UTF-8" : encoding.toAscii());
        encoding = textStream->codec()->name();
        file->setProperty(File::Encoding, encoding);
        return true;
    } else if (line.startsWith("@comment{x-kbibtex-personnameformatting=") && line.endsWith("}")) {
        QString personNameFormatting = line.mid(40, line.length() - 41);
        file->setProperty(File::NameFormatting, personNameFormatting);
        return true;
    } else if (line.startsWith(QLatin1String("% encoding:"))) {
        /// Interprete JabRef's encoding information
        QString encoding = line.mid(12);
        kDebug() << "Using JabRef's encoding:" << encoding;
        textStream->setCodec(encoding.toAscii());
        encoding = textStream->codec()->name();
        file->setProperty(File::Encoding, encoding);
        return true;
    }

    return false;
}

QString FileImporterBibTeX::tokenidToString(Token token)
{
    switch (token) {
    case tAt: return QString("At");
    case tBracketClose: return QString("BracketClose");
    case tBracketOpen: return QString("BracketOpen");
    case tAlphaNumText: return QString("AlphaNumText");
    case tAssign: return QString("Assign");
    case tComma: return QString("Comma");
    case tDoublecross: return QString("Doublecross");
    case tEOF: return QString("EOF");
    case tUnknown: return QString("Unknown");
    default: return QString("<Unknown>");
    }
}
