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
    m_keysForPersonDetection.append(Entry::ftAuthor);
    m_keysForPersonDetection.append(Entry::ftEditor);
}

FileImporterBibTeX::~FileImporterBibTeX()
{
}

File *FileImporterBibTeX::load(QIODevice *iodevice)
{
    m_cancelFlag = false;

    File *result = new File();
    /// Used to determine if file prefers quotation marks over
    /// curly brackets or the other way around
    m_statistics.countCurlyBrackets = 0;
    m_statistics.countQuotationMarks = 0;
    m_statistics.countFirstNameFirst = 0;
    m_statistics.countLastNameFirst = 0;
    m_statistics.countNoCommentQuote = 0;
    m_statistics.countCommentPercent = 0;
    m_statistics.countCommentCommand = 0;

    m_nextDuePos = 0;
    m_textStream = new QTextStream(iodevice);
    m_textStream->setCodec("us-ascii"); ///< unless we learn something else, assume 7-bit US-ASCII
    result->setProperty(File::Encoding, QLatin1String("latex"));

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

    m_nextDuePos = 0;
    m_textStream = new QTextStream(&rawText, QIODevice::ReadOnly);
    m_textStream->setCodec("UTF-8");
    m_lineNo = 1;
    m_prevLine = m_currentLine = QString();

    while (!m_cancelFlag && !m_textStream->atEnd()) {
        emit progress(m_textStream->pos(), rawText.length());
        Element *element = nextElement();

        if (element != NULL) {
            if (!m_ignoreComments || typeid(*element) != typeid(Comment))
                result->append(QSharedPointer<Element>(element));
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

    /// Set the file's preferences for string delimiters
    /// deduced from statistics built while parsing the file
    result->setProperty(File::StringDelimiter, m_statistics.countQuotationMarks > m_statistics.countCurlyBrackets ? QLatin1String("\"\"") : QLatin1String("{}"));
    /// Set the file's preferences for name formatting
    result->setProperty(File::NameFormatting, m_statistics.countFirstNameFirst > m_statistics.countLastNameFirst ? QLatin1String("<%f ><%l>< %s>") : QLatin1String("<%l><, %s>,< %f>"));
    /// Set the file's preferences for quoting of comments
    if (m_statistics.countNoCommentQuote > m_statistics.countCommentCommand && m_statistics.countNoCommentQuote > m_statistics.countCommentPercent)
        result->setProperty(File::QuoteComment, (int)FileExporterBibTeX::qcNone);
    else if (m_statistics.countCommentCommand > m_statistics.countNoCommentQuote && m_statistics.countCommentCommand > m_statistics.countCommentPercent)
        result->setProperty(File::QuoteComment, (int)FileExporterBibTeX::qcCommand);
    else
        result->setProperty(File::QuoteComment, (int)FileExporterBibTeX::qcPercentSign);
    // TODO gather more statistics for keyword casing etc.

    return result;
}

bool FileImporterBibTeX::guessCanDecode(const QString &rawText)
{
    static const QRegExp bibtexLikeText("@\\w+\\{.+\\}");
    QString text = EncoderLaTeX::instance()->decode(rawText);
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
        if (elementType.toLower() == "comment") {
            ++m_statistics.countCommentCommand;
            return readCommentElement();
        } else if (elementType.toLower() == "string")
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
        ++m_statistics.countCommentPercent;
        return readPlainCommentElement();
    } else if (token == tUnknown) {
        kDebug() << "Unknown token \"" << m_currentChar << "(" << QString("0x%1").arg(m_currentChar.unicode(), 4, 16, QLatin1Char('0')) << ")" << "\" near line " << m_lineNo << "(" << m_prevLine << endl << m_currentLine << ")" << ", treating as comment";
        ++m_statistics.countNoCommentQuote;
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

    return new Comment(EncoderLaTeX::instance()->decode(readBracketString(m_currentChar)));
}

Comment *FileImporterBibTeX::readPlainCommentElement()
{
    QString result = EncoderLaTeX::instance()->decode(readLine());
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
        result.append(EncoderLaTeX::instance()->decode(readLine()));
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
        QString text = EncoderLaTeX::instance()->decode(readString(isStringKey).simplified());
        if (isStringKey)
            macro->value().append(QSharedPointer<MacroKey>(new MacroKey(text)));
        else
            macro->value().append(QSharedPointer<PlainText>(new PlainText(text)));

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
            preamble->value().append(QSharedPointer<MacroKey>(new MacroKey(text)));
        else
            preamble->value().append(QSharedPointer<PlainText>(new PlainText(text)));

        token = nextToken();
    } while (token == tDoublecross);

    return preamble;
}

Entry *FileImporterBibTeX::readEntryElement(const QString &typeString)
{
    BibTeXEntries *be = BibTeXEntries::self();
    BibTeXFields *bf = BibTeXFields::self();
    EncoderLaTeX *encoder = EncoderLaTeX::instance();

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
    if (!EncoderLaTeX::containsOnlyAscii(id)) {
        const QString newId = encoder->convertToPlainAscii(id);
        kWarning() << "Entry id" << id << "contains non-ASCII characters, converted to" << newId;
        id = newId;
    }

    Entry *entry = new Entry(be->format(typeString, m_keywordCasing), id);

    token = nextToken();
    do {
        if (token == tBracketClose || token == tEOF)
            break;
        else if (token != tComma) {
            if (m_currentChar.isLetter())
                kWarning() << "Error in parsing entry" << id << "(near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Comma symbol (,) expected but got letter" << m_currentChar << "(token" << tokenidToString(token) << ")";
            else
                kWarning() << "Error in parsing entry" << id << "(near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Comma symbol (,) expected but got character" << QString("0x%1").arg(m_currentChar.unicode(), 4, 16, QLatin1Char('0')) << "(token" << tokenidToString(token) << ")";
            delete entry;
            return NULL;
        }

        QString keyName = bf->format(readSimpleString(), m_keywordCasing);
        /// try to avoid non-ascii characters in keys
        keyName = encoder->convertToPlainAscii(keyName);

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
            } else if (m_keysForPersonDetection.contains(keyName.toLower())) {
                /// Special handling of authors and editors: instead of using fallback names
                /// like "author2", "author3", ..., append new authors to
                /// already existing author value
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
    if (m_textStream->pos() == m_nextDuePos)
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

    m_nextDuePos = m_textStream->pos();

    if (curToken != tUnknown && curToken != tEOF) {
        if (m_currentChar == '\n') {
            ++m_lineNo;
            m_prevLine = m_currentLine;
            m_currentLine = QLatin1String("");
        } else
            m_currentLine.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

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
    case '(': {
        ++m_statistics.countCurlyBrackets;
        return readBracketString(m_currentChar);
    }
    case '"': {
        ++m_statistics.countQuotationMarks;
        return readQuotedString();
    }
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
        } else if (m_currentChar.isLetterOrNumber() || extraAlphaNumChars.contains(m_currentChar))
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

FileImporterBibTeX::Token FileImporterBibTeX::readValue(Value &value, const QString &key)
{
    Token token = tUnknown;
    const QString iKey = key.toLower();

    do {
        bool isStringKey = false;
        QString text = EncoderLaTeX::instance()->decode(readString(isStringKey));
        /// for all entries except for abstracts ...
        if (iKey != Entry::ftAbstract && !(iKey.startsWith(Entry::ftUrl) && !iKey.startsWith(Entry::ftUrlDate)) && !iKey.startsWith(Entry::ftLocalFile) && !iKey.startsWith(Entry::ftFile)) {
            /// ... remove redundant spaces including newlines
            text = text.simplified();
        }
        /// abstracts will keep their formatting (regarding line breaks)
        /// as requested by Thomas Jensch via mail (20 October 2010)

        if (m_keysForPersonDetection.contains(iKey)) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                CommaContainment comma;
                parsePersonList(text, value, &comma);

                /// Update statistics on name formatting
                if (comma == ccContainsComma)
                    ++m_statistics.countLastNameFirst;
                else
                    ++m_statistics.countFirstNameFirst;
            }
        } else if (iKey == Entry::ftPages) {
            static const QRegExp rangeInAscii("\\s*--?\\s*");
            text.replace(rangeInAscii, QChar(0x2013));
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                value.append(QSharedPointer<PlainText>(new PlainText(text)));
        } else if ((iKey.startsWith(Entry::ftUrl) && !iKey.startsWith(Entry::ftUrlDate)) || iKey.startsWith(Entry::ftLocalFile) || iKey.compare(QLatin1String("ee"), Qt::CaseInsensitive) == 0 || iKey.compare(QLatin1String("biburl"), Qt::CaseInsensitive) == 0) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                /// Assumption: in fields like Url or LocalFile, file names are separated by ; or ,
                static const QRegExp semicolonSpace = QRegExp("[;]\\s*");
                QStringList fileList = text.split(semicolonSpace, QString::SkipEmptyParts);
                foreach(const QString &filename, fileList) {
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(filename)));
                }
            }
        } else if (iKey.startsWith(Entry::ftFile)) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                /// Assumption: this field was written by Mendeley, which uses
                /// a very strange format for file names:
                ///  :C$\backslash$:/Users/BarisEvrim/Documents/Mendeley Desktop/GeversPAMI10.pdf:pdf
                ///  ::
                ///  :Users/Fred/Library/Application Support/Mendeley Desktop/Downloaded/Hasselman et al. - 2011 - (Still) Growing Up What should we be a realist about in the cognitive and behavioural sciences Abstract.pdf:pdf
                if (KBibTeX::mendeleyFileRegExp.indexIn(text) >= 0)    {
                    const QString backslashLaTeX = QLatin1String("$\\backslash$");
                    QString filename = KBibTeX::mendeleyFileRegExp.cap(1);
                    filename = filename.replace(backslashLaTeX, QString::null);
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(filename)));
                } else
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
            }
        } else if (iKey == Entry::ftMonth) {
            if (isStringKey) {
                static const QRegExp monthThreeChars("^[a-z]{3}", Qt::CaseInsensitive);
                if (monthThreeChars.indexIn(text) == 0)
                    text = text.left(3).toLower();
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            } else
                value.append(QSharedPointer<PlainText>(new PlainText(text)));
        } else if (iKey.startsWith(Entry::ftDOI)) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                int p = -5;
                while ((p = KBibTeX::doiRegExp.indexIn(text, p + 5)) >= 0)
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(KBibTeX::doiRegExp.cap(0))));
            }
        } else if (iKey == Entry::ftColor) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                value.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
        } else if (iKey == Entry::ftCrossRef) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                value.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
        } else if (iKey == Entry::ftKeywords) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                QList<Keyword *> keywords = splitKeywords(text);
                for (QList<Keyword *>::Iterator it = keywords.begin(); it != keywords.end(); ++it)
                    value.append(QSharedPointer<Keyword>(*it));
            }
        } else {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                value.append(QSharedPointer<PlainText>(new PlainText(text)));
        }

        token = nextToken();
    } while (token == tDoublecross);

    return token;
}

QList<Keyword *> FileImporterBibTeX::splitKeywords(const QString &text)
{
    QList<Keyword *> result;
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

QList<QSharedPointer<Person> > FileImporterBibTeX::splitNames(const QString &text)
{
    /// Case: Smith, John and Johnson, Tim
    /// Case: Smith, John and Fulkerson, Ford and Johnson, Tim
    /// Case: Smith, John, Fulkerson, Ford, and Johnson, Tim
    /// Case: John Smith and Tim Johnson
    /// Case: John Smith and Ford Fulkerson and Tim Johnson
    /// Case: Smith, John, Johnson, Tim
    /// Case: Smith, John, Fulkerson, Ford, Johnson, Tim
    /// Case: John Smith, Tim Johnson
    /// Case: John Smith, Tim Johnson, Ford Fulkerson

    QList<QSharedPointer<Person> > result;
    QString internalText = text;

    /// Remove invalid characters such as daggers for footnotes
    static const QList<QChar> invalidChars = QList<QChar>() << QChar(0x2020) << QChar(0x2021);
    for (QList<QChar>::ConstIterator it = invalidChars.constBegin(); it != invalidChars.constEnd(); ++it)
        internalText = internalText.replace(*it, QString::null);
    /// Remove numbers to footnotes
    static const QRegExp numberFootnoteRegExp(QLatin1String("(\\w)\\d+\\b"));
    internalText = internalText.replace(numberFootnoteRegExp, QLatin1String("\\1"));

    /// Split input string into tokens which are either name components (first or last name)
    /// or full names (composed of first and last name), depending on the input string's structure
    static const QRegExp split(QLatin1String("\\s*([,]|[,]?\\band\\b|\\n|\\s{4,})\\s*"));
    QStringList authorTokenList = internalText.split(split, QString::SkipEmptyParts);

    bool containsSpace = false;
    for (QStringList::ConstIterator it = authorTokenList.constBegin(); !containsSpace && it != authorTokenList.constEnd(); ++it)
        containsSpace = (*it).contains(QChar(' '));

    if (containsSpace) {
        /// Tokens look like "John Smith"
        for (QStringList::ConstIterator it = authorTokenList.constBegin(); it != authorTokenList.constEnd(); ++it)
            result.append(personFromString(*it));
    } else {
        /// Tokens look like "Smith" or "John"
        /// Assumption: two consecutive tokens form a name
        for (QStringList::ConstIterator it = authorTokenList.constBegin(); it != authorTokenList.constEnd(); ++it) {
            QString lastname = *it;
            ++it;
            if (it != authorTokenList.constEnd()) {
                lastname += QLatin1String(", ") + (*it);
                result.append(personFromString(lastname));
            } else
                break;
        }
    }

    return result;
}

void FileImporterBibTeX::parsePersonList(const QString &text, Value &value)
{
    parsePersonList(text, value, NULL);
}

void FileImporterBibTeX::parsePersonList(const QString &text, Value &value, CommaContainment *comma)
{
    static const QString tokenAnd = QLatin1String("and");
    static const QString tokenOthers = QLatin1String("others");
    static QStringList tokens;
    contextSensitiveSplit(text, tokens);

    int nameStart = 0;
    QString prevToken = QString::null;
    bool encounteredName = false;
    for (int i = 0; i < tokens.count(); ++i) {
        if (tokens[i] == tokenAnd) {
            if (prevToken == tokenAnd)
                kDebug() << "Two subsequent" << tokenAnd << "found in person list";
            else if (!encounteredName)
                kDebug() << "Found" << tokenAnd << "but no name before it";
            else
                value.append(personFromTokenList(tokens.mid(nameStart, i - nameStart), comma));
            nameStart = i + 1;
            encounteredName = false;
        } else if (tokens[i] == tokenOthers) {
            if (i < tokens.count() - 1)
                kDebug() << "Special word" << tokenOthers << "found before last position in person name";
            else
                value.append(QSharedPointer<PlainText>(new PlainText(QLatin1String("others"))));
            nameStart = tokens.count() + 1;
            encounteredName = false;
        } else
            encounteredName = true;
        prevToken = tokens[i];
    }

    if (nameStart < tokens.count())
        value.append(personFromTokenList(tokens.mid(nameStart), comma));
}

QSharedPointer<Person> FileImporterBibTeX::personFromString(const QString &name)
{
    return personFromString(name, NULL);
}

QSharedPointer<Person> FileImporterBibTeX::personFromString(const QString &name, CommaContainment *comma)
{
    static QStringList tokens;
    contextSensitiveSplit(name, tokens);
    return personFromTokenList(tokens, comma);
}

void FileImporterBibTeX::setKeysForPersonDetection(const QStringList &keylist)
{
    m_keysForPersonDetection.append(keylist);
}

QSharedPointer<Person> FileImporterBibTeX::personFromTokenList(const QStringList &tokens, CommaContainment *comma)
{
    if (comma != NULL) *comma = ccNoComma;

    /// Simple case: provided list of tokens is empty, return invalid Person
    if (tokens.isEmpty())
        return QSharedPointer<Person>();

    /**
     * Sequence of tokens may contain somewhere a comma, like
     * "Tuckwell," "Peter". In this case, fill two string lists:
     * one with tokens before the comma, one with tokens after the
     * comma (excluding the comma itself). Example:
     * partA = ( "Tuckwell" );  partB = ( "Peter" );  partC = ( "Jr." )
     * If a comma was found, boolean variable gotComma is set.
     */
    QStringList partA, partB, partC;
    int commaCount = 0;
    foreach(const QString &token, tokens) {
        /// Position where comma was found, or -1 if no comma in token
        int p = -1;
        if (commaCount < 2) {
            /// Only check if token contains comma
            /// if no comma was found before
            int bracketCounter = 0;
            for (int i = 0; i < token.length(); ++i) {
                /// Consider opening curly brackets
                if (token[i] == QChar('{')) ++bracketCounter;
                /// Consider closing curly brackets
                else if (token[i] == QChar('}')) --bracketCounter;
                /// Only if outside any open curly bracket environments
                /// consider comma characters
                else if (bracketCounter == 0 && token[i] == QChar(',')) {
                    /// Memorize comma's position and break from loop
                    p = i;
                    break;
                } else if (bracketCounter < 0)
                    /// Should never happen: more closing brackets than opening ones
                    kWarning() << "Opening and closing brackets do not match!";
            }
        }

        if (p >= 0) {
            if (commaCount == 0) {
                if (p > 0) partA.append(token.left(p));
                if (p < token.length() - 1) partB.append(token.mid(p + 1));
            } else if (commaCount == 1) {
                if (p > 0) partB.append(token.left(p));
                if (p < token.length() - 1) partC.append(token.mid(p + 1));
            }
            ++commaCount;
        } else if (commaCount == 0)
            partA.append(token);
        else if (commaCount == 1)
            partB.append(token);
        else if (commaCount == 2)
            partC.append(token);
    }
    if (commaCount > 0) {
        if (comma != NULL) *comma = ccContainsComma;
        return QSharedPointer<Person>(new Person(partC.isEmpty() ? partB.join(QChar(' ')) : partC.join(QChar(' ')), partA.join(QChar(' ')), partC.isEmpty() ? QString::null : partB.join(QChar(' '))));
    }

    /**
     * PubMed uses a special writing style for names, where the
     * last name is followed by single capital letters, each being
     * the first letter of each first name. Example: Tuckwell P H
     * So, check how many single capital letters are at the end of
     * the given token list
     */
    partA.clear(); partB.clear();
    bool singleCapitalLetters = true;
    QStringList::ConstIterator it = tokens.constEnd();
    while (it != tokens.constBegin()) {
        --it;
        if (singleCapitalLetters && it->length() == 1 && it->at(0).isUpper())
            partB.prepend(*it);
        else {
            singleCapitalLetters = false;
            partA.prepend(*it);
        }
    }
    if (!partB.isEmpty()) {
        /// Name was actually given in PubMed format
        return QSharedPointer<Person>(new Person(partB.join(QChar(' ')), partA.join(QChar(' '))));
    }

    /**
     * Normally, the last upper case token in a name is the last name
     * (last names consisting of multiple space-separated parts *have*
     * to be protected by {...}), but some languages have fill words
     * in lower caps beloning to the last name as well (example: "van").
     * Exception: Special keywords such as "Jr." can be appended to the
     * name, not counted as part of the last name
     */
    partA.clear(); partB.clear(); partC.clear();
    it = tokens.constEnd();
    while (it != tokens.constBegin()) {
        --it;
        if (partB.isEmpty() && (it->toLower().startsWith(QLatin1String("jr")) || it->toLower().startsWith(QLatin1String("sr")) || it->toLower().startsWith(QLatin1String("iii"))))
            /// handle name suffices like "Jr" or "III."
            partC.prepend(*it);
        else if (partB.isEmpty() || it->at(0).isLower())
            partB.prepend(*it);
        else
            partA.prepend(*it);
    }
    if (!partB.isEmpty()) {
        /// Name was actually like "Peter Ole van der Tuckwell",
        /// split into "Peter Ole" and "van der Tuckwell"
        return QSharedPointer<Person>(new Person(partA.join(QChar(' ')), partB.join(QChar(' ')), partC.isEmpty() ? QString::null : partC.join(QChar(' '))));
    }

    kWarning() << "Don't know how to handle name" << tokens.join(QChar(' '));
    return QSharedPointer<Person>();
}

void FileImporterBibTeX::contextSensitiveSplit(const QString &text, QStringList &segments)
{
    int bracketCounter = 0; ///< keep track of opening and closing brackets: {...}
    QString buffer;
    int len = text.length();
    segments.clear(); ///< empty list for results before proceeding

    for (int pos = 0; pos < len; ++pos) {
        if (text[pos] == '{')
            ++bracketCounter;
        else if (text[pos] == '}')
            --bracketCounter;

        if (text[pos].isSpace() && bracketCounter == 0) {
            if (!buffer.isEmpty()) {
                segments.append(buffer);
                buffer.clear();
            }
        } else
            buffer.append(text[pos]);
    }

    if (!buffer.isEmpty())
        segments.append(buffer);
}

bool FileImporterBibTeX::evaluateParameterComments(QTextStream *textStream, const QString &line, File *file)
{
    /// Assertion: variable "line" is all lower-case

    /** check if this file requests a special encoding */
    if (line.startsWith("@comment{x-kbibtex-encoding=") && line.endsWith("}")) {
        QString encoding = line.mid(28, line.length() - 29);
        textStream->setCodec(encoding == "latex" ? "utf-8" : encoding.toAscii());
        file->setProperty(File::Encoding, encoding == "latex" ? encoding : textStream->codec()->name());
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
