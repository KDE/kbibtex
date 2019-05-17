/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileimporterbibtex.h"

#include <QTextCodec>
#include <QIODevice>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QStringList>

#include <BibTeXEntries>
#include <BibTeXFields>
#include <Preferences>
#include <File>
#include <Comment>
#include <Macro>
#include <Preamble>
#include <Entry>
#include <Element>
#include <Value>
#include "encoder.h"
#include "encoderlatex.h"
#include "logging_io.h"

#define qint64toint(a) (static_cast<int>(qMax(0LL,qMin(0x7fffffffLL,(a)))))

FileImporterBibTeX::FileImporterBibTeX(QObject *parent)
        : FileImporter(parent), m_cancelFlag(false), m_textStream(nullptr), m_commentHandling(IgnoreComments), m_keywordCasing(KBibTeX::cLowerCase), m_lineNo(1)
{
    m_keysForPersonDetection.append(Entry::ftAuthor);
    m_keysForPersonDetection.append(Entry::ftEditor);
    m_keysForPersonDetection.append(QStringLiteral("bookauthor")); /// used by JSTOR
}

File *FileImporterBibTeX::load(QIODevice *iodevice)
{
    m_cancelFlag = false;

    if (!iodevice->isReadable() && !iodevice->open(QIODevice::ReadOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device not readable";
        emit message(SeverityError, QStringLiteral("Input device not readable"));
        return nullptr;
    }

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
    m_statistics.countProtectedTitle = 0;
    m_statistics.countUnprotectedTitle = 0;
    m_statistics.mostRecentListSeparator.clear();

    m_textStream = new QTextStream(iodevice);
    m_textStream->setCodec(Preferences::defaultBibTeXEncoding.toLatin1()); ///< unless we learn something else, assume default codec
    result->setProperty(File::Encoding, Preferences::defaultBibTeXEncoding);

    QString rawText;
    rawText.reserve(qint64toint(iodevice->size()));
    while (!m_textStream->atEnd()) {
        QString line = m_textStream->readLine();
        bool skipline = evaluateParameterComments(m_textStream, line.toLower(), result);
        // FIXME XML data should be removed somewhere else? onlinesearch ...
        if (line.startsWith(QStringLiteral("<?xml")) && line.endsWith(QStringLiteral("?>")))
            /// Hop over XML declarations
            skipline = true;
        if (!skipline)
            rawText.append(line).append("\n");
    }

    delete m_textStream;

    /** Remove HTML code from the input source */
    // FIXME HTML data should be removed somewhere else? onlinesearch ...
    const int originalLength = rawText.length();
    rawText = rawText.remove(KBibTeX::htmlRegExp);
    const int afterHTMLremovalLength = rawText.length();
    if (originalLength != afterHTMLremovalLength) {
        qCInfo(LOG_KBIBTEX_IO) << (originalLength - afterHTMLremovalLength) << "characters of HTML tags have been removed";
        emit message(SeverityInfo, QString(QStringLiteral("%1 characters of HTML tags have been removed")).arg(originalLength - afterHTMLremovalLength));
    }

    // TODO really necessary to pipe data through several QTextStreams?
    m_textStream = new QTextStream(&rawText, QIODevice::ReadOnly);
    m_textStream->setCodec(Preferences::defaultBibTeXEncoding.toLower() == QStringLiteral("latex") ? "us-ascii" : Preferences::defaultBibTeXEncoding.toLatin1());
    m_lineNo = 1;
    m_prevLine = m_currentLine = QString();
    m_knownElementIds.clear();
    readChar();

    while (!m_nextChar.isNull() && !m_cancelFlag && !m_textStream->atEnd()) {
        emit progress(qint64toint(m_textStream->pos()), rawText.length());
        Element *element = nextElement();

        if (element != nullptr) {
            if (m_commentHandling == KeepComments || !Comment::isComment(*element))
                result->append(QSharedPointer<Element>(element));
            else
                delete element;
        }
    }
    emit progress(100, 100);

    if (m_cancelFlag) {
        qCWarning(LOG_KBIBTEX_IO) << "Loading bibliography data has been canceled";
        emit message(SeverityError, QStringLiteral("Loading bibliography data has been canceled"));
        delete result;
        result = nullptr;
    }

    delete m_textStream;

    if (result != nullptr) {
        /// Set the file's preferences for string delimiters
        /// deduced from statistics built while parsing the file
        result->setProperty(File::StringDelimiter, m_statistics.countQuotationMarks > m_statistics.countCurlyBrackets ? QStringLiteral("\"\"") : QStringLiteral("{}"));
        /// Set the file's preferences for name formatting
        result->setProperty(File::NameFormatting, m_statistics.countFirstNameFirst > m_statistics.countLastNameFirst ? Preferences::personNameFormatFirstLast : Preferences::personNameFormatLastFirst);
        /// Set the file's preferences for title protected
        Qt::CheckState triState = (m_statistics.countProtectedTitle > m_statistics.countUnprotectedTitle * 4) ? Qt::Checked : ((m_statistics.countProtectedTitle * 4 < m_statistics.countUnprotectedTitle) ? Qt::Unchecked : Qt::PartiallyChecked);
        result->setProperty(File::ProtectCasing, static_cast<int>(triState));
        /// Set the file's preferences for quoting of comments
        if (m_statistics.countNoCommentQuote > m_statistics.countCommentCommand && m_statistics.countNoCommentQuote > m_statistics.countCommentPercent)
            result->setProperty(File::QuoteComment, static_cast<int>(Preferences::qcNone));
        else if (m_statistics.countCommentCommand > m_statistics.countNoCommentQuote && m_statistics.countCommentCommand > m_statistics.countCommentPercent)
            result->setProperty(File::QuoteComment, static_cast<int>(Preferences::qcCommand));
        else
            result->setProperty(File::QuoteComment, static_cast<int>(Preferences::qcPercentSign));
        if (!m_statistics.mostRecentListSeparator.isEmpty())
            result->setProperty(File::ListSeparator, m_statistics.mostRecentListSeparator);
        // TODO gather more statistics for keyword casing etc.
    }

    iodevice->close();
    return result;
}

bool FileImporterBibTeX::guessCanDecode(const QString &rawText)
{
    static const QRegularExpression bibtexLikeText(QStringLiteral("@\\w+\\{.+\\}"));
    QString text = EncoderLaTeX::instance().decode(rawText);
    return bibtexLikeText.match(text).hasMatch();
}

void FileImporterBibTeX::cancel()
{
    m_cancelFlag = true;
}

Element *FileImporterBibTeX::nextElement()
{
    Token token = nextToken();

    if (token == tAt) {
        const QString elementType = readSimpleString();
        const QString elementTypeLower = elementType.toLower();

        if (elementTypeLower == QStringLiteral("comment")) {
            ++m_statistics.countCommentCommand;
            return readCommentElement();
        } else if (elementTypeLower == QStringLiteral("string"))
            return readMacroElement();
        else if (elementTypeLower == QStringLiteral("preamble"))
            return readPreambleElement();
        else if (elementTypeLower == QStringLiteral("import")) {
            qCDebug(LOG_KBIBTEX_IO) << "Skipping potential HTML/JavaScript @import statement near line" << m_lineNo;
            emit message(SeverityInfo, QString(QStringLiteral("Skipping potential HTML/JavaScript @import statement near line %1")).arg(m_lineNo));
            return nullptr;
        } else if (!elementType.isEmpty())
            return readEntryElement(elementType);
        else {
            qCWarning(LOG_KBIBTEX_IO) << "Element type after '@' is empty or invalid near line" << m_lineNo;
            emit message(SeverityError, QString(QStringLiteral("Element type after '@' is empty or invalid near line %1")).arg(m_lineNo));
            return nullptr;
        }
    } else if (token == tUnknown && m_nextChar == QLatin1Char('%')) {
        /// do not complain about LaTeX-like comments, just eat them
        ++m_statistics.countCommentPercent;
        return readPlainCommentElement(QString());
    } else if (token == tUnknown) {
        if (m_nextChar.isLetter()) {
            qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << m_nextChar << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << ")" << ", treating as comment";
            emit message(SeverityInfo, QString(QStringLiteral("Unknown character '%1' near line %2, treating as comment")).arg(m_nextChar).arg(m_lineNo));
        } else if (m_nextChar.isPrint()) {
            qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << m_nextChar << "(" << QString(QStringLiteral("0x%1")).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')) << ") near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << ")" << ", treating as comment";
            emit message(SeverityInfo, QString(QStringLiteral("Unknown character '%1' (0x%2) near line %3, treating as comment")).arg(m_nextChar).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(m_lineNo));
        } else {
            qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << QString(QStringLiteral("0x%1")).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')) << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << ")" << ", treating as comment";
            emit message(SeverityInfo, QString(QStringLiteral("Unknown character 0x%1 near line %2, treating as comment")).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(m_lineNo));
        }
        ++m_statistics.countNoCommentQuote;
        return readPlainCommentElement(QString(m_prevChar) + m_nextChar);
    }

    if (token != tEOF) {
        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to parse next token of type" << tokenidToString(token) << "in line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << ")" << endl;
        emit message(SeverityError, QString(QStringLiteral("Don't know how to parse next token of type %1 in line %2")).arg(tokenidToString(token)).arg(m_lineNo));
    }

    return nullptr;
}

Comment *FileImporterBibTeX::readCommentElement()
{
    if (!readCharUntil(QStringLiteral("{(")))
        return nullptr;
    return new Comment(EncoderLaTeX::instance().decode(readBracketString()));
}

Comment *FileImporterBibTeX::readPlainCommentElement(const QString &prefix)
{
    QString result = EncoderLaTeX::instance().decode(prefix + readLine());
    while (m_nextChar == QLatin1Char('\n') || m_nextChar == QLatin1Char('\r')) readChar();
    while (!m_nextChar.isNull() && m_nextChar != QLatin1Char('@')) {
        const QChar nextChar = m_nextChar;
        const QString line = readLine();
        while (m_nextChar == QLatin1Char('\n') || m_nextChar == QLatin1Char('\r')) readChar();
        result.append(EncoderLaTeX::instance().decode((nextChar == QLatin1Char('%') ? QString() : QString(nextChar)) + line));
    }

    if (result.startsWith(QStringLiteral("x-kbibtex"))) {
        qCWarning(LOG_KBIBTEX_IO) << "Plain comment element starts with 'x-kbibtex', this should not happen";
        emit message(SeverityWarning, QStringLiteral("Plain comment element starts with 'x-kbibtex', this should not happen"));
        /// ignore special comments
        return nullptr;
    }

    return new Comment(result);
}

Macro *FileImporterBibTeX::readMacroElement()
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing macro near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Opening curly brace '{' expected";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing macro near line %1: Opening curly brace '{' expected")).arg(m_lineNo));
            return nullptr;
        }
        token = nextToken();
    }

    QString key = readSimpleString();

    if (key.isEmpty()) {
        /// Cope with empty keys,
        /// duplicates are handled further below
        key = QStringLiteral("EmptyId");
    } else if (!Encoder::containsOnlyAscii(key)) {
        /// Try to avoid non-ascii characters in ids
        const QString newKey = Encoder::instance().convertToPlainAscii(key);
        qCWarning(LOG_KBIBTEX_IO) << "Macro key" << key << "near line" << m_lineNo << "contains non-ASCII characters, converted to" << newKey;
        emit message(SeverityWarning, QString(QStringLiteral("Macro key '%1'  near line %2 contains non-ASCII characters, converted to '%3'")).arg(key).arg(m_lineNo).arg(newKey));
        key = newKey;
    }

    /// Check for duplicate entry ids, avoid collisions
    if (m_knownElementIds.contains(key)) {
        static const QString newIdPattern = QStringLiteral("%1-%2");
        int idx = 2;
        QString newKey = newIdPattern.arg(key).arg(idx);
        while (m_knownElementIds.contains(newKey))
            newKey = newIdPattern.arg(key).arg(++idx);
        qCDebug(LOG_KBIBTEX_IO) << "Duplicate macro key" << key << ", using replacement key" << newKey;
        emit message(SeverityWarning, QString(QStringLiteral("Duplicate macro key '%1', using replacement key '%2'")).arg(key, newKey));
        key = newKey;
    }
    m_knownElementIds.insert(key);

    if (nextToken() != tAssign) {
        qCCritical(LOG_KBIBTEX_IO) << "Error in parsing macro" << key << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Assign symbol '=' expected";
        emit message(SeverityError, QString(QStringLiteral("Error in parsing macro '%1' near line %2: Assign symbol '=' expected")).arg(key).arg(m_lineNo));
        return nullptr;
    }

    Macro *macro = new Macro(key);
    do {
        bool isStringKey = false;
        QString text = readString(isStringKey);
        if (text.isNull()) {
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing macro" << key << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Could not read macro's text";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing macro '%1' near line %2: Could not read macro's text")).arg(key).arg(m_lineNo));
            delete macro;
        }
        text = EncoderLaTeX::instance().decode(bibtexAwareSimplify(text));
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
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing preamble near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Opening curly brace '{' expected";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing preamble near line %1: Opening curly brace '{' expected")).arg(m_lineNo));
            return nullptr;
        }
        token = nextToken();
    }

    Preamble *preamble = new Preamble();
    do {
        bool isStringKey = false;
        QString text = readString(isStringKey);
        if (text.isNull()) {
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing preamble near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Could not read preamble's text";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing preamble near line %1: Could not read preamble's text")).arg(m_lineNo));
            delete preamble;
            return nullptr;
        }
        /// Remember: strings from preamble do not get encoded,
        /// may contain raw LaTeX commands and code
        text = bibtexAwareSimplify(text);
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
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Opening curly brace '{' expected";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing entry near line %1: Opening curly brace '{' expected")).arg(m_lineNo));
            return nullptr;
        }
        token = nextToken();
    }

    QString id = readSimpleString(QStringLiteral(",}"), true).trimmed();
    if (id.isEmpty()) {
        if (m_nextChar == QLatin1Char(',') || m_nextChar == QLatin1Char('}')) {
            /// Cope with empty ids,
            /// duplicates are handled further below
            id = QStringLiteral("EmptyId");
        }
        else {
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine << "): Could not read entry id";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing preambentryle near line %1: Could not read entry id")).arg(m_lineNo));
            return nullptr;
        }
    } else {
        if (id.contains(QStringLiteral("\\")) || id.contains(QStringLiteral("{"))) {
            const QString newId = EncoderLaTeX::instance().decode(id);
            qCWarning(LOG_KBIBTEX_IO) << "Entry id" << id << "near line" << m_lineNo << "contains backslashes or curly brackets, converted to" << newId;
            emit message(SeverityWarning, QString(QStringLiteral("Entry id '%1' near line %2 contains backslashes or curly brackets, converted to '%3'")).arg(id).arg(m_lineNo).arg(newId));
            id = newId;
        }
        if (!Encoder::containsOnlyAscii(id)) {
            /// Try to avoid non-ascii characters in ids
            const QString newId = Encoder::instance().convertToPlainAscii(id);
            qCWarning(LOG_KBIBTEX_IO) << "Entry id" << id << "near line" << m_lineNo << "contains non-ASCII characters, converted to" << newId;
            emit message(SeverityWarning, QString(QStringLiteral("Entry id '%1' near line %2 contains non-ASCII characters, converted to '%3'")).arg(id).arg(m_lineNo).arg(newId));
            id = newId;
        }
    }
    static const QVector<QChar> invalidIdCharacters = {QLatin1Char('{'), QLatin1Char('}'), QLatin1Char(',')};
    for (const QChar &invalidIdCharacter : invalidIdCharacters)
        if (id.contains(invalidIdCharacter)) {
            qCWarning(LOG_KBIBTEX_IO) << "Entry id" << id << "near line" << m_lineNo << "contains invalid character" << invalidIdCharacter;
            emit message(SeverityError, QString(QStringLiteral("Entry id '%1' near line %2 contains invalid character '%3'")).arg(id).arg(m_lineNo).arg(invalidIdCharacter));
            return nullptr;
        }

    /// Check for duplicate entry ids, avoid collisions
    if (m_knownElementIds.contains(id)) {
        static const QString newIdPattern = QStringLiteral("%1-%2");
        int idx = 2;
        QString newId = newIdPattern.arg(id).arg(idx);
        while (m_knownElementIds.contains(newId))
            newId = newIdPattern.arg(id).arg(++idx);
        qCDebug(LOG_KBIBTEX_IO) << "Duplicate id" << id << "near line" << m_lineNo << ", using replacement id" << newId;
        emit message(SeverityInfo, QString(QStringLiteral("Duplicate id '%1' near line %2, using replacement id '%3'")).arg(id).arg(m_lineNo).arg(newId));
        id = newId;
    }
    m_knownElementIds.insert(id);

    Entry *entry = new Entry(BibTeXEntries::instance().format(typeString, m_keywordCasing), id);

    token = nextToken();
    do {
        if (token == tBracketClose)
            break;
        else if (token == tEOF) {
            qCWarning(LOG_KBIBTEX_IO) << "Unexpected end of data in entry" << id << "near line" << m_lineNo << ":" << m_prevLine << endl << m_currentLine;
            emit message(SeverityError, QString(QStringLiteral("Unexpected end of data in entry '%1' near line %2")).arg(id).arg(m_lineNo));
            delete entry;
            return nullptr;
        } else if (token != tComma) {
            if (m_nextChar.isLetter()) {
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Comma symbol ',' expected but got character" << m_nextChar << "(token" << tokenidToString(token) << ")";
                emit message(SeverityError, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Comma symbol ',' expected but got character '%3' (token %4)")).arg(id).arg(m_lineNo).arg(m_nextChar).arg(tokenidToString(token)));
            } else if (m_nextChar.isPrint()) {
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Comma symbol ',' expected but got character" << m_nextChar << "(" << QString(QStringLiteral("0x%1")).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')) << ", token" << tokenidToString(token) << ")";
                emit message(SeverityError, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Comma symbol ',' expected but got character '%3' (0x%4, token %5)")).arg(id).arg(m_lineNo).arg(m_nextChar).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(tokenidToString(token)));
            } else {
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Comma symbol (,) expected but got character" << QString(QStringLiteral("0x%1")).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')) << "(token" << tokenidToString(token) << ")";
                emit message(SeverityError, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Comma symbol ',' expected but got character 0x%3 (token %4)")).arg(id).arg(m_lineNo).arg(m_nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(tokenidToString(token)));
            }
            delete entry;
            return nullptr;
        }

        QString keyName = BibTeXFields::instance().format(readSimpleString(), m_keywordCasing);
        if (keyName.isEmpty()) {
            token = nextToken();
            if (token == tBracketClose) {
                /// Most often it is the case that the previous line ended with a comma,
                /// implying that this entry continues, but instead it gets closed by
                /// a closing curly bracket.
                qCDebug(LOG_KBIBTEX_IO) << "Issue while parsing entry" << id << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Last key-value pair ended with a non-conformant comma, ignoring that";
                emit message(SeverityInfo, QString(QStringLiteral("Issue while parsing entry '%1' near line %2: Last key-value pair ended with a non-conformant comma, ignoring that")).arg(id).arg(m_lineNo));
                break;
            } else {
                /// Something looks terribly wrong
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "): Closing curly bracket expected, but found" << tokenidToString(token);
                emit message(SeverityError, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Closing curly bracket expected, but found %3")).arg(id).arg(m_lineNo).arg(tokenidToString(token)));
                delete entry;
                return nullptr;
            }
        }
        /// Try to avoid non-ascii characters in keys
        const QString newkeyName = Encoder::instance().convertToPlainAscii(keyName);
        if (newkeyName != keyName) {
            qCWarning(LOG_KBIBTEX_IO) << "Field name " << keyName << "near line" << m_lineNo << "contains non-ASCII characters, converted to" << newkeyName;
            emit message(SeverityWarning, QString(QStringLiteral("Field name '%1' near line %2 contains non-ASCII characters, converted to '%3'")).arg(keyName).arg(m_lineNo).arg(newkeyName));
            keyName = newkeyName;
        }

        token = nextToken();
        if (token != tAssign) {
            qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << ", field name" << keyName << "near line" << m_lineNo  << "(" << m_prevLine << endl << m_currentLine << "): Assign symbol '=' expected after field name";
            emit message(SeverityError, QString(QStringLiteral("Error in parsing entry '%1', field name '%2' near line %3: Assign symbol '=' expected after field name")).arg(id, keyName).arg(m_lineNo));
            delete entry;
            return nullptr;
        }

        Value value;

        /// check for duplicate fields
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
                qCDebug(LOG_KBIBTEX_IO) << "Entry" << id << "already contains a key" << keyName << "near line" << m_lineNo << "(" << m_prevLine << endl << m_currentLine << "), using" << (keyName + appendix);
                emit message(SeverityWarning, QString(QStringLiteral("Entry '%1' already contains a key '%2' near line %4, using '%3'")).arg(id, keyName, keyName + appendix).arg(m_lineNo));
                keyName += appendix;
            }
        }

        token = readValue(value, keyName);
        if (token != tBracketClose && token != tComma) {
            qCWarning(LOG_KBIBTEX_IO) << "Failed to read value in entry" << id << ", field name" << keyName << "near line" << m_lineNo  << "(" << m_prevLine << endl << m_currentLine << ")";
            emit message(SeverityError, QString(QStringLiteral("Failed to read value in entry '%1', field name '%2' near line %3")).arg(id, keyName).arg(m_lineNo));
            delete entry;
            return nullptr;
        }

        entry->insert(keyName, value);
    } while (true);

    return entry;
}

FileImporterBibTeX::Token FileImporterBibTeX::nextToken()
{
    if (!skipWhiteChar()) {
        /// Some error occurred while reading from data stream
        return tEOF;
    }

    Token result = tUnknown;

    switch (m_nextChar.toLatin1()) {
    case '@':
        result = tAt;
        break;
    case '{':
    case '(':
        result = tBracketOpen;
        break;
    case '}':
    case ')':
        result = tBracketClose;
        break;
    case ',':
        result = tComma;
        break;
    case '=':
        result = tAssign;
        break;
    case '#':
        result = tDoublecross;
        break;
    default:
        if (m_textStream->atEnd())
            result = tEOF;
    }

    if (m_nextChar != QLatin1Char('%')) {
        /// Unclean solution, but necessary for comments
        /// that have a percent sign as a prefix
        readChar();
    }
    return result;
}

QString FileImporterBibTeX::readString(bool &isStringKey)
{
    /// Most often it is not a string key
    isStringKey = false;

    if (!skipWhiteChar()) {
        /// Some error occurred while reading from data stream
        return QString::null;
    }

    switch (m_nextChar.toLatin1()) {
    case '{':
    case '(': {
        ++m_statistics.countCurlyBrackets;
        const QString result = readBracketString();
        return result;
    }
    case '"': {
        ++m_statistics.countQuotationMarks;
        const QString result = readQuotedString();
        return result;
    }
    default:
        isStringKey = true;
        const QString result = readSimpleString();
        return result;
    }
}

QString FileImporterBibTeX::readSimpleString(const QString &until, const bool readNestedCurlyBrackets)
{
    static const QString extraAlphaNumChars = QString(QStringLiteral("?'`-_:.+/$\\\"&"));

    QString result; ///< 'result' is Null on purpose: simple strings cannot be empty in contrast to e.g. quoted strings

    if (!skipWhiteChar()) {
        /// Some error occurred while reading from data stream
        return QString::null;
    }

    QChar prevChar = QChar(0x00);
    while (!m_nextChar.isNull()) {
        if (readNestedCurlyBrackets && m_nextChar == QLatin1Char('{') && prevChar != QLatin1Char('\\')) {
            int depth = 1;
            while (depth > 0) {
                result.append(m_nextChar);
                prevChar = m_nextChar;
                if (!readChar()) return result;
                if (m_nextChar == QLatin1Char('{') && prevChar != QLatin1Char('\\')) ++depth;
                else if (m_nextChar == QLatin1Char('}') && prevChar != QLatin1Char('\\')) --depth;
            }
            result.append(m_nextChar);
            prevChar = m_nextChar;
            if (!readChar()) return result;
        }

        const ushort nextCharUnicode = m_nextChar.unicode();
        if (!until.isEmpty()) {
            /// Variable "until" has user-defined value
            if (m_nextChar == QLatin1Char('\n') || m_nextChar == QLatin1Char('\r') || until.contains(m_nextChar)) {
                /// Force break on line-breaks or if one of the "until" chars has been read
                break;
            } else {
                /// Append read character to final result
                result.append(m_nextChar);
            }
        } else if ((nextCharUnicode >= (ushort)'a' && nextCharUnicode <= (ushort)'z') || (nextCharUnicode >= (ushort)'A' && nextCharUnicode <= (ushort)'Z') || (nextCharUnicode >= (ushort)'0' && nextCharUnicode <= (ushort)'9') || extraAlphaNumChars.contains(m_nextChar)) {
            /// Accept default set of alpha-numeric characters
            result.append(m_nextChar);
        } else
            break;
        prevChar = m_nextChar;
        if (!readChar()) break;
    }
    return result;
}

QString FileImporterBibTeX::readQuotedString()
{
    QString result(0, QChar()); ///< Construct an empty but non-null string

    Q_ASSERT_X(m_nextChar == QLatin1Char('"'), "QString FileImporterBibTeX::readQuotedString()", "m_nextChar is not '\"'");

    if (!readChar()) return QString::null;

    while (!m_nextChar.isNull()) {
        if (m_nextChar == QLatin1Char('"') && m_prevChar != QLatin1Char('\\') && m_prevChar != QLatin1Char('{'))
            break;
        else
            result.append(m_nextChar);

        if (!readChar()) return QString::null;
    }

    if (!readChar()) return QString::null;

    /// Remove protection around quotation marks
    result.replace(QStringLiteral("{\"}"), QStringLiteral("\""));

    return result;
}

QString FileImporterBibTeX::readBracketString()
{
    static const QChar backslash = QLatin1Char('\\');
    QString result(0, QChar()); ///< Construct an empty but non-null string
    const QChar openingBracket = m_nextChar;
    const QChar closingBracket = openingBracket == QLatin1Char('{') ? QLatin1Char('}') : (openingBracket == QLatin1Char('(') ? QLatin1Char(')') : QChar());
    Q_ASSERT_X(!closingBracket.isNull(), "QString FileImporterBibTeX::readBracketString()", "openingBracket==m_nextChar is neither '{' nor '('");
    int counter = 1;

    if (!readChar()) return QString::null;

    while (!m_nextChar.isNull()) {
        if (m_nextChar == openingBracket && m_prevChar != backslash)
            ++counter;
        else if (m_nextChar == closingBracket && m_prevChar != backslash)
            --counter;

        if (counter == 0) {
            break;
        } else
            result.append(m_nextChar);

        if (!readChar()) return QString::null;
    }

    if (!readChar()) return QString::null;
    return result;
}

FileImporterBibTeX::Token FileImporterBibTeX::readValue(Value &value, const QString &key)
{
    Token token = tUnknown;
    const QString iKey = key.toLower();
    static const QSet<QString> verbatimKeys {Entry::ftColor.toLower(), Entry::ftCrossRef.toLower(), Entry::ftXData.toLower()};

    do {
        bool isStringKey = false;
        const QString rawText = readString(isStringKey);
        if (rawText.isNull())
            return tEOF;
        QString text = EncoderLaTeX::instance().decode(rawText);
        /// for all entries except for abstracts ...
        if (iKey != Entry::ftAbstract && !(iKey.startsWith(Entry::ftUrl) && !iKey.startsWith(Entry::ftUrlDate)) && !iKey.startsWith(Entry::ftLocalFile) && !iKey.startsWith(Entry::ftFile)) {
            /// ... remove redundant spaces including newlines
            text = bibtexAwareSimplify(text);
        }
        /// abstracts will keep their formatting (regarding line breaks)
        /// as requested by Thomas Jensch via mail (20 October 2010)

        /// Maintain statistics on if (book) titles are protected
        /// by surrounding curly brackets
        if (iKey == Entry::ftTitle || iKey == Entry::ftBookTitle) {
            if (text[0] == QLatin1Char('{') && text[text.length() - 1] == QLatin1Char('}'))
                ++m_statistics.countProtectedTitle;
            else
                ++m_statistics.countUnprotectedTitle;
        }

        if (m_keysForPersonDetection.contains(iKey)) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                CommaContainment comma = ccContainsComma;
                parsePersonList(text, value, &comma, m_lineNo, this);

                /// Update statistics on name formatting
                if (comma == ccContainsComma)
                    ++m_statistics.countLastNameFirst;
                else
                    ++m_statistics.countFirstNameFirst;
            }
        } else if (iKey == Entry::ftPages) {
            static const QRegularExpression rangeInAscii(QStringLiteral("\\s*--?\\s*"));
            text.replace(rangeInAscii, QChar(0x2013));
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                value.append(QSharedPointer<PlainText>(new PlainText(text)));
        } else if ((iKey.startsWith(Entry::ftUrl) && !iKey.startsWith(Entry::ftUrlDate)) || iKey.startsWith(Entry::ftLocalFile) || iKey.startsWith(Entry::ftFile) || iKey == QStringLiteral("ee") || iKey == QStringLiteral("biburl")) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                /// Assumption: in fields like Url or LocalFile, file names are separated by ;
                static const QRegularExpression semicolonSpace = QRegularExpression(QStringLiteral("[;]\\s*"));
                const QStringList fileList = rawText.split(semicolonSpace, QString::SkipEmptyParts);
                for (const QString &filename : fileList) {
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
                const QRegularExpressionMatch match = KBibTeX::mendeleyFileRegExp.match(rawText);
                if (match.hasMatch()) {
                    static const QString backslashLaTeX = QStringLiteral("$\\backslash$");
                    QString filename = match.captured(1).remove(backslashLaTeX);
                    if (filename.startsWith(QStringLiteral("home/")) || filename.startsWith(QStringLiteral("Users/"))) {
                        /// Mendeley doesn't have a slash at the beginning of absolute paths,
                        /// so, insert one
                        /// See bug 19833, comment 5: https://gna.org/bugs/index.php?19833#comment5
                        filename.prepend(QLatin1Char('/'));
                    }
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(filename)));
                } else
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
            }
        } else if (iKey == Entry::ftMonth) {
            if (isStringKey) {
                static const QRegularExpression monthThreeChars(QStringLiteral("^[a-z]{3}"), QRegularExpression::CaseInsensitiveOption);
                if (monthThreeChars.match(text).hasMatch())
                    text = text.left(3).toLower();
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            } else
                value.append(QSharedPointer<PlainText>(new PlainText(text)));
        } else if (iKey.startsWith(Entry::ftDOI)) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                /// Take care of "; " which separates multiple DOIs, but which may baffle the regexp
                QString preprocessedText = rawText;
                preprocessedText.replace(QStringLiteral("; "), QStringLiteral(" "));
                /// Extract everything that looks like a DOI using a regular expression,
                /// ignore everything else
                QRegularExpressionMatchIterator doiRegExpMatchIt = KBibTeX::doiRegExp.globalMatch(preprocessedText);
                while (doiRegExpMatchIt.hasNext()) {
                    const QRegularExpressionMatch doiRegExpMatch = doiRegExpMatchIt.next();
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured(0))));
                }
            }
        } else if (iKey == Entry::ftKeywords) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else {
                char splitChar;
                const QList<QSharedPointer<Keyword> > keywords = splitKeywords(text, &splitChar);
                for (const auto &keyword : keywords)
                    value.append(keyword);
                /// Memorize (some) split characters for later use
                /// (e.g. when writing file again)
                if (splitChar == ';')
                    m_statistics.mostRecentListSeparator = QStringLiteral("; ");
                else if (splitChar == ',')
                    m_statistics.mostRecentListSeparator = QStringLiteral(", ");

            }
        } else if (verbatimKeys.contains(iKey)) {
            if (isStringKey)
                value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                value.append(QSharedPointer<VerbatimText>(new VerbatimText(rawText)));
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

bool FileImporterBibTeX::readChar()
{
    /// Memorize previous char
    m_prevChar = m_nextChar;

    if (m_textStream->atEnd()) {
        /// At end of data stream
        m_nextChar = QChar::Null;
        return false;
    }

    /// Read next char
    *m_textStream >> m_nextChar;

    /// Test for new line
    if (m_nextChar == QLatin1Char('\n')) {
        /// Update variables tracking line numbers and line content
        ++m_lineNo;
        m_prevLine = m_currentLine;
        m_currentLine.clear();
    } else {
        /// Add read char to current line
        m_currentLine.append(m_nextChar);
    }

    return true;
}

bool  FileImporterBibTeX::readCharUntil(const QString &until)
{
    Q_ASSERT_X(!until.isEmpty(), "bool  FileImporterBibTeX::readCharUntil(const QString &until)", "\"until\" is empty or invalid");
    bool result = true;
    while (!until.contains(m_nextChar) && (result = readChar()));
    return result;
}

bool FileImporterBibTeX::skipWhiteChar()
{
    bool result = true;
    while ((m_nextChar.isSpace() || m_nextChar == QLatin1Char('\t') || m_nextChar == QLatin1Char('\n') || m_nextChar == QLatin1Char('\r')) && result) result = readChar();
    return result;
}

QString FileImporterBibTeX::readLine()
{
    QString result;
    while (m_nextChar != QLatin1Char('\n') && m_nextChar != QLatin1Char('\r') && readChar())
        result.append(m_nextChar);
    return result;
}

QList<QSharedPointer<Keyword> > FileImporterBibTeX::splitKeywords(const QString &text, char *usedSplitChar)
{
    QList<QSharedPointer<Keyword> > result;
    static const QHash<char, QRegularExpression> splitAlong = {
        {'\n', QRegularExpression(QStringLiteral("\\s*\n\\s*"))},
        {';', QRegularExpression(QStringLiteral("\\s*;\\s*"))},
        {',', QRegularExpression(QString("\\s*,\\s*"))}
    };
    if (usedSplitChar != nullptr)
        *usedSplitChar = '\0';

    for (auto it = splitAlong.constBegin(); it != splitAlong.constEnd(); ++it) {
        /// check if character is contained in text (should be cheap to test)
        if (text.contains(QLatin1Char(it.key()))) {
            /// split text along a pattern like spaces-splitchar-spaces
            /// extract keywords
            static const QRegularExpression unneccessarySpacing(QStringLiteral("[ \n\r\t]+"));
            const QStringList keywords = text.split(it.value(), QString::SkipEmptyParts).replaceInStrings(unneccessarySpacing, QStringLiteral(" "));
            /// build QList of Keyword objects from keywords
            for (const QString &keyword : keywords) {
                result.append(QSharedPointer<Keyword>(new Keyword(keyword)));
            }
            /// Memorize (some) split characters for later use
            /// (e.g. when writing file again)
            if (usedSplitChar != nullptr)
                *usedSplitChar = it.key();
            /// no more splits necessary
            break;
        }
    }

    /// no split was performed, so whole text must be a single keyword
    if (result.isEmpty())
        result.append(QSharedPointer<Keyword>(new Keyword(text)));

    return result;
}

QList<QSharedPointer<Person> > FileImporterBibTeX::splitNames(const QString &text, const int line_number, QObject *parent)
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
    /// Case: Smith, John ;  Johnson, Tim ;  Fulkerson, Ford (IEEE Xplore)
    /// German case: Robert A. Gehring und Bernd Lutterbeck

    QString internalText = text;

    /// Remove invalid characters such as dots or (double) daggers for footnotes
    static const QList<QChar> invalidChars {QChar(0x00b7), QChar(0x2020), QChar(0x2217), QChar(0x2021), QChar(0x002a), QChar(0x21d1) /** Upwards double arrow */};
    for (const auto &invalidChar : invalidChars)
        /// Replacing daggers with commas ensures that they act as persons' names separator
        internalText = internalText.replace(invalidChar, QChar(','));
    /// Remove numbers to footnotes
    static const QRegularExpression numberFootnoteRegExp(QStringLiteral("(\\w)\\d+\\b"));
    internalText = internalText.replace(numberFootnoteRegExp, QStringLiteral("\\1"));
    /// Remove academic degrees
    static const QRegularExpression academicDegreesRegExp(QStringLiteral("(,\\s*)?(MA|PhD)\\b"));
    internalText = internalText.remove(academicDegreesRegExp);
    /// Remove email addresses
    static const QRegularExpression emailAddressRegExp(QStringLiteral("\\b[a-zA-Z0-9][a-zA-Z0-9._-]+[a-zA-Z0-9]@[a-z0-9][a-z0-9-]*([.][a-z0-9-]+)*([.][a-z]+)+\\b"));
    internalText = internalText.remove(emailAddressRegExp);

    /// Split input string into tokens which are either name components (first or last name)
    /// or full names (composed of first and last name), depending on the input string's structure
    static const QRegularExpression split(QStringLiteral("\\s*([,]+|[,]*\\b[au]nd\\b|[;]|&|\\n|\\s{4,})\\s*"));
    const QStringList authorTokenList = internalText.split(split, QString::SkipEmptyParts);

    bool containsSpace = true;
    for (QStringList::ConstIterator it = authorTokenList.constBegin(); containsSpace && it != authorTokenList.constEnd(); ++it)
        containsSpace = (*it).contains(QChar(' '));

    QList<QSharedPointer<Person> > result;
    result.reserve(authorTokenList.size());
    if (containsSpace) {
        /// Tokens look like "John Smith"
        for (const QString &authorToken : authorTokenList) {
            QSharedPointer<Person> person = personFromString(authorToken, nullptr, line_number, parent);
            if (!person.isNull())
                result.append(person);
        }
    } else {
        /// Tokens look like "Smith" or "John"
        /// Assumption: two consecutive tokens form a name
        for (QStringList::ConstIterator it = authorTokenList.constBegin(); it != authorTokenList.constEnd(); ++it) {
            QString lastname = *it;
            ++it;
            if (it != authorTokenList.constEnd()) {
                lastname += QStringLiteral(", ") + (*it);
                QSharedPointer<Person> person = personFromString(lastname, nullptr, line_number, parent);
                if (!person.isNull())
                    result.append(person);
            } else
                break;
        }
    }

    return result;
}

void FileImporterBibTeX::parsePersonList(const QString &text, Value &value, const int line_number, QObject *parent)
{
    parsePersonList(text, value, nullptr, line_number, parent);
}

void FileImporterBibTeX::parsePersonList(const QString &text, Value &value, CommaContainment *comma, const int line_number, QObject *parent)
{
    static const QString tokenAnd = QStringLiteral("and");
    static const QString tokenOthers = QStringLiteral("others");
    static QStringList tokens;
    contextSensitiveSplit(text, tokens);

    if (tokens.count() > 0) {
        if (tokens[0] == tokenAnd) {
            qCInfo(LOG_KBIBTEX_IO) << "Person list starts with" << tokenAnd << "near line" << line_number;
            if (parent != nullptr)
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Person list starts with 'and' near line %1")).arg(line_number)));
        } else if (tokens.count() > 1 && tokens[tokens.count() - 1] == tokenAnd) {
            qCInfo(LOG_KBIBTEX_IO) << "Person list ends with" << tokenAnd << "near line" << line_number;
            if (parent != nullptr)
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Person list ends with 'and' near line %1")).arg(line_number)));
        }
        if (tokens[0] == tokenOthers) {
            qCInfo(LOG_KBIBTEX_IO) << "Person list starts with" << tokenOthers << "near line" << line_number;
            if (parent != nullptr)
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Person list starts with 'others' near line %1")).arg(line_number)));
        } else if (tokens[tokens.count() - 1] == tokenOthers && (tokens.count() < 3 || tokens[tokens.count() - 2] != tokenAnd)) {
            qCInfo(LOG_KBIBTEX_IO) << "Person list ends with" << tokenOthers << "but is not preceeded with name and" << tokenAnd << "near line" << line_number;
            if (parent != nullptr)
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Person list ends with 'others' but is not preceeded with name and 'and' near line %1")).arg(line_number)));
        }
    }

    int nameStart = 0;
    QString prevToken;
    for (int i = 0; i < tokens.count(); ++i) {
        if (tokens[i] == tokenAnd) {
            if (prevToken == tokenAnd) {
                qCInfo(LOG_KBIBTEX_IO) << "Two subsequent" << tokenAnd << "found in person list near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Two subsequent 'and' found in person list near line %1")).arg(line_number)));
            } else if (nameStart < i) {
                const QSharedPointer<Person> person = personFromTokenList(tokens.mid(nameStart, i - nameStart), comma, line_number, parent);
                if (!person.isNull())
                    value.append(person);
                else {
                    qCInfo(LOG_KBIBTEX_IO) << "Text" << tokens.mid(nameStart, i - nameStart).join(' ') << "does not form a name near line" << line_number;
                    if (parent != nullptr)
                        QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Text '%1' does not form a name near line %2")).arg(tokens.mid(nameStart, i - nameStart).join(' ')).arg(line_number)));
                }
            } else {
                qCInfo(LOG_KBIBTEX_IO) << "Found" << tokenAnd << "but no name before it near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Found 'and' but no name before it near line %1")).arg(line_number)));
            }
            nameStart = i + 1;
        } else if (tokens[i] == tokenOthers) {
            if (i < tokens.count() - 1) {
                qCInfo(LOG_KBIBTEX_IO) << "Special word" << tokenOthers << "found before last position in person name near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Special word 'others' found before last position in person name near line %1")).arg(line_number)));
            } else
                value.append(QSharedPointer<PlainText>(new PlainText(QStringLiteral("others"))));
            nameStart = tokens.count() + 1;
        }
        prevToken = tokens[i];
    }

    if (nameStart < tokens.count()) {
        const QSharedPointer<Person> person = personFromTokenList(tokens.mid(nameStart), comma, line_number, parent);
        if (!person.isNull())
            value.append(person);
        else {
            qCInfo(LOG_KBIBTEX_IO) << "Text" << tokens.mid(nameStart).join(' ') << "does not form a name near line" << line_number;
            if (parent != nullptr)
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Text '%1' does not form a name near line %2")).arg(tokens.mid(nameStart).join(' ')).arg(line_number)));
        }
    }
}

QSharedPointer<Person> FileImporterBibTeX::personFromString(const QString &name, const int line_number, QObject *parent)
{
    return personFromString(name, nullptr, line_number, parent);
}

QSharedPointer<Person> FileImporterBibTeX::personFromString(const QString &name, CommaContainment *comma, const int line_number, QObject *parent)
{
    static QStringList tokens;
    contextSensitiveSplit(name, tokens);
    return personFromTokenList(tokens, comma, line_number, parent);
}

QSharedPointer<Person> FileImporterBibTeX::personFromTokenList(const QStringList &tokens, CommaContainment *comma, const int line_number, QObject *parent)
{
    if (comma != nullptr) *comma = ccNoComma;

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
    for (const QString &token : tokens) {
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
                } else if (bracketCounter < 0) {
                    /// Should never happen: more closing brackets than opening ones
                    qCWarning(LOG_KBIBTEX_IO) << "Opening and closing brackets do not match near line" << line_number;
                    if (parent != nullptr)
                        QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Opening and closing brackets do not match near line %1")).arg(line_number)));
                }
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
        if (comma != nullptr) *comma = ccContainsComma;
        return QSharedPointer<Person>(new Person(partC.isEmpty() ? partB.join(QChar(' ')) : partC.join(QChar(' ')), partA.join(QChar(' ')), partC.isEmpty() ? QString() : partB.join(QChar(' '))));
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
     * in lower case belonging to the last name as well (example: "van").
     * In addition, some languages have capital case letters as well
     * (example: "Di Cosmo").
     * Exception: Special keywords such as "Jr." can be appended to the
     * name, not counted as part of the last name.
     */
    partA.clear(); partB.clear(); partC.clear();
    static const QSet<QString> capitalCaseLastNameFragments {QStringLiteral("Di")};
    it = tokens.constEnd();
    while (it != tokens.constBegin()) {
        --it;
        if (partB.isEmpty() && (it->toLower().startsWith(QStringLiteral("jr")) || it->toLower().startsWith(QStringLiteral("sr")) || it->toLower().startsWith(QStringLiteral("iii"))))
            /// handle name suffices like "Jr" or "III."
            partC.prepend(*it);
        else if (partB.isEmpty() || it->at(0).isLower() || capitalCaseLastNameFragments.contains(*it))
            partB.prepend(*it);
        else
            partA.prepend(*it);
    }
    if (!partB.isEmpty()) {
        /// Name was actually like "Peter Ole van der Tuckwell",
        /// split into "Peter Ole" and "van der Tuckwell"
        return QSharedPointer<Person>(new Person(partA.join(QChar(' ')), partB.join(QChar(' ')), partC.isEmpty() ? QString() : partC.join(QChar(' '))));
    }

    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to handle name" << tokens.join(QLatin1Char(' ')) << "near line" << line_number;
    if (parent != nullptr)
        QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, SeverityWarning), Q_ARG(QString, QString(QStringLiteral("Don't know how to handle name '%1' near line %2")).arg(tokens.join(QLatin1Char(' '))).arg(line_number)));
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

QString FileImporterBibTeX::bibtexAwareSimplify(const QString &text)
{
    QString result;
    int i = 0;

    /// Consume initial spaces ...
    while (i < text.length() && text[i].isSpace()) ++i;
    /// ... but if there have been spaces (i.e. i>0), then record a single space only
    if (i > 0)
        result.append(QStringLiteral(" "));

    while (i < text.length()) {
        /// Consume non-spaces
        while (i < text.length() && !text[i].isSpace()) {
            result.append(text[i]);
            ++i;
        }

        /// String may end with a non-space
        if (i >= text.length()) break;

        /// Consume spaces, ...
        while (i < text.length() && text[i].isSpace()) ++i;
        /// ... but record only a single space
        result.append(QStringLiteral(" "));
    }

    return result;
}

bool FileImporterBibTeX::evaluateParameterComments(QTextStream *textStream, const QString &line, File *file)
{
    /// Assertion: variable "line" is all lower-case

    /** check if this file requests a special encoding */
    if (line.startsWith(QStringLiteral("@comment{x-kbibtex-encoding=")) && line.endsWith(QLatin1Char('}'))) {
        const QString encoding = line.mid(28, line.length() - 29).toLower();
        textStream->setCodec(encoding.toLower() == QStringLiteral("latex") ? "us-ascii" : encoding.toLatin1());
        file->setProperty(File::Encoding, encoding.toLower() == QStringLiteral("latex") ? encoding : QString::fromLatin1(textStream->codec()->name()));
        return true;
    } else if (line.startsWith(QStringLiteral("@comment{x-kbibtex-personnameformatting=")) && line.endsWith(QLatin1Char('}'))) {
        // TODO usage of x-kbibtex-personnameformatting is deprecated,
        // as automatic detection is in place
        QString personNameFormatting = line.mid(40, line.length() - 41);
        file->setProperty(File::NameFormatting, personNameFormatting);
        return true;
    } else if (line.startsWith(QStringLiteral("% encoding:"))) {
        /// Interprete JabRef's encoding information
        QString encoding = line.mid(12);
        qCDebug(LOG_KBIBTEX_IO) << "Using JabRef's encoding:" << encoding;
        textStream->setCodec(encoding.toLatin1());
        file->setProperty(File::Encoding, QString::fromLatin1(textStream->codec()->name()));
        return true;
    }

    return false;
}

QString FileImporterBibTeX::tokenidToString(Token token)
{
    switch (token) {
    case tAt: return QString(QStringLiteral("At"));
    case tBracketClose: return QString(QStringLiteral("BracketClose"));
    case tBracketOpen: return QString(QStringLiteral("BracketOpen"));
    case tAlphaNumText: return QString(QStringLiteral("AlphaNumText"));
    case tAssign: return QString(QStringLiteral("Assign"));
    case tComma: return QString(QStringLiteral("Comma"));
    case tDoublecross: return QString(QStringLiteral("Doublecross"));
    case tEOF: return QString(QStringLiteral("EOF"));
    case tUnknown: return QString(QStringLiteral("Unknown"));
    default: return QString(QStringLiteral("<Unknown>"));
    }
}

void FileImporterBibTeX::setCommentHandling(CommentHandling commentHandling) {
    m_commentHandling = commentHandling;
}
