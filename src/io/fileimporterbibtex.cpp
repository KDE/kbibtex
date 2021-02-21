/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

class FileImporterBibTeX::Private
{
private:
    FileImporterBibTeX *parent;

public:
    static const QStringList keysForPersonDetection;

    /// Set via @see setCommentHandling
    CommentHandling commentHandling;

    enum class Token {
        At = 1, BracketOpen = 2, BracketClose = 3, AlphaNumText = 4, Comma = 5, Assign = 6, Doublecross = 7, EndOfFile = 0xffff, Unknown = -1
    };

    enum class CommaContainment { None, Contains };

    typedef struct Statistics {
        /// Used to determine if file prefers quotation marks over
        /// curly brackets or the other way around
        int countCurlyBrackets;

        int countQuotationMarks, countFirstNameFirst, countLastNameFirst;
        int countNoCommentQuote, countCommentPercent, countCommentCommand;
        int countProtectedTitle, countUnprotectedTitle;
        QString mostRecentListSeparator;

        Statistics()
                : countCurlyBrackets(0), countQuotationMarks(0), countFirstNameFirst(0),
              countLastNameFirst(0), countNoCommentQuote(0), countCommentPercent(0),
              countCommentCommand(0), countProtectedTitle(0), countUnprotectedTitle(0)
        {
            /// nothing
        }
    } Statistics;

    typedef struct State {
        QTextStream *textStream;
        /// Low-level character operations
        QChar prevChar, nextChar;
        /// Current line
        int lineNo;
        QString prevLine, currentLine;
        QSet<QString> knownElementIds;

        State(QTextStream *_textStream)
                : textStream(_textStream), lineNo(1)
        {
            /// nothing
        }
    } State;

    Private(FileImporterBibTeX *p)
            : parent(p), commentHandling(CommentHandling::Ignore)
    {
        // TODO
    }

    bool readChar(State &state)
    {
        /// Memorize previous char
        state.prevChar = state.nextChar;

        if (state.textStream->atEnd()) {
            /// At end of data stream
            state.nextChar = QChar::Null;
            return false;
        }

        /// Read next char
        *state.textStream >> state.nextChar;

        /// Test for new line
        if (state.nextChar == QLatin1Char('\n')) {
            /// Update variables tracking line numbers and line content
            ++state.lineNo;
            state.prevLine = state.currentLine;
            state.currentLine.clear();
        } else {
            /// Add read char to current line
            state.currentLine.append(state.nextChar);
        }

        return true;
    }

    bool skipWhiteChar(State &state)
    {
        bool result = true;
        while ((state.nextChar.isSpace() || state.nextChar == QLatin1Char('\t') || state.nextChar == QLatin1Char('\n') || state.nextChar == QLatin1Char('\r')) && result) result = readChar(state);
        return result;
    }

    Token nextToken(State &state)
    {
        if (!skipWhiteChar(state)) {
            /// Some error occurred while reading from data stream
            return Token::EndOfFile;
        }

        Token result = Token::Unknown;

        switch (state.nextChar.toLatin1()) {
        case '@':
            result = Token::At;
            break;
        case '{':
        case '(':
            result = Token::BracketOpen;
            break;
        case '}':
        case ')':
            result = Token::BracketClose;
            break;
        case ',':
            result = Token::Comma;
            break;
        case '=':
            result = Token::Assign;
            break;
        case '#':
            result = Token::Doublecross;
            break;
        default:
            if (state.textStream->atEnd())
                result = Token::EndOfFile;
        }

        if (state.nextChar != QLatin1Char('%')) {
            /// Unclean solution, but necessary for comments
            /// that have a percent sign as a prefix
            readChar(state);
        }
        return result;
    }

// FIXME duplicate
    static void parsePersonList(const QString &text, Value &value, CommaContainment *comma, const int line_number, QObject *parent)
    {
        static const QString tokenAnd = QStringLiteral("and");
        static const QString tokenOthers = QStringLiteral("others");
        static QStringList tokens;
        contextSensitiveSplit(text, tokens);

        if (tokens.count() > 0) {
            if (tokens[0] == tokenAnd) {
                qCInfo(LOG_KBIBTEX_IO) << "Person list starts with" << tokenAnd << "near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Person list starts with 'and' near line %1")).arg(line_number)));
            } else if (tokens.count() > 1 && tokens[tokens.count() - 1] == tokenAnd) {
                qCInfo(LOG_KBIBTEX_IO) << "Person list ends with" << tokenAnd << "near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Person list ends with 'and' near line %1")).arg(line_number)));
            }
            if (tokens[0] == tokenOthers) {
                qCInfo(LOG_KBIBTEX_IO) << "Person list starts with" << tokenOthers << "near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Person list starts with 'others' near line %1")).arg(line_number)));
            } else if (tokens[tokens.count() - 1] == tokenOthers && (tokens.count() < 3 || tokens[tokens.count() - 2] != tokenAnd)) {
                qCInfo(LOG_KBIBTEX_IO) << "Person list ends with" << tokenOthers << "but is not preceded with name and" << tokenAnd << "near line" << line_number;
                if (parent != nullptr)
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Person list ends with 'others' but is not preceded with name and 'and' near line %1")).arg(line_number)));
            }
        }

        int nameStart = 0;
        QString prevToken;
        for (int i = 0; i < tokens.count(); ++i) {
            if (tokens[i] == tokenAnd) {
                if (prevToken == tokenAnd) {
                    qCInfo(LOG_KBIBTEX_IO) << "Two subsequent" << tokenAnd << "found in person list near line" << line_number;
                    if (parent != nullptr)
                        QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Two subsequent 'and' found in person list near line %1")).arg(line_number)));
                } else if (nameStart < i) {
                    const QSharedPointer<Person> person = personFromTokenList(tokens.mid(nameStart, i - nameStart), comma, line_number, parent);
                    if (!person.isNull())
                        value.append(person);
                    else {
                        qCInfo(LOG_KBIBTEX_IO) << "Text" << tokens.mid(nameStart, i - nameStart).join(' ') << "does not form a name near line" << line_number;
                        if (parent != nullptr)
                            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Text '%1' does not form a name near line %2")).arg(tokens.mid(nameStart, i - nameStart).join(' ')).arg(line_number)));
                    }
                } else {
                    qCInfo(LOG_KBIBTEX_IO) << "Found" << tokenAnd << "but no name before it near line" << line_number;
                    if (parent != nullptr)
                        QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Found 'and' but no name before it near line %1")).arg(line_number)));
                }
                nameStart = i + 1;
            } else if (tokens[i] == tokenOthers) {
                if (i < tokens.count() - 1) {
                    qCInfo(LOG_KBIBTEX_IO) << "Special word" << tokenOthers << "found before last position in person name near line" << line_number;
                    if (parent != nullptr)
                        QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Special word 'others' found before last position in person name near line %1")).arg(line_number)));
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
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Text '%1' does not form a name near line %2")).arg(tokens.mid(nameStart).join(' ')).arg(line_number)));
            }
        }
    }

    Token readValue(Value &value, const QString &key, Statistics &statistics, State &state)
    {
        Token token = Token::Unknown;
        const QString iKey = key.toLower();
        static const QSet<QString> verbatimKeys {Entry::ftColor.toLower(), Entry::ftCrossRef.toLower(), Entry::ftXData.toLower()};

        do {
            bool isStringKey = false;
            const QString rawText = readString(isStringKey, statistics, state);
            if (rawText.isNull())
                return Token::EndOfFile;
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
                    ++statistics.countProtectedTitle;
                else
                    ++statistics.countUnprotectedTitle;
            }

            if (keysForPersonDetection.contains(iKey)) {
                if (isStringKey)
                    value.append(QSharedPointer<MacroKey>(new MacroKey(text)));
                else {
                    CommaContainment comma = CommaContainment::Contains;
                    parsePersonList(text, value, &comma, state.lineNo, parent);

                    /// Update statistics on name formatting
                    if (comma == CommaContainment::Contains)
                        ++statistics.countLastNameFirst;
                    else
                        ++statistics.countFirstNameFirst;
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
#if QT_VERSION >= 0x050e00
                    const QStringList fileList = rawText.split(semicolonSpace, Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
                    const QStringList fileList = rawText.split(semicolonSpace, QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
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
                        statistics.mostRecentListSeparator = QStringLiteral("; ");
                    else if (splitChar == ',')
                        statistics.mostRecentListSeparator = QStringLiteral(", ");

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

            token = nextToken(state);
        } while (token == Token::Doublecross);

        return token;
    }

    QString readBracketString(State &state)
    {
        static const QChar backslash = QLatin1Char('\\');
        QString result(0, QChar()); ///< Construct an empty but non-null string
        const QChar openingBracket = state.nextChar;
        const QChar closingBracket = openingBracket == QLatin1Char('{') ? QLatin1Char('}') : (openingBracket == QLatin1Char('(') ? QLatin1Char(')') : QChar());
        Q_ASSERT_X(!closingBracket.isNull(), "QString FileImporterBibTeX::readBracketString()", "openingBracket==state.nextChar is neither '{' nor '('");
        int counter = 1;

        if (!readChar(state)) {
            /// Some error occurred while reading from data stream
            return QString(); ///< return null QString
        }

        while (!state.nextChar.isNull()) {
            if (state.nextChar == openingBracket && state.prevChar != backslash)
                ++counter;
            else if (state.nextChar == closingBracket && state.prevChar != backslash)
                --counter;

            if (counter == 0) {
                break;
            } else
                result.append(state.nextChar);

            if (!readChar(state)) {
                /// Some error occurred while reading from data stream
                return QString(); ///< return null QString
            }
        }

        if (!readChar(state)) {
            /// Some error occurred while reading from data stream
            return QString(); ///< return null QString
        }
        return result;
    }

    QString readSimpleString(State &state, const QString &until = QString(), const bool readNestedCurlyBrackets = false)
    {
        static const QString extraAlphaNumChars = QString(QStringLiteral("?'`-_:.+/$\\\"&"));

        QString result; ///< 'result' is Null on purpose: simple strings cannot be empty in contrast to e.g. quoted strings

        if (!skipWhiteChar(state)) {
            /// Some error occurred while reading from data stream
            return QString(); ///< return null QString
        }

        QChar prevChar = QChar(0x00);
        while (!state.nextChar.isNull()) {
            if (readNestedCurlyBrackets && state.nextChar == QLatin1Char('{') && prevChar != QLatin1Char('\\')) {
                int depth = 1;
                while (depth > 0) {
                    result.append(state.nextChar);
                    prevChar = state.nextChar;
                    if (!readChar(state)) return result;
                    if (state.nextChar == QLatin1Char('{') && prevChar != QLatin1Char('\\')) ++depth;
                    else if (state.nextChar == QLatin1Char('}') && prevChar != QLatin1Char('\\')) --depth;
                }
                result.append(state.nextChar);
                prevChar = state.nextChar;
                if (!readChar(state)) return result;
            }

            const ushort nextCharUnicode = state.nextChar.unicode();
            if (!until.isEmpty()) {
                /// Variable "until" has user-defined value
                if (state.nextChar == QLatin1Char('\n') || state.nextChar == QLatin1Char('\r') || until.contains(state.nextChar)) {
                    /// Force break on line-breaks or if one of the "until" chars has been read
                    break;
                } else {
                    /// Append read character to final result
                    result.append(state.nextChar);
                }
            } else if ((nextCharUnicode >= static_cast<ushort>('a') && nextCharUnicode <= static_cast<ushort>('z')) || (nextCharUnicode >= static_cast<ushort>('A') && nextCharUnicode <= static_cast<ushort>('Z')) || (nextCharUnicode >= static_cast<ushort>('0') && nextCharUnicode <= static_cast<ushort>('9')) || extraAlphaNumChars.contains(state.nextChar)) {
                /// Accept default set of alpha-numeric characters
                result.append(state.nextChar);
            } else
                break;
            prevChar = state.nextChar;
            if (!readChar(state)) break;
        }
        return result;
    }

    QString readQuotedString(State &state)
    {
        QString result(0, QChar()); ///< Construct an empty but non-null string

        Q_ASSERT_X(state.nextChar == QLatin1Char('"'), "QString FileImporterBibTeX::readQuotedString()", "state.nextChar is not '\"'");

        if (!readChar(state)) {
            /// Some error occurred while reading from data stream
            return QString(); ///< return null QString
        }

        while (!state.nextChar.isNull()) {
            if (state.nextChar == QLatin1Char('"') && state.prevChar != QLatin1Char('\\') && state.prevChar != QLatin1Char('{'))
                break;
            else
                result.append(state.nextChar);

            if (!readChar(state)) {
                /// Some error occurred while reading from data stream
                return QString(); ///< return null QString
            }
        }

        if (!readChar(state)) {
            /// Some error occurred while reading from data stream
            return QString(); ///< return null QString
        }

        /// Remove protection around quotation marks
        result.replace(QStringLiteral("{\"}"), QStringLiteral("\""));

        return result;
    }

    QString readString(bool &isStringKey, Statistics &statistics, State &state)
    {
        /// Most often it is not a string key
        isStringKey = false;

        if (!skipWhiteChar(state)) {
            /// Some error occurred while reading from data stream
            return QString(); ///< return null QString
        }

        switch (state.nextChar.toLatin1()) {
        case '{':
        case '(': {
            ++statistics.countCurlyBrackets;
            const QString result = readBracketString(state);
            return result;
        }
        case '"': {
            ++statistics.countQuotationMarks;
            const QString result = readQuotedString(state);
            return result;
        }
        default:
            isStringKey = true;
            const QString result = readSimpleString(state);
            return result;
        }
    }

    bool readCharUntil(const QString &until, State &state)
    {
        Q_ASSERT_X(!until.isEmpty(), "bool  FileImporterBibTeX::readCharUntil(const QString &until)", "\"until\" is empty or invalid");
        bool result = true;
        while (!until.contains(state.nextChar) && (result = readChar(state)));
        return result;
    }

    QString readLine(State &state)
    {
        QString result;
        while (state.nextChar != QLatin1Char('\n') && state.nextChar != QLatin1Char('\r') && readChar(state))
            result.append(state.nextChar);
        return result;
    }

    Macro *readMacroElement(Statistics &statistics, State &state)
    {
        Token token = nextToken(state);
        while (token != Token::BracketOpen) {
            if (token == Token::EndOfFile) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing macro near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Opening curly brace '{' expected";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing macro near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Opening curly brace '{' expected";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing macro near line %1: Opening curly brace '{' expected")).arg(state.lineNo)));
                return nullptr;
            }
            token = nextToken(state);
        }

        QString key = readSimpleString(state);

        if (key.isEmpty()) {
            /// Cope with empty keys,
            /// duplicates are handled further below
            key = QStringLiteral("EmptyId");
        } else if (!Encoder::containsOnlyAscii(key)) {
            /// Try to avoid non-ascii characters in ids
            const QString newKey = Encoder::instance().convertToPlainAscii(key);
            qCWarning(LOG_KBIBTEX_IO) << "Macro key" << key << "near line" << state.lineNo << "contains non-ASCII characters, converted to" << newKey;
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Macro key '%1'  near line %2 contains non-ASCII characters, converted to '%3'")).arg(key).arg(state.lineNo).arg(newKey)));
            key = newKey;
        }

        /// Check for duplicate entry ids, avoid collisions
        if (state.knownElementIds.contains(key)) {
            static const QString newIdPattern = QStringLiteral("%1-%2");
            int idx = 2;
            QString newKey = newIdPattern.arg(key).arg(idx);
            while (state.knownElementIds.contains(newKey))
                newKey = newIdPattern.arg(key).arg(++idx);
            qCDebug(LOG_KBIBTEX_IO) << "Duplicate macro key" << key << ", using replacement key" << newKey;
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Duplicate macro key '%1', using replacement key '%2'")).arg(key, newKey)));
            key = newKey;
        }
        state.knownElementIds.insert(key);

        if (nextToken(state) != Token::Assign) {
#if QT_VERSION >= 0x050e00
            qCCritical(LOG_KBIBTEX_IO) << "Error in parsing macro" << key << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Assign symbol '=' expected";
#else // QT_VERSION < 0x050e00
            qCCritical(LOG_KBIBTEX_IO) << "Error in parsing macro" << key << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Assign symbol '=' expected";
#endif // QT_VERSION >= 0x050e00
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing macro '%1' near line %2: Assign symbol '=' expected")).arg(key).arg(state.lineNo)));
            return nullptr;
        }

        Macro *macro = new Macro(key);
        do {
            bool isStringKey = false;
            QString text = readString(isStringKey, statistics, state);
            if (text.isNull()) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing macro" << key << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Could not read macro's text";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing macro" << key << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Could not read macro's text";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing macro '%1' near line %2: Could not read macro's text")).arg(key).arg(state.lineNo)));
                delete macro;
                return nullptr;
            }
            text = EncoderLaTeX::instance().decode(bibtexAwareSimplify(text));
            if (isStringKey)
                macro->value().append(QSharedPointer<MacroKey>(new MacroKey(text)));
            else
                macro->value().append(QSharedPointer<PlainText>(new PlainText(text)));

            token = nextToken(state);
        } while (token == Token::Doublecross);

        return macro;
    }

    Comment *readCommentElement(State &state)
    {
        if (!readCharUntil(QStringLiteral("{("), state))
            return nullptr;
        return new Comment(EncoderLaTeX::instance().decode(readBracketString(state)));
    }

    Comment *readPlainCommentElement(const QString &prefix, State &state)
    {
        QString result = rstrip(EncoderLaTeX::instance().decode(prefix + readLine(state)));
        while (state.nextChar == QLatin1Char('\n') || state.nextChar == QLatin1Char('\r')) readChar(state);
        while (!state.nextChar.isNull() && state.nextChar != QLatin1Char('@')) {
            const QChar nextChar = state.nextChar;
            const QString line = readLine(state);
            while (state.nextChar == QLatin1Char('\n') || state.nextChar == QLatin1Char('\r')) readChar(state);
            result.append(rstrip(EncoderLaTeX::instance().decode((nextChar == QLatin1Char('%') ? QString() : QString(nextChar)) + line)));
        }

        if (result.startsWith(QStringLiteral("x-kbibtex"))) {
            qCWarning(LOG_KBIBTEX_IO) << "Plain comment element starts with 'x-kbibtex', this should not happen";
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QStringLiteral("Plain comment element starts with 'x-kbibtex', this should not happen")));
            /// ignore special comments
            return nullptr;
        }

        return new Comment(result);
    }


/// GCC's C++ compiler may complain about that the following function does reach
/// its end without returning anything. Seemingly, the compiler is not aware that
/// the switch covers all cases of the FileImporterBibTeX::Private::Token enum.
/// Thus, this particular check gets temporarily disabled.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
    QString tokenidToString(Token token)
    {
        switch (token) {
        case Token::At: return QString(QStringLiteral("At"));
        case Token::BracketClose: return QString(QStringLiteral("BracketClose"));
        case Token::BracketOpen: return QString(QStringLiteral("BracketOpen"));
        case Token::AlphaNumText: return QString(QStringLiteral("AlphaNumText"));
        case Token::Assign: return QString(QStringLiteral("Assign"));
        case Token::Comma: return QString(QStringLiteral("Comma"));
        case Token::Doublecross: return QString(QStringLiteral("Doublecross"));
        case Token::EndOfFile: return QString(QStringLiteral("EOF"));
        case Token::Unknown: return QString(QStringLiteral("Unknown"));
        }
    }
#pragma GCC diagnostic pop

    Preamble *readPreambleElement(Statistics &statistics, State &state)
    {
        Token token = nextToken(state);
        while (token != Token::BracketOpen) {
            if (token == Token::EndOfFile) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing preamble near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Opening curly brace '{' expected";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing preamble near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Opening curly brace '{' expected";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing preamble near line %1: Opening curly brace '{' expected")).arg(state.lineNo)));
                return nullptr;
            }
            token = nextToken(state);
        }

        Preamble *preamble = new Preamble();
        do {
            bool isStringKey = false;
            QString text = readString(isStringKey, statistics, state);
            if (text.isNull()) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing preamble near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Could not read preamble's text";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing preamble near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Could not read preamble's text";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing preamble near line %1: Could not read preamble's text")).arg(state.lineNo)));
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

            token = nextToken(state);
        } while (token == Token::Doublecross);

        return preamble;
    }

    Entry *readEntryElement(const QString &typeString, Statistics &statistics, State &state)
    {
        const KBibTeX::Casing keywordCasing = Preferences::instance().bibTeXKeywordCasing();

        Token token = nextToken(state);
        while (token != Token::BracketOpen) {
            if (token == Token::EndOfFile) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Opening curly brace '{' expected";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Opening curly brace '{' expected";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing entry near line %1: Opening curly brace '{' expected")).arg(state.lineNo)));
                return nullptr;
            }
            token = nextToken(state);
        }

        QString id = readSimpleString(state, QStringLiteral(",}"), true).trimmed();
        if (id.isEmpty()) {
            if (state.nextChar == QLatin1Char(',') || state.nextChar == QLatin1Char('}')) {
                /// Cope with empty ids,
                /// duplicates are handled further below
                id = QStringLiteral("EmptyId");
            }
            else {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry near line" << state.lineNo << ":" << state.prevLine << Qt::endl << state.currentLine << "): Could not read entry id";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry near line" << state.lineNo << ":" << state.prevLine << endl << state.currentLine << "): Could not read entry id";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing preambentryle near line %1: Could not read entry id")).arg(state.lineNo)));
                return nullptr;
            }
        } else {
            if (id.contains(QStringLiteral("\\")) || id.contains(QStringLiteral("{"))) {
                const QString newId = EncoderLaTeX::instance().decode(id);
                qCWarning(LOG_KBIBTEX_IO) << "Entry id" << id << "near line" << state.lineNo << "contains backslashes or curly brackets, converted to" << newId;
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Entry id '%1' near line %2 contains backslashes or curly brackets, converted to '%3'")).arg(id).arg(state.lineNo).arg(newId)));
                id = newId;
            }
            if (!Encoder::containsOnlyAscii(id)) {
                /// Try to avoid non-ascii characters in ids
                const QString newId = Encoder::instance().convertToPlainAscii(id);
                qCWarning(LOG_KBIBTEX_IO) << "Entry id" << id << "near line" << state.lineNo << "contains non-ASCII characters, converted to" << newId;
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Entry id '%1' near line %2 contains non-ASCII characters, converted to '%3'")).arg(id).arg(state.lineNo).arg(newId)));
                id = newId;
            }
        }
        static const QVector<QChar> invalidIdCharacters = {QLatin1Char('{'), QLatin1Char('}'), QLatin1Char(',')};
        for (const QChar &invalidIdCharacter : invalidIdCharacters)
            if (id.contains(invalidIdCharacter)) {
                qCWarning(LOG_KBIBTEX_IO) << "Entry id" << id << "near line" << state.lineNo << "contains invalid character" << invalidIdCharacter;
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Entry id '%1' near line %2 contains invalid character '%3'")).arg(id).arg(state.lineNo).arg(invalidIdCharacter)));
                return nullptr;
            }

        /// Check for duplicate entry ids, avoid collisions
        if (state.knownElementIds.contains(id)) {
            static const QString newIdPattern = QStringLiteral("%1-%2");
            int idx = 2;
            QString newId = newIdPattern.arg(id).arg(idx);
            while (state.knownElementIds.contains(newId))
                newId = newIdPattern.arg(id).arg(++idx);
            qCDebug(LOG_KBIBTEX_IO) << "Duplicate id" << id << "near line" << state.lineNo << ", using replacement id" << newId;
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Info), Q_ARG(QString, QString(QStringLiteral("Duplicate id '%1' near line %2, using replacement id '%3'")).arg(id).arg(state.lineNo).arg(newId)));
            id = newId;
        }
        state.knownElementIds.insert(id);

        Entry *entry = new Entry(BibTeXEntries::instance().format(typeString), id);

        token = nextToken(state);
        do {
            if (token == Token::BracketClose)
                break;
            else if (token == Token::EndOfFile) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Unexpected end of data in entry" << id << "near line" << state.lineNo << ":" << state.prevLine << Qt::endl << state.currentLine;
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Unexpected end of data in entry" << id << "near line" << state.lineNo << ":" << state.prevLine << endl << state.currentLine;
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Unexpected end of data in entry '%1' near line %2")).arg(id).arg(state.lineNo)));
                delete entry;
                return nullptr;
            } else if (token != Token::Comma) {
                if (state.nextChar.isLetter()) {
#if QT_VERSION >= 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Comma symbol ',' expected but got character" << state.nextChar << "(token" << tokenidToString(token) << ")";
#else // QT_VERSION < 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Comma symbol ',' expected but got character" << state.nextChar << "(token" << tokenidToString(token) << ")";
#endif // QT_VERSION >= 0x050e00
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Comma symbol ',' expected but got character '%3' (token %4)")).arg(id).arg(state.lineNo).arg(state.nextChar).arg(tokenidToString(token))));
                } else if (state.nextChar.isPrint()) {
#if QT_VERSION >= 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Comma symbol ',' expected but got character" << state.nextChar << "(" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << ", token" << tokenidToString(token) << ")";
#else // QT_VERSION < 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Comma symbol ',' expected but got character" << state.nextChar << "(" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << ", token" << tokenidToString(token) << ")";
#endif // QT_VERSION >= 0x050e00
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Comma symbol ',' expected but got character '%3' (0x%4, token %5)")).arg(id).arg(state.lineNo).arg(state.nextChar).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(tokenidToString(token))));
                } else {
#if QT_VERSION >= 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Comma symbol (,) expected but got character" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << "(token" << tokenidToString(token) << ")";
#else // QT_VERSION < 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Comma symbol (,) expected but got character" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << "(token" << tokenidToString(token) << ")";
#endif // QT_VERSION >= 0x050e00
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Comma symbol ',' expected but got character 0x%3 (token %4)")).arg(id).arg(state.lineNo).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(tokenidToString(token))));
                }
                delete entry;
                return nullptr;
            }

            QString keyName = BibTeXFields::instance().format(readSimpleString(state), keywordCasing);
            if (keyName.isEmpty()) {
                token = nextToken(state);
                if (token == Token::BracketClose) {
                    /// Most often it is the case that the previous line ended with a comma,
                    /// implying that this entry continues, but instead it gets closed by
                    /// a closing curly bracket.
#if QT_VERSION >= 0x050e00
                    qCDebug(LOG_KBIBTEX_IO) << "Issue while parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Last key-value pair ended with a non-conformant comma, ignoring that";
#else // QT_VERSION < 0x050e00
                    qCDebug(LOG_KBIBTEX_IO) << "Issue while parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Last key-value pair ended with a non-conformant comma, ignoring that";
#endif // QT_VERSION >= 0x050e00
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Info), Q_ARG(QString, QString(QStringLiteral("Issue while parsing entry '%1' near line %2: Last key-value pair ended with a non-conformant comma, ignoring that")).arg(id).arg(state.lineNo)));
                    break;
                } else {
                    /// Something looks terribly wrong
#if QT_VERSION >= 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "): Closing curly bracket expected, but found" << tokenidToString(token);
#else // QT_VERSION < 0x050e00
                    qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "): Closing curly bracket expected, but found" << tokenidToString(token);
#endif // QT_VERSION >= 0x050e00
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing entry '%1' near line %2: Closing curly bracket expected, but found %3")).arg(id).arg(state.lineNo).arg(tokenidToString(token))));
                    delete entry;
                    return nullptr;
                }
            }
            /// Try to avoid non-ascii characters in keys
            const QString newkeyName = Encoder::instance().convertToPlainAscii(keyName);
            if (newkeyName != keyName) {
                qCWarning(LOG_KBIBTEX_IO) << "Field name " << keyName << "near line" << state.lineNo << "contains non-ASCII characters, converted to" << newkeyName;
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Field name '%1' near line %2 contains non-ASCII characters, converted to '%3'")).arg(keyName).arg(state.lineNo).arg(newkeyName)));
                keyName = newkeyName;
            }

            token = nextToken(state);
            if (token != Token::Assign) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << ", field name" << keyName << "near line" << state.lineNo  << "(" << state.prevLine << Qt::endl << state.currentLine << "): Assign symbol '=' expected after field name";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Error in parsing entry" << id << ", field name" << keyName << "near line" << state.lineNo  << "(" << state.prevLine << endl << state.currentLine << "): Assign symbol '=' expected after field name";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Error in parsing entry '%1', field name '%2' near line %3: Assign symbol '=' expected after field name")).arg(id, keyName).arg(state.lineNo)));
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
                } else if (keysForPersonDetection.contains(keyName.toLower())) {
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
#if QT_VERSION >= 0x050e00
                    qCDebug(LOG_KBIBTEX_IO) << "Entry" << id << "already contains a key" << keyName << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << "), using" << (keyName + appendix);
#else // QT_VERSION < 0x050e00
                    qCDebug(LOG_KBIBTEX_IO) << "Entry" << id << "already contains a key" << keyName << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << "), using" << (keyName + appendix);
#endif // QT_VERSION >= 0x050e00
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Entry '%1' already contains a key '%2' near line %4, using '%3'")).arg(id, keyName, keyName + appendix).arg(state.lineNo)));
                    keyName += appendix;
                }
            }

            token = readValue(value, keyName, statistics, state);
            if (token != Token::BracketClose && token != Token::Comma) {
#if QT_VERSION >= 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Failed to read value in entry" << id << ", field name" << keyName << "near line" << state.lineNo  << "(" << state.prevLine << Qt::endl << state.currentLine << ")";
#else // QT_VERSION < 0x050e00
                qCWarning(LOG_KBIBTEX_IO) << "Failed to read value in entry" << id << ", field name" << keyName << "near line" << state.lineNo  << "(" << state.prevLine << endl << state.currentLine << ")";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Failed to read value in entry '%1', field name '%2' near line %3")).arg(id, keyName).arg(state.lineNo)));
                delete entry;
                return nullptr;
            }

            entry->insert(keyName, value);
        } while (true);

        return entry;
    }

    Element *nextElement(Statistics &statistics, State &state)
    {
        Token token = nextToken(state);

        if (token == Token::At) {
            const QString elementType = readSimpleString(state);
            const QString elementTypeLower = elementType.toLower();

            if (elementTypeLower == QStringLiteral("comment")) {
                ++statistics.countCommentCommand;
                return readCommentElement(state);
            } else if (elementTypeLower == QStringLiteral("string"))
                return readMacroElement(statistics, state);
            else if (elementTypeLower == QStringLiteral("preamble"))
                return readPreambleElement(statistics, state);
            else if (elementTypeLower == QStringLiteral("import")) {
                qCDebug(LOG_KBIBTEX_IO) << "Skipping potential HTML/JavaScript @import statement near line" << state.lineNo;
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Info), Q_ARG(QString, QString(QStringLiteral("Skipping potential HTML/JavaScript @import statement near line %1")).arg(state.lineNo)));
                return nullptr;
            } else if (!elementType.isEmpty())
                return readEntryElement(elementType, statistics, state);
            else {
                qCWarning(LOG_KBIBTEX_IO) << "Element type after '@' is empty or invalid near line" << state.lineNo;
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Element type after '@' is empty or invalid near line %1")).arg(state.lineNo)));
                return nullptr;
            }
        } else if (token == Token::Unknown && state.nextChar == QLatin1Char('%')) {
            /// do not complain about LaTeX-like comments, just eat them
            ++statistics.countCommentPercent;
            return readPlainCommentElement(QString(), state);
        } else if (token == Token::Unknown) {
            if (state.nextChar.isLetter()) {
#if QT_VERSION >= 0x050e00
                qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << state.nextChar << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << ")" << ", treating as comment";
#else // QT_VERSION < 0x050e00
                qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << state.nextChar << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << ")" << ", treating as comment";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Info), Q_ARG(QString, QString(QStringLiteral("Unknown character '%1' near line %2, treating as comment")).arg(state.nextChar).arg(state.lineNo)));
            } else if (state.nextChar.isPrint()) {
#if QT_VERSION >= 0x050e00
                qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << state.nextChar << "(" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << ") near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << ")" << ", treating as comment";
#else // QT_VERSION < 0x050e00
                qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << state.nextChar << "(" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << ") near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << ")" << ", treating as comment";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Info), Q_ARG(QString, QString(QStringLiteral("Unknown character '%1' (0x%2) near line %3, treating as comment")).arg(state.nextChar).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(state.lineNo)));
            } else {
#if QT_VERSION >= 0x050e00
                qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << "near line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << ")" << ", treating as comment";
#else // QT_VERSION < 0x050e00
                qCDebug(LOG_KBIBTEX_IO) << "Unknown character" << QString(QStringLiteral("0x%1")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')) << "near line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << ")" << ", treating as comment";
#endif // QT_VERSION >= 0x050e00
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Info), Q_ARG(QString, QString(QStringLiteral("Unknown character 0x%1 near line %2, treating as comment")).arg(state.nextChar.unicode(), 4, 16, QLatin1Char('0')).arg(state.lineNo)));
            }
            ++statistics.countNoCommentQuote;
            return readPlainCommentElement(QString(state.prevChar) + state.nextChar, state);
        }

        if (token != Token::EndOfFile) {
#if QT_VERSION >= 0x050e00
            qCWarning(LOG_KBIBTEX_IO) << "Don't know how to parse next token of type" << tokenidToString(token) << "in line" << state.lineNo << "(" << state.prevLine << Qt::endl << state.currentLine << ")";
#else // QT_VERSION < 0x050e00
            qCWarning(LOG_KBIBTEX_IO) << "Don't know how to parse next token of type" << tokenidToString(token) << "in line" << state.lineNo << "(" << state.prevLine << endl << state.currentLine << ")";
#endif // QT_VERSION >= 0x050e00
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Error), Q_ARG(QString, QString(QStringLiteral("Don't know how to parse next token of type %1 in line %2")).arg(tokenidToString(token)).arg(state.lineNo)));
        }

        return nullptr;
    }


    static QSharedPointer<Person> personFromString(const QString &name, CommaContainment *comma, const int line_number, QObject *parent)
    {
        // TODO Merge with FileImporter::splitName and FileImporterBibTeX::contextSensitiveSplit
        static QStringList tokens;
        contextSensitiveSplit(name, tokens);
        return personFromTokenList(tokens, comma, line_number, parent);
    }

    static QSharedPointer<Person> personFromTokenList(const QStringList &tokens, CommaContainment *comma, const int line_number, QObject *parent)
    {
        if (comma != nullptr) *comma = CommaContainment::None;

        /// Simple case: provided list of tokens is empty, return invalid Person
        if (tokens.isEmpty())
            return QSharedPointer<Person>();

        /**
         * The sequence of tokens may contain in up to two of its elements one comma each:
         * {"Tuckwell,", "Peter,", "Jr."}. In this case, fill three string lists:
         * one with tokens before the first comma, one with tokens after the second commas,
         * and one with tokens after the second commas. If commas appear in the middle of a
         * token, split token into two new tokens and add them to two different string lists.
         * The comma itself will not be part of any string in the string lists.
         * Example:
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
                            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Opening and closing brackets do not match near line %1")).arg(line_number)));
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
            if (comma != nullptr) *comma = CommaContainment::Contains;
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
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Don't know how to handle name '%1' near line %2")).arg(tokens.join(QLatin1Char(' '))).arg(line_number)));
        return QSharedPointer<Person>();
    }

};

const QStringList FileImporterBibTeX::Private::keysForPersonDetection {Entry::ftAuthor, Entry::ftEditor, QStringLiteral("bookauthor") /** used by JSTOR */};


FileImporterBibTeX::FileImporterBibTeX(QObject *parent)
        : FileImporter(parent), d(new Private(this)), m_cancelFlag(false)
{
    /// nothing
}

FileImporterBibTeX::~FileImporterBibTeX()
{
    delete d;
}

File *FileImporterBibTeX::fromString(const QString &rawText)
{
    if (rawText.isEmpty()) {
        qCInfo(LOG_KBIBTEX_IO) << "BibTeX data converted to string is empty";
        emit message(MessageSeverity::Warning, QStringLiteral("BibTeX data converted to string is empty"));
        return new File();
    }

    File *result = new File();

    /** Remove HTML code from the input source */
    // FIXME HTML data should be removed somewhere else? onlinesearch ...
    const int originalLength = rawText.length();
    QString internalRawText = rawText;
    internalRawText = internalRawText.remove(KBibTeX::htmlRegExp);
    const int afterHTMLremovalLength = internalRawText.length();
    if (originalLength != afterHTMLremovalLength) {
        qCInfo(LOG_KBIBTEX_IO) << (originalLength - afterHTMLremovalLength) << "characters of HTML tags have been removed";
        emit message(MessageSeverity::Info, QString(QStringLiteral("%1 characters of HTML tags have been removed")).arg(originalLength - afterHTMLremovalLength));
    }

    Private::Statistics statistics;
    Private::State state(new QTextStream(&internalRawText, QIODevice::ReadOnly));
    d->readChar(state);

    bool gotAtLeastOneElement = false;
    while (!state.nextChar.isNull() && !m_cancelFlag && !state.textStream->atEnd()) {
        emit progress(qint64toint(state.textStream->pos()), internalRawText.length());
        Element *element = d->nextElement(statistics, state);

        if (element != nullptr) {
            gotAtLeastOneElement = true;
            if (d->commentHandling == CommentHandling::Keep || !Comment::isComment(*element))
                result->append(QSharedPointer<Element>(element));
            else
                delete element;
        }
    }

    if (!gotAtLeastOneElement) {
        qCWarning(LOG_KBIBTEX_IO) << "In non-empty input, did not find a single BibTeX element";
        emit message(MessageSeverity::Error, QStringLiteral("In non-empty input, did not find a single BibTeX element"));
        delete result;
        result = nullptr;
    }

    emit progress(100, 100);

    if (m_cancelFlag) {
        qCWarning(LOG_KBIBTEX_IO) << "Loading bibliography data has been canceled";
        emit message(MessageSeverity::Error, QStringLiteral("Loading bibliography data has been canceled"));
        delete result;
        result = nullptr;
    }

    delete state.textStream;

    if (result != nullptr) {
        /// Set the file's preferences for string delimiters
        /// deduced from statistics built while parsing the file
        result->setProperty(File::StringDelimiter, statistics.countQuotationMarks > statistics.countCurlyBrackets ? QStringLiteral("\"\"") : QStringLiteral("{}"));
        /// Set the file's preferences for name formatting
        result->setProperty(File::NameFormatting, statistics.countFirstNameFirst > statistics.countLastNameFirst ? Preferences::personNameFormatFirstLast : Preferences::personNameFormatLastFirst);
        /// Set the file's preferences for title protected
        Qt::CheckState triState = (statistics.countProtectedTitle > statistics.countUnprotectedTitle * 4) ? Qt::Checked : ((statistics.countProtectedTitle * 4 < statistics.countUnprotectedTitle) ? Qt::Unchecked : Qt::PartiallyChecked);
        result->setProperty(File::ProtectCasing, static_cast<int>(triState));
        /// Set the file's preferences for quoting of comments
        if (statistics.countNoCommentQuote > statistics.countCommentCommand && statistics.countNoCommentQuote > statistics.countCommentPercent)
            result->setProperty(File::QuoteComment, static_cast<int>(Preferences::QuoteComment::None));
        else if (statistics.countCommentCommand > statistics.countNoCommentQuote && statistics.countCommentCommand > statistics.countCommentPercent)
            result->setProperty(File::QuoteComment, static_cast<int>(Preferences::QuoteComment::Command));
        else
            result->setProperty(File::QuoteComment, static_cast<int>(Preferences::QuoteComment::PercentSign));
        if (!statistics.mostRecentListSeparator.isEmpty())
            result->setProperty(File::ListSeparator, statistics.mostRecentListSeparator);
        // TODO gather more statistics for keyword casing etc.
    }

    return result;
}

File *FileImporterBibTeX::load(QIODevice *iodevice)
{
    m_cancelFlag = false;

    if (!iodevice->isReadable() && !iodevice->open(QIODevice::ReadOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device not readable";
        emit message(MessageSeverity::Error, QStringLiteral("Input device not readable"));
        return nullptr;
    } else if (iodevice->atEnd() || iodevice->size() <= 0) {
        qCInfo(LOG_KBIBTEX_IO) << "Input device at end or does not contain any data";
        emit message(MessageSeverity::Warning, QStringLiteral("Input device at end or does not contain any data"));
        return new File();
    }

    QByteArray rawData = iodevice->readAll();
    iodevice->close();

    bool encodingMayGetDeterminedByRawData = true;
    QString encoding(Preferences::instance().bibTeXEncoding()); ///< default value taken from Preferences
    if (rawData.length() >= 8 && rawData.at(0) != 0 && rawData.at(1) == 0 && rawData.at(2) == 0 && rawData.at(3) == 0 && rawData.at(4) != 0 && rawData.at(5) == 0 && rawData.at(6) == 0 && rawData.at(7) == 0) {
        /// UTF-32LE (Little Endian)
        encoding = QStringLiteral("UTF-32LE");
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 4 && static_cast<unsigned char>(rawData.at(0)) == 0xff && static_cast<unsigned char>(rawData.at(1)) == 0xfe && rawData.at(2) == 0 && rawData.at(3) == 0) {
        /// UTF-32LE (Little Endian) with BOM
        encoding = QStringLiteral("UTF-32LE");
        rawData = rawData.mid(4); ///< skip BOM
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 8 && rawData.at(0) == 0 && rawData.at(1) == 0 && rawData.at(2) == 0 && rawData.at(3) != 0 && rawData.at(4) == 0 && rawData.at(5) == 0 && rawData.at(6) == 0 && rawData.at(7) != 0) {
        /// UTF-32BE (Big Endian)
        encoding = QStringLiteral("UTF-32BE");
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 4 && static_cast<unsigned char>(rawData.at(0)) == 0 && static_cast<unsigned char>(rawData.at(1)) == 0 && static_cast<unsigned char>(rawData.at(2)) == 0xfe && static_cast<unsigned char>(rawData.at(3)) == 0xff) {
        /// UTF-32BE (Big Endian) with BOM
        encoding = QStringLiteral("UTF-32BE");
        rawData = rawData.mid(4); ///< skip BOM
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 6 && rawData.at(0) != 0 && rawData.at(1) == 0 && rawData.at(2) != 0 && rawData.at(3) == 0 && rawData.at(4) != 0 && rawData.at(5) == 0) {
        /// UTF-16LE (Little Endian)
        encoding = QStringLiteral("UTF-16LE");
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 4 && static_cast<unsigned char>(rawData.at(0)) == 0xff && static_cast<unsigned char>(rawData.at(1)) == 0xfe && rawData.at(2) != 0 && rawData.at(3) == 0) {
        /// UTF-16LE (Little Endian) with BOM
        encoding = QStringLiteral("UTF-16LE");
        rawData = rawData.mid(2); ///< skip BOM
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 6 && rawData.at(0) == 0 && rawData.at(1) != 0 && rawData.at(2) == 0 && rawData.at(3) != 0 && rawData.at(4) == 0 && rawData.at(5) != 0) {
        /// UTF-16BE (Big Endian)
        encoding = QStringLiteral("UTF-16BE");
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 4 && static_cast<unsigned char>(rawData.at(0)) == 0xfe && static_cast<unsigned char>(rawData.at(1)) == 0xff && rawData.at(2) == 0 && rawData.at(3) != 0) {
        /// UTF-16BE (Big Endian) with BOM
        encoding = QStringLiteral("UTF-16BE");
        rawData = rawData.mid(2); ///< skip BOM
        encodingMayGetDeterminedByRawData = false;
    } else if (rawData.length() >= 3 && static_cast<unsigned char>(rawData.at(0)) == 0xef && static_cast<unsigned char>(rawData.at(1)) == 0xbb && static_cast<unsigned char>(rawData.at(2)) == 0xbf) {
        /// UTF-8 BOM
        encoding = QStringLiteral("UTF-8");
        rawData = rawData.mid(3); ///< skip BOM
        encodingMayGetDeterminedByRawData = false;
    } else {
        /// Assuming that encoding is ASCII-compatible, thus it is possible
        /// to search for a byte sequence containin ASCII text
        const QByteArray rawDataBeginning = rawData.left(1024);
        const int xkbibtexencodingpos = qMax(rawDataBeginning.indexOf("@comment{x-kbibtex-encoding="), rawDataBeginning.indexOf("@Comment{x-kbibtex-encoding="));
        if (xkbibtexencodingpos >= 0) {
            int i = xkbibtexencodingpos + 28, l = 0;
            encoding.clear();
            encoding.reserve(32);
            while (l < 32 && rawData.at(i) >= 0x20 && rawData.at(i) != '\n' && rawData.at(i) != '\r' && rawData.at(i) != '}' && rawData.at(i) != ')' && static_cast<unsigned char>(rawData.at(i)) < 0x80) {
                encoding.append(QLatin1Char(rawData.at(i)));
                ++i;
                ++l;
            }
            rawData = rawData.left(xkbibtexencodingpos) + rawData.mid(i + 1); ///< remove encoding comment
            encodingMayGetDeterminedByRawData = encoding.isEmpty();
        } else {
            const int jabrefencodingpos = qMax(rawDataBeginning.indexOf("% Encoding:"), rawDataBeginning.indexOf("% encoding:"));
            if (jabrefencodingpos >= 0) {
                int i = jabrefencodingpos + 11, l = 0;
                encoding.clear();
                encoding.reserve(32);
                while (l < 32 && rawData.at(i) >= 0x20 && rawData.at(i) != '\n' && rawData.at(i) != '\r' && rawData.at(i) != '}' && rawData.at(i) != ')' && static_cast<unsigned char>(rawData.at(i)) < 0x80) {
                    encoding.append(QLatin1Char(rawData.at(i)));
                    ++i;
                    ++l;
                }
                encoding = encoding.trimmed();
                rawData = rawData.left(jabrefencodingpos) + rawData.mid(i + 1); ///< remove encoding comment
                encodingMayGetDeterminedByRawData = encoding.isEmpty();
            }
        }
    }

    if (encoding.isEmpty()) {
        encoding = Preferences::instance().bibTeXEncoding(); ///< just in case something went wrong
        encodingMayGetDeterminedByRawData = true;
    }

    if (encodingMayGetDeterminedByRawData) {
        // Unclear which encoding raw data makes use of, so test for
        // two popular choices: (1) only ASCII (means 'LaTeX' encoding)
        // and (2) UTF-8
        bool hasUTF8 = false;
        bool outsideUTF8 = false;
        const int len = qMin(2048, rawData.length() - 3);
        for (int i = 0; i < len; ++i) {
            const char c1 = rawData.at(i);
            if ((c1 & 0x80) == 0) {
                // This character is probably ASCII, so ignore it
            } else {
                const char c2 = rawData.at(i + 1);
                if ((c1 & 0xe0) == 0xc0 && (c2 & 0xc0) == 0x80) {
                    // This is a two-byte UTF-8 symbol
                    hasUTF8 = true;
                    ++i;
                } else {
                    const char c3 = rawData.at(i + 2);
                    if ((c1 & 0xf0) == 0xe0 && (c2 & 0xc0) == 0x80 && (c3 & 0xc0) == 0x80) {
                        // This is a three-byte UTF-8 symbol
                        hasUTF8 = true;
                        i += 2;
                    } else {
                        const char c4 = rawData.at(i + 3);
                        if ((c1 & 0xf8) == 0xf0 && (c2 & 0xc0) == 0x80 && (c3 & 0xc0) == 0x80 && (c4 & 0xc0) == 0x80) {
                            // This is a four-byte UTF-8 symbol
                            hasUTF8 = true;
                            i += 3;
                        } else {
                            outsideUTF8 = true;
                            break; //< No point in further testing more raw data
                        }
                    }
                }
            }
        }

        if (!outsideUTF8) {
            encoding = hasUTF8 ? QStringLiteral("UTF-8") : QStringLiteral("LaTeX");
            encodingMayGetDeterminedByRawData = false; //< Now the encoding is known
        }
    }

    encoding = encoding.toLower();
    if (encoding == QStringLiteral("us-ascii")) {
        qCDebug(LOG_KBIBTEX_IO) << "Replacing deprecated encoding 'US-ASCII' with 'LaTeX'";
        encoding = QStringLiteral("latex"); //< encoding 'US-ASCII' is deprecated in favour of 'LaTeX'
    }
    // For encoding 'LaTeX', fall back to encoding 'UTF-8' when creating
    // a QTextCodec instance, but keep 'LaTeX' as the bibliography's 'actual' encoding (used as its encoding property)
    QTextCodec *codec = QTextCodec::codecForName(encoding == QStringLiteral("latex") ? "utf-8" : encoding.toLatin1());
    if (codec == nullptr) {
        qCWarning(LOG_KBIBTEX_IO) << "Could not determine codec for encoding" << encoding;
        emit message(MessageSeverity::Warning, QString(QStringLiteral("Could not determine codec for encoding '%1'")).arg(encoding));
        return nullptr;
    }
    QString rawText = codec->toUnicode(rawData);

    /// Remove deprecated 'x-kbibtex-personnameformatting' from BibTeX raw text
    const int posPersonNameFormatting = rawText.indexOf(QStringLiteral("@comment{x-kbibtex-personnameformatting="));
    if (posPersonNameFormatting >= 0) {
        const int endOfPersonNameFormatting = rawText.indexOf(QLatin1Char('}'), posPersonNameFormatting + 39);
        if (endOfPersonNameFormatting > 0)
            rawText = rawText.left(posPersonNameFormatting) + rawText.mid(endOfPersonNameFormatting + 1);
    }

    File *result = fromString(rawText);
    /// In the File object's property, store the encoding used to load the data
    result->setProperty(File::Encoding, encoding);

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
#if QT_VERSION >= 0x050e00
            const QStringList keywords = text.split(it.value(), Qt::SkipEmptyParts).replaceInStrings(unneccessarySpacing, QStringLiteral(" "));
#else // QT_VERSION < 0x050e00
            const QStringList keywords = text.split(it.value(), QString::SkipEmptyParts).replaceInStrings(unneccessarySpacing, QStringLiteral(" "));
#endif // QT_VERSION >= 0x050e00
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
    static const QRegularExpression split(QStringLiteral("\\s*([,]+|[,]*\\b[au]nd\\b|[;]|&|\u00b7|\u2022|\\n|\\s{4,})\\s*"));
#if QT_VERSION >= 0x050e00
    const QStringList authorTokenList = internalText.split(split, Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
    const QStringList authorTokenList = internalText.split(split, QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00

    bool containsSpace = true;
    for (QStringList::ConstIterator it = authorTokenList.constBegin(); containsSpace && it != authorTokenList.constEnd(); ++it)
        containsSpace = (*it).contains(QChar(' '));

    QList<QSharedPointer<Person> > result;
    result.reserve(authorTokenList.size());
    if (containsSpace) {
        /// Tokens look like "John Smith"
        for (const QString &authorToken : authorTokenList) {
            QSharedPointer<Person> person = Private::personFromString(authorToken, nullptr, 1, nullptr);
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
                QSharedPointer<Person> person = Private::personFromString(lastname, nullptr, 1, nullptr);
                if (!person.isNull())
                    result.append(person);
            } else
                break;
        }
    }

    return result;
}

QSharedPointer<Person> FileImporterBibTeX::personFromString(const QString &name, const int line_number, QObject *parent)
{
    // TODO Merge with FileImporter::splitName
    return Private::personFromString(name, nullptr, line_number, parent);
}

void FileImporterBibTeX::parsePersonList(const QString &text, Value &value, const int line_number, QObject *parent)
{
    Private::parsePersonList(text, value, nullptr, line_number, parent);
}


int FileImporterBibTeX::editionStringToNumber(const QString &editionString, bool *ok)
{
    // TODO   This function is not really used (much) yet. It can clean up
    // several types of non-standard variations used to describe the edition
    // of a publication, but the question is whether this re-write should be
    // done without the user's approval.
    // Thus, the intention is to make use of this function in a future 'clean
    // BibTeX file' feature which is to be triggered by the user and which
    // would rewrite and update a bibliography file. Ideas would include not
    // just rewriting the edition, but, for example updating URLs from HTTP
    // to HTTPS, retrieving new or updated data, etc.

    *ok = true; // Assume the best for now as this function only returns if successful (except for last return)

    // Test if string is just digits that can be converted into a positive int
    bool toIntok = false;
    int edition = editionString.toInt(&toIntok);
    if (toIntok && edition >= 1)
        return edition;

    const QString editionStringLower = editionString.toLower().trimmed();

    // Test if string starts with digits, followed by English ordinal suffices
    static const QRegularExpression englishOrdinal(QStringLiteral("^([1-9][0-9]*)(st|nd|rd|th|[.])($| edition)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch englishOrdinalMatch = englishOrdinal.match(editionStringLower);
    if (englishOrdinalMatch.hasMatch()) {
        bool toIntok = false;
        int edition = englishOrdinalMatch.captured(1).toInt(&toIntok);
        if (toIntok && edition >= 1)
            return edition;
    }

    // Test if string is a spelled-out English ordinal (in some cases consider mis-spellings)
    if (editionStringLower == QLatin1String("first"))
        return 1;
    else if (editionStringLower == QLatin1String("second"))
        return 2;
    else if (editionStringLower == QLatin1String("third"))
        return 3;
    else if (editionStringLower == QLatin1String("fourth"))
        return 4;
    else if (editionStringLower == QLatin1String("fifth") || editionStringLower == QLatin1String("fivth"))
        return 5;
    else if (editionStringLower == QLatin1String("sixth"))
        return 6;
    else if (editionStringLower == QLatin1String("seventh"))
        return 7;
    else if (editionStringLower == QLatin1String("eighth") || editionStringLower == QLatin1String("eigth"))
        return 8;
    else if (editionStringLower == QLatin1String("nineth") || editionStringLower == QLatin1String("ninth"))
        return 9;
    else if (editionStringLower == QLatin1String("tenth"))
        return 10;
    else if (editionStringLower == QLatin1String("eleventh"))
        return 11;
    else if (editionStringLower == QLatin1String("twelvth") || editionStringLower == QLatin1String("twelfth"))
        return 12;
    else if (editionStringLower == QLatin1String("thirdteeth"))
        return 13;
    else if (editionStringLower == QLatin1String("fourteenth"))
        return 14;
    else if (editionStringLower == QLatin1String("fifteenth"))
        return 15;
    else if (editionStringLower == QLatin1String("sixteenth"))
        return 16;

    // No test above succeeded, so communicate that conversion failed
    *ok = false;
    return 0;
}

void FileImporterBibTeX::contextSensitiveSplit(const QString &text, QStringList &segments)
{
    // TODO Merge with FileImporter::splitName and FileImporterBibTeX::personFromString
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

QString FileImporterBibTeX::rstrip(const QString &text)
{
    for (int p = text.length() - 1; p >= 0; --p)
        if (!text.at(p).isSpace())
            return text.left(p + 1);
    return QString();
}

void FileImporterBibTeX::setCommentHandling(CommentHandling commentHandling) {
    d->commentHandling = commentHandling;
}
