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
#include <QTextCodec>
#include <QTextStream>
#include <QStringList>

#include <KDebug>

#include <file.h>
#include <element.h>
#include <entry.h>
#include <macro.h>
#include <preamble.h>
#include <value.h>
#include <comment.h>
#include <encoderlatex.h>

#include "fileexporterbibtex.h"

using namespace KBibTeX::IO;

FileExporterBibTeX::FileExporterBibTeX(const QString& encoding, const QChar& stringOpenDelimiter, const QChar& stringCloseDelimiter, KeywordCasing keywordCasing, QuoteComment quoteComment, bool protectCasing)
        : FileExporter(), m_stringOpenDelimiter(stringOpenDelimiter), m_stringCloseDelimiter(stringCloseDelimiter), m_keywordCasing(keywordCasing), m_quoteComment(quoteComment), m_encoding(encoding), m_protectCasing(protectCasing), cancelFlag(FALSE)
{
// nothing
}

FileExporterBibTeX::~FileExporterBibTeX()
{
// nothing
}

bool FileExporterBibTeX::save(QIODevice* iodevice, const File* bibtexfile, QStringList * /*errorLog*/)
{
    m_mutex.lock();
    bool result = TRUE;

    /**
      * Categorize elements from the bib file into four groups,
      * to ensure that BibTeX finds all connected elements
      * in the correct order.
      */

    QList<Comment*> parameterCommentsList;
    QList<Preamble*> preambleList;
    QList<Macro*> macroList;
    QList<Entry*> crossRefingEntryList;
    QList<Element*> remainingList;

    for (File::ConstIterator it = bibtexfile->begin(); it != bibtexfile->end() && result && !cancelFlag; it++) {
        Preamble *preamble = dynamic_cast<Preamble*>(*it);
        if (preamble != NULL)
            preambleList.append(preamble);
        else {
            Macro *macro = dynamic_cast<Macro*>(*it);
            if (macro != NULL)
                macroList.append(macro);
            else {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if ((entry != NULL) && entry->contains(Entry::ftCrossRef))
                    crossRefingEntryList.append(entry);
                else {
                    Comment *comment = dynamic_cast<Comment*>(*it);
                    QString commentText = QString::null;
                    /** check if this file requests a special encoding */
                    if (comment != NULL && comment->useCommand() && ((commentText = comment->text())).startsWith("x-kbibtex-encoding=")) {
                        QString encoding = commentText.mid(19);
                        qDebug() << "Old x-kbibtex-encoding is \"" << encoding << "\"" << endl;
                    } else
                        remainingList.append(*it);
                }
            }
        }
    }

    int totalElements = (int) bibtexfile->count();
    int currentPos = 0;

    QTextStream stream(iodevice);
    stream.setCodec(m_encoding == "latex" ? "UTF-8" : m_encoding.toAscii());
    parameterCommentsList << new Comment("x-kbibtex-encoding=" + m_encoding, true);
    qDebug() << "New x-kbibtex-encoding is \"" << m_encoding << "\"" << endl;

    /** before anything else, write parameter comments */
    for (QList<Comment*>::ConstIterator it = parameterCommentsList.begin(); it != parameterCommentsList.end() && result && !cancelFlag; it++) {
        result &= writeComment(stream, **it);
        emit progress(++currentPos, totalElements);
    }

    /** first, write preambles and strings (macros) at the beginning */
    for (QList<Preamble*>::ConstIterator it = preambleList.begin(); it != preambleList.end() && result && !cancelFlag; it++) {
        result &= writePreamble(stream, **it);
        emit progress(++currentPos, totalElements);
    }

    for (QList<Macro*>::ConstIterator it = macroList.begin(); it != macroList.end() && result && !cancelFlag; it++) {
        result &= writeMacro(stream, **it);
        emit progress(++currentPos, totalElements);
    }

    /** second, write cross-referencing elements */
    for (QList<Entry*>::ConstIterator it = crossRefingEntryList.begin(); it != crossRefingEntryList.end() && result && !cancelFlag; it++) {
        result &= writeEntry(stream, **it);
        emit progress(++currentPos, totalElements);
    }

    /** third, write remaining elements */
    for (QList<Element*>::ConstIterator it = remainingList.begin(); it != remainingList.end() && result && !cancelFlag; it++) {
        Entry *entry = dynamic_cast<Entry*>(*it);
        if (entry != NULL)
            result &= writeEntry(stream, *entry);
        else {
            Comment *comment = dynamic_cast<Comment*>(*it);
            if (comment != NULL)
                result &= writeComment(stream, *comment);
        }
        emit progress(++currentPos, totalElements);
    }

    m_mutex.unlock();
    return result && !cancelFlag;
}

bool FileExporterBibTeX::save(QIODevice* iodevice, const Element* element, QStringList * /*errorLog*/)
{
    m_mutex.lock();
    bool result = FALSE;

    QTextStream stream(iodevice);
    stream.setCodec(m_encoding == "latex" ? "UTF-8" : m_encoding.toAscii());

    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL)
        result |= writeEntry(stream, *entry);
    else {
        const Macro * macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL)
            result |= writeMacro(stream, *macro);
        else {
            const Comment * comment = dynamic_cast<const Comment*>(element);
            if (comment != NULL)
                result |= writeComment(stream, *comment);
            else {
                const Preamble * preamble = dynamic_cast<const Preamble*>(element);
                if (preamble != NULL)
                    result |= writePreamble(stream, *preamble);
            }
        }
    }

    m_mutex.unlock();
    return result && !cancelFlag;
}

void FileExporterBibTeX::cancel()
{
    cancelFlag = TRUE;
}

bool FileExporterBibTeX::writeEntry(QTextStream &stream, const Entry& entry)
{
    stream << "@" << applyKeywordCasing(entry.type()) << "{" << entry.id();

    for (Entry::ConstIterator it = entry.begin(); it != entry.end(); ++it) {
        QString key = it.key();
        Value value = it.value();
        QString text = valueToBibTeX(value, key);
        if (text.isEmpty()) kWarning() << "Value for field " << key << " is empty" << endl;
        if (m_protectCasing && dynamic_cast<PlainText*>(value.first()) != NULL && (key == Entry::ftTitle || key == Entry::ftBookTitle || key == Entry::ftSeries))
            addProtectiveCasing(text);
        stream << "," << endl << "\t" << key << " = " << text;
    }
    stream << endl << "}" << endl << endl;
    return TRUE;
}

bool FileExporterBibTeX::writeMacro(QTextStream &stream, const Macro& macro)
{
    QString text = valueToBibTeX(macro.value());
    if (m_protectCasing)
        addProtectiveCasing(text);

    stream << "@" << applyKeywordCasing("String") << "{ " << macro.key() << " = " << text << " }" << endl << endl;

    return TRUE;
}

bool FileExporterBibTeX::writeComment(QTextStream &stream, const Comment& comment)
{
    QString text = comment.text() ;
    escapeLaTeXChars(text);
    if (m_encoding == "latex")
        text = EncoderLaTeX::currentEncoderLaTeX() ->encode(text);

    if (comment.useCommand() || m_quoteComment == qcCommand)
        stream << "@" << applyKeywordCasing("Comment") << "{" << text << "}" << endl << endl;
    else    if (m_quoteComment == qcPercentSign) {
        QStringList commentLines = text.split('\n', QString::SkipEmptyParts);
        for (QStringList::Iterator it = commentLines.begin(); it != commentLines.end(); it++) {
            if (m_quoteComment == qcPercentSign)
                stream << "% ";
            stream << (*it) << endl;
        }
        stream << endl;
    } else
        stream << text << endl << endl;

    return TRUE;
}

bool FileExporterBibTeX::writePreamble(QTextStream &stream, const Preamble& preamble)
{
    stream << "@" << applyKeywordCasing("Preamble") << "{" << valueToBibTeX(preamble.value()) << "}" << endl << endl;

    return TRUE;
}

QString FileExporterBibTeX::valueToBibTeX(const Value& value, const QString& key)
{
    enum ValueItemType { VITOther = 0, VITPerson, VITKeyword} lastItem;
    lastItem = VITOther;

    if (value.isEmpty())
        return "";

    QString result = "";
    QString accumulatedText = "";
    for (QList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
        MacroKey *macroKey = dynamic_cast<MacroKey*>(*it);
        if (macroKey != NULL) {
            bool nonEmptyFlush = flushAccumulatedText(accumulatedText, result, key);
            if (nonEmptyFlush)
                result.append(" # ");
            result.append(macroKey->text());
            lastItem = VITOther;
        } else {
            QString text;
            PlainText *plainText = dynamic_cast<PlainText*>(*it);
            if (plainText != NULL) {
                bool nonEmptyFlush = flushAccumulatedText(accumulatedText, result, key);
                if (nonEmptyFlush)
                    result.append(" # ");
                accumulatedText = plainText->text();
                lastItem = VITOther;
            } else {
                Person *person = dynamic_cast<Person*>(*it);

                if (person != NULL) {
                    if (lastItem != VITPerson) {
                        bool nonEmptyFlush = flushAccumulatedText(accumulatedText, result, key);
                        if (nonEmptyFlush)
                            result.append(" # ");
                    } else
                        accumulatedText.append(" and ");

                    QString thisName = "";
                    QString v = person->firstName();
                    if (!v.isEmpty()) {
                        bool requiresQuoting = requiresPersonQuoting(v, false);
                        if (requiresQuoting) thisName.append("{");
                        thisName.append(v);
                        if (requiresQuoting) thisName.append("}");
                        thisName.append(" ");
                    }

                    v = person->lastName();
                    if (!v.isEmpty()) {
                        bool requiresQuoting = requiresPersonQuoting(v, false);
                        if (requiresQuoting) thisName.append("{");
                        thisName.append(v);
                        if (requiresQuoting) thisName.append("}");
                    }

                    // TODO: Prefix and suffix

                    accumulatedText.append(thisName);
                    lastItem = VITPerson;
                } else {
                    Keyword *keyword = dynamic_cast<Keyword*>(*it);
                    if (keyword != NULL) {
                        if (lastItem != VITKeyword) {
                            bool nonEmptyFlush = flushAccumulatedText(accumulatedText, result, key);
                            if (nonEmptyFlush)
                                result.append(" # ");
                        } else
                            accumulatedText.append("; ");

                        accumulatedText.append(keyword->text());
                        lastItem = VITKeyword;
                    } else
                        lastItem = VITOther;
                }
            }
        }

    }

    flushAccumulatedText(accumulatedText, result, key);

    return result;
}

bool FileExporterBibTeX::flushAccumulatedText(QString &accumulatedText, QString &result, const QString& key)
{
    if (accumulatedText.isEmpty())
        return false;

    escapeLaTeXChars(accumulatedText);

    EncoderLaTeX *encoder = EncoderLaTeX::currentEncoderLaTeX();
    // FIXME if (m_encoding == "latex")
    accumulatedText = encoder->encodeSpecialized(accumulatedText, key);

    /** if the text to save contains a quote char ("),
      * force string delimiters to be curly brackets,
      * as quote chars as string delimiters would result
      * in parser failures
      */
    QChar stringOpenDelimiter = '{'; // FIXME m_stringOpenDelimiter;
    QChar stringCloseDelimiter = '}'; // FIXME m_stringCloseDelimiter;
    if (accumulatedText.contains('"') /* FIXME && (m_stringOpenDelimiter == '"' || m_stringCloseDelimiter == '"')*/) {
        stringOpenDelimiter = '{';
        stringCloseDelimiter = '}';
    }

    result.append(stringOpenDelimiter).append(accumulatedText).append(stringCloseDelimiter);
    accumulatedText = "";

    return true;
}

void FileExporterBibTeX::escapeLaTeXChars(QString &text)
{
    text.replace("&", "\\&");
}

QString FileExporterBibTeX::applyKeywordCasing(const QString &keyword)
{
    // TODO: kcCamelCase: Write file containing all camel cases
    switch (m_keywordCasing) {
    case kcLowerCase: return keyword.toLower();
    case kcInitialCapital: return keyword.at(0) + keyword.toLower().mid(1);
    case kcCapital: return keyword.toUpper();
    default: return keyword;
    }
}

bool FileExporterBibTeX::requiresPersonQuoting(const QString &text, bool isLastName)
{
    if (isLastName && !text.contains(" "))
        /** Last name contains NO spaces, no quoting necessary */
        return FALSE;
    else if (!isLastName && !text.contains(" and "))
        /** First name contains no " and " no quoting necessary */
        return FALSE;
    else if (text[0] != '{' || text[text.length()-1] != '}')
        /** as either last name contains spaces or first name contains " and " and there is no protective quoting yet, there must be a protective quoting added */
        return TRUE;

    int bracketCounter = 0;
    for (int i = text.length() - 1; i >= 0; --i) {
        if (text[i] == '{')
            ++bracketCounter;
        else if (text[i] == '}')
            --bracketCounter;
        if (bracketCounter == 0 && i > 0)
            return TRUE;
    }
    return FALSE;
}

void FileExporterBibTeX::addProtectiveCasing(QString &text)
{
    if ((text[0] != '"' || text[text.length()-1] != '"') && (text[0] != '{' || text[text.length()-1] != '}')) {
        /** nothing to protect, as this is no text string */
        return;
    }

    bool addBrackets = TRUE;

    if (text[1] == '{' && text[text.length() - 2] == '}') {
        addBrackets = FALSE;
        int count = 0;
        for (int i = text.length() - 2; !addBrackets && i >= 1; --i)
            if (text[i] == '{')++count;
            else if (text[i] == '}')--count;
            else if (count == 0) addBrackets = TRUE;
    }

    if (addBrackets)
        text.insert(1, '{').insert(text.length(), '}');
}
