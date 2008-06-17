/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
        : FileExporter(),
        m_stringOpenDelimiter(stringOpenDelimiter), m_stringCloseDelimiter(stringCloseDelimiter), m_keywordCasing(keywordCasing), m_quoteComment(quoteComment), m_encoding(encoding), m_protectCasing(protectCasing), cancelFlag(FALSE)
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

    QLinkedList<Preamble*> preambleList;
    QLinkedList<Macro*> macroList;
    QLinkedList<Entry*> crossRefingEntryList;
    QLinkedList<Element*> remainingList;

    for (File::ElementList::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !cancelFlag; it++) {
        Preamble *preamble = dynamic_cast<Preamble*>(*it);
        if (preamble != NULL)
            preambleList.append(preamble);
        else {
            Macro *macro = dynamic_cast<Macro*>(*it);
            if (macro != NULL)
                macroList.append(macro);
            else {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if ((entry != NULL) && (entry->getField(EntryField::ftCrossRef) != NULL))
                    crossRefingEntryList.append(entry);
                else
                    remainingList.append(*it);
            }
        }
    }

    QTextStream stream(iodevice);
    stream.setCodec(m_encoding == "latex" ? "UTF-8" : m_encoding.toAscii());

    /** first, write preambles and strings (macros) at the beginning */
    for (QLinkedList<Preamble*>::ConstIterator it = preambleList.begin(); it != preambleList.end() && result && !cancelFlag; it++)
        result &= writePreamble(stream, *it);

    for (QLinkedList<Macro*>::ConstIterator it = macroList.begin(); it != macroList.end() && result && !cancelFlag; it++)
        result &= writeMacro(stream, *it);

    /** second, write cross-referencing elements */
    for (QLinkedList<Entry*>::ConstIterator it = crossRefingEntryList.begin(); it != crossRefingEntryList.end() && result && !cancelFlag; it++)
        result &= writeEntry(stream, *it);

    /** third, write remaining elements */
    for (QLinkedList<Element*>::ConstIterator it = remainingList.begin(); it != remainingList.end() && result && !cancelFlag; it++) {
        Entry *entry = dynamic_cast<Entry*>(*it);
        if (entry != NULL)
            result &= writeEntry(stream, entry);
        else {
            Comment *comment = dynamic_cast<Comment*>(*it);
            if (comment != NULL)
                result &= writeComment(stream, comment);
        }
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
        result |= writeEntry(stream, entry);
    else {
        const Macro * macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL)
            result |= writeMacro(stream, macro);
        else {
            const Comment * comment = dynamic_cast<const Comment*>(element);
            if (comment != NULL)
                result |= writeComment(stream, comment);
            else {
                const Preamble * preamble = dynamic_cast<const Preamble*>(element);
                if (preamble != NULL)
                    result |= writePreamble(stream, preamble);
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

bool FileExporterBibTeX::writeEntry(QTextStream &stream, const Entry* entry)
{
    stream << "@" << applyKeywordCasing(entry->entryTypeString()) << "{" << entry->id();

    for (Entry::EntryFields::ConstIterator it = entry->begin(); it != entry->end(); ++it) {
        EntryField *field = *it;
        QString text = valueToString(field->value(), field->fieldType());
        if (m_protectCasing && dynamic_cast<PlainText*>(field->value()->items.first()) != NULL && (field->fieldType() == EntryField::ftTitle || field->fieldType() == EntryField::ftBookTitle || field->fieldType() == EntryField::ftSeries))
            addProtectiveCasing(text);
        stream << "," << endl << "\t" << field->fieldTypeName() << " = " << text;
    }
    stream << endl << "}" << endl << endl;
    return TRUE;
}

bool FileExporterBibTeX::writeMacro(QTextStream &stream, const Macro *macro)
{
    QString text = valueToString(macro->value());
    if (m_protectCasing)
        addProtectiveCasing(text);

    stream << "@" << applyKeywordCasing("String") << "{ " << macro->key() << " = " << text << " }" << endl << endl;

    return TRUE;
}

bool FileExporterBibTeX::writeComment(QTextStream &stream, const Comment *comment)
{
    QString text = comment->text() ;
    escapeLaTeXChars(text);
    if (m_encoding == "latex")
        text = EncoderLaTeX::currentEncoderLaTeX() ->encode(text);

    if (m_quoteComment != qcCommand) {
        QStringList commentLines = text.split('\n', QString::SkipEmptyParts);
        for (QStringList::Iterator it = commentLines.begin(); it != commentLines.end(); it++) {
            if (m_quoteComment == qcPercentSign)
                stream << "% ";
            stream << (*it) << endl;
        }
        stream << endl;
    } else {
        stream << "@" << applyKeywordCasing("Comment") << "{" << text << "}" << endl << endl;
    }
    return TRUE;
}

bool FileExporterBibTeX::writePreamble(QTextStream &stream, const Preamble* preamble)
{
    stream << "@" << applyKeywordCasing("Preamble") << "{" << valueToString(preamble->value()) << "}" << endl << endl;

    return TRUE;
}

QString FileExporterBibTeX::valueToString(const Value *value, const EntryField::FieldType fieldType)
{
    if (value == NULL)
        return "";

    QString result;
    bool isFirst = TRUE;
    EncoderLaTeX *encoder = EncoderLaTeX::currentEncoderLaTeX();

    for (QLinkedList<ValueItem*>::ConstIterator it = value->items.begin(); it != value->items.end(); ++it) {
        if (!isFirst)
            result.append(" # ");
        else
            isFirst = FALSE;

        MacroKey *macroKey = dynamic_cast<MacroKey*>(*it);
        if (macroKey != NULL)
            result.append(macroKey->text());
        else {
            QString text;
            PersonContainer *personContainer = dynamic_cast<PersonContainer*>(*it);
            PlainText *plainText = dynamic_cast<PlainText*>(*it);
            KeywordContainer *keywordContainer = dynamic_cast<KeywordContainer*>(*it);

            if (plainText != NULL)
                text = plainText->text();
            else if (keywordContainer != NULL) {
                bool first = TRUE;
                for (QLinkedList<Keyword*>::ConstIterator it = keywordContainer->keywords.begin(); it != keywordContainer->keywords.end(); ++it) {
                    if (!first)
                        text.append(", ");
                    else
                        first = FALSE;
                    text.append((*it)->text());
                }
            } else if (personContainer != NULL) {
                bool first = TRUE;
                for (QLinkedList<Person*>::ConstIterator it = personContainer->persons.begin(); it != personContainer->persons.end(); ++it) {
                    if (!first)
                        text.append(" and ");
                    else
                        first = FALSE;

                    QString v = (*it)->firstName();
                    if (!v.isEmpty()) {
                        bool requiresQuoting = requiresPersonQuoting(v, FALSE);
                        if (requiresQuoting) text.append("{");
                        text.append(v);
                        if (requiresQuoting) text.append("}");
                        text.append(" ");
                    }

                    v = (*it)->lastName();
                    if (!v.isEmpty()) {
                        bool requiresQuoting = requiresPersonQuoting(v, TRUE);
                        if (requiresQuoting) text.append("{");
                        text.append(v);
                        if (requiresQuoting) text.append("}");
                    }
                }
            }

            escapeLaTeXChars(text);

            if (m_encoding == "latex")
                text = encoder->encodeSpecialized(text, fieldType);

            /** if the text to save contains a quote char ("),
              * force string delimiters to be curly brackets,
              * as quote chars as string delimiters would result
              * in parser failures
              */
            QChar stringOpenDelimiter = m_stringOpenDelimiter;
            QChar stringCloseDelimiter = m_stringCloseDelimiter;
            if (result.contains('"') && (m_stringOpenDelimiter == '"' || m_stringCloseDelimiter == '"')) {
                stringOpenDelimiter = '{';
                stringCloseDelimiter = '}';
            }

            result.append(stringOpenDelimiter).append(text).append(stringCloseDelimiter);
        }
    }

    return result;
}

void FileExporterBibTeX::escapeLaTeXChars(QString &text)
{
    text.replace("&", "\\&");
}

QString FileExporterBibTeX::applyKeywordCasing(const QString &keyword)
{
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
        return FALSE;
    else if (!isLastName && !text.contains(" and "))
        return FALSE;
    else if (text[0] != '{' || text[text.length()-1] != '}')
        return TRUE;

    int bracketCounter = 0;
    for (int i = text.length() - 1;i >= 0;--i) {
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
