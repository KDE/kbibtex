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
#include <QIODevice>
#include <QRegExp>
#include <QCoreApplication>
#include <QRegExp>
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

#include "fileimporterbibtex.h"

const QString extraAlphaNumChars = QString("?'`-_:.+/$\\\"&");
const QRegExp htmlRegExp = QRegExp("</?(a|pre)[^>]*>", Qt::CaseInsensitive);

FileImporterBibTeX::FileImporterBibTeX(const QString& encoding, bool ignoreComments)
        : FileImporter(), m_cancelFlag(false), m_lineNo(1), m_textStream(NULL), m_currentChar(' '), m_ignoreComments(ignoreComments), m_encoding(encoding)
{
    // nothing
}

FileImporterBibTeX::~FileImporterBibTeX()
{
}

File* FileImporterBibTeX::load(QIODevice *iodevice)
{
    m_mutex.lock();
    QCoreApplication::instance()->processEvents();
    m_cancelFlag = false;

    m_textStream = new QTextStream(iodevice);
    m_textStream->setCodec(m_encoding == "latex" ? "UTF-8" : m_encoding.toAscii());
    QString rawText = "";
    while (!m_textStream->atEnd()) {
        QString line = m_textStream->readLine();
        bool skipline = evaluateParameterComments(m_textStream, line);
        if (!skipline)
            rawText.append(line).append("\n");
    }
    delete m_textStream;

    QCoreApplication::instance()->processEvents();

    /** Remove HTML code from the input source */
    rawText = rawText.replace(htmlRegExp, "");

    rawText = EncoderLaTeX::currentEncoderLaTeX() ->decode(rawText);
    QCoreApplication::instance()->processEvents();

    unescapeLaTeXChars(rawText);
    QCoreApplication::instance()->processEvents();

    m_textStream = new QTextStream(&rawText, QIODevice::ReadOnly);
    m_textStream->setCodec("UTF-8");
    m_lineNo = 1;

    File *result = new File();
    while (!m_cancelFlag && !m_textStream->atEnd()) {
        emit progress(m_textStream->pos(), rawText.length());
        QCoreApplication::instance()->processEvents();
        Element * element = nextElement();

        if (element != NULL) {
            Comment *comment = dynamic_cast<Comment*>(element);
            if (!m_ignoreComments || comment == NULL)
                result->append(element);
            else
                delete element;
        }
        QCoreApplication::instance()->processEvents();
    }
    emit progress(100, 100);

    if (m_cancelFlag) {
        qWarning("Loading file has been canceled");
        delete result;
        result = NULL;
    }

    delete m_textStream;

    m_mutex.unlock();
    return result;
}

bool FileImporterBibTeX::guessCanDecode(const QString & rawText)
{
    QString text = EncoderLaTeX::currentEncoderLaTeX() ->decode(rawText);
    return text.indexOf(QRegExp("@\\w+\\{.+\\}")) >= 0;
}

void FileImporterBibTeX::cancel()
{
    m_cancelFlag = TRUE;
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
        else if (!elementType.isEmpty())
            return readEntryElement(elementType);
        else {
            qWarning("ElementType is empty");
            return NULL;
        }
    } else if (token == tUnknown)
        return readPlainCommentElement();

    if (token != tEOF)
        qWarning() << "Don't know how to parse next token of type " << token << " in line " << m_lineNo << endl;

    return NULL;
}

Comment *FileImporterBibTeX::readCommentElement()
{
    while (m_currentChar != '{' && m_currentChar != '(' && !m_textStream->atEnd()) {
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }

    return new Comment(readBracketString(m_currentChar));
}

Comment *FileImporterBibTeX::readPlainCommentElement()
{
    QString result = readLine();
    if (m_currentChar == '\n') ++m_lineNo;
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd() && m_currentChar != '@' && !m_currentChar.isSpace()) {
        result.append('\n').append(m_currentChar);
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
        result.append(readLine());
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }
    return new Comment(result);
}

Macro *FileImporterBibTeX::readMacroElement()
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qWarning("Error in parsing unknown macro: Opening curly brace ({) expected");
            return NULL;
        }
        token = nextToken();
    }

    QString key = readSimpleString();
    if (nextToken() != tAssign) {
        qCritical() << "Error in parsing macro '" << key << "': Assign symbol (=) expected";
        return NULL;
    }

    Macro *macro = new Macro(key);
    do {
        bool isStringKey = FALSE;
        QString text = readString(isStringKey).replace(QRegExp("\\s+"), " ");
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
            qWarning("Error in parsing unknown preamble: Opening curly brace ({) expected");
            return NULL;
        }
        token = nextToken();
    }

    Preamble *preamble = new Preamble();
    do {
        bool isStringKey = FALSE;
        QString text = readString(isStringKey).replace(QRegExp("\\s+"), " ");
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
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qWarning("Error in parsing unknown entry: Opening curly brace ({) expected");
            return NULL;
        }
        token = nextToken();
    }

    QString key = readSimpleString();
    Entry *entry = new Entry(typeString, key);

    token = nextToken();
    do {
        if (token == tBracketClose || token == tEOF)
            break;
        else if (token != tComma) {
            qCritical() << "Error in parsing entry '" << key << "': Comma symbol (,) expected";
            delete entry;
            return NULL;
        }

        QString keyName = readSimpleString();
        token = nextToken();
        if (keyName == QString::null || token == tBracketClose) {
            // entry is buggy, but we still accept it
            break;
        } else if (token != tAssign) {
            qCritical() << "Error in parsing entry '" << key << "': Assign symbol (=) expected after field name '" << keyName << "'";
            delete entry;
            return NULL;
        }

        /** check for duplicate fields */
        if (entry->contains(keyName)) {
            int i = 1;
            QString appendix = QString::number(i);
            while (entry->contains(keyName + appendix)) {
                ++i;
                appendix = QString::number(i);
            }
            keyName += appendix;
        }

        Value value;
        token = readValue(value, keyName);

        entry->insert(keyName, value);
    } while (true);

    return entry;
}

FileImporterBibTeX::Token FileImporterBibTeX::nextToken()
{
    if (m_textStream->atEnd())
        return tEOF;

    Token curToken = tUnknown;

    while ((m_currentChar.isSpace() || m_currentChar == '\t') && !m_textStream->atEnd()) {
        if (m_currentChar == '\n') ++m_lineNo;
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
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }

    return curToken;
}

QString FileImporterBibTeX::readString(bool &isStringKey)
{
    if (m_currentChar.isSpace()) {
        m_textStream->skipWhiteSpace();
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }

    isStringKey = FALSE;
    switch (m_currentChar.toAscii()) {
    case '{':
    case '(':
        return readBracketString(m_currentChar);
    case '"':
        return readQuotedString();
    default:
        isStringKey = TRUE;
        return readSimpleString();
    }
}

QString FileImporterBibTeX::readSimpleString(QChar until)
{
    QString result;

    while (m_currentChar.isSpace()) {
        m_textStream->skipWhiteSpace();
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }

    if (m_currentChar.isLetterOrNumber() || extraAlphaNumChars.contains(m_currentChar)) {
        result.append(m_currentChar);
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }

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
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }
    return result;
}

QString FileImporterBibTeX::readQuotedString()
{
    QString result;
    QChar lastChar = m_currentChar;
    if (m_currentChar == '\n') ++m_lineNo;
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd()) {
        if (m_currentChar != '"' || lastChar == '\\')
            result.append(m_currentChar);
        else
            break;
        lastChar = m_currentChar;
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }

    /** read character after closing " */
    if (m_currentChar == '\n') ++m_lineNo;
    *m_textStream >> m_currentChar;

    return result;
}

QString FileImporterBibTeX::readLine()
{
    QString result;
    while (!m_textStream->atEnd() && m_currentChar != '\n') {
        result.append(m_currentChar);
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }
    return result;
}

QString FileImporterBibTeX::readBracketString(const QChar openingBracket)
{
    QString result;
    QChar closingBracket = '}';
    if (openingBracket == '(')
        closingBracket = ')';
    int counter = 1;
    if (m_currentChar == '\n') ++m_lineNo;
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
        if (m_currentChar == '\n') ++m_lineNo;
        *m_textStream >> m_currentChar;
    }
    if (m_currentChar == '\n') ++m_lineNo;
    *m_textStream >> m_currentChar;
    return result;
}

FileImporterBibTeX::Token FileImporterBibTeX::readValue(Value& value, const QString& key)
{
    Token token = tUnknown;
    QString iKey = key.toLower();

    do {
        bool isStringKey = FALSE;
        QString text = readString(isStringKey).replace(QRegExp("\\s+"), " ");

        // FIXME: comparisons should be made case-insensitive
        if (iKey == Entry::ftAuthor || iKey == Entry::ftEditor) {
            if (isStringKey)
                value.append(new MacroKey(text));
            else {
                QStringList persons;
                splitPersonList(text, persons);
                for (QStringList::ConstIterator pit = persons.constBegin(); pit != persons.constEnd(); ++pit) {
                    Person *person = splitName(*pit);
                    if (person != NULL)
                        value.append(person);
                }
            }
        } else if (iKey == Entry::ftPages) {
            text.replace(QRegExp("\\s*--?\\s*"), QChar(0x2013));
            if (isStringKey)
                value.append(new MacroKey(text));
            else
                value.append(new PlainText(text));
        }        else {
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
    const QLatin1String sepText[] = {QLatin1String(";"), QLatin1String(",")};
    const int max = sizeof(sepText) / sizeof(sepText[0]);

    for (int i = 0; i < max; ++i)
        if (text.contains(sepText[i])) {
            QRegExp sepRegExp("\\s*" + sepText[0] + "\\s*");
            QStringList token = text.split(sepRegExp, QString::SkipEmptyParts);
            for (QStringList::Iterator it = token.begin(); it != token.end(); ++it)
                result.append(new Keyword(*it));
            break;
        }

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
    bool containsComma = splitName(text, segments);
    QString firstName = "";
    QString lastName = "";

    if (segments.isEmpty())
        return NULL;

    if (!containsComma) {
        /** PubMed uses a special writing style for names, where the last name is followed by single capital letter,
          * each being the first letter of each first name
          * So, check how many single capital letters are at the end of the given segment list */
        int singleCapitalLettersCounter = 0;
        int p = segments.count() - 1;
        while (segments[p].length() == 1 && segments[p].compare(segments[p].toUpper()) == 0) {
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
bool FileImporterBibTeX::splitName(const QString& text, QStringList& segments)
{
    int bracketCounter = 0;
    bool result = FALSE;
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
            result = TRUE;
        } else
            buffer.append(text[pos]);
    }

    if (!buffer.isEmpty())
        segments.append(buffer);

    return result;
}

bool FileImporterBibTeX::evaluateParameterComments(QTextStream *textStream, const QString &line)
{
    /** check if this file requests a special encoding */
    if (line.startsWith("@comment{x-kbibtex-encoding=") && line.endsWith("}")) {
        QString newEncoding = line.mid(28, line.length() - 29);
        qDebug() << "x-kbibtex-encoding=" << newEncoding << endl;
        if (newEncoding == "latex") newEncoding = "UTF-8";
        textStream->setCodec(newEncoding.toAscii());
        return true;
    }

    return false;
}
