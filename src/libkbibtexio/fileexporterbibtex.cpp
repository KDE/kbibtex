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
#include <typeinfo>

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
#include <bibtexentries.h>
#include <bibtexfields.h>

#include "fileexporterbibtex.h"

FileExporterBibTeX::FileExporterBibTeX()
        : FileExporter(), m_stringOpenDelimiter(QChar('"')), m_stringCloseDelimiter(QChar('"')), m_keywordCasing(KBibTeX::cLowerCase), m_quoteComment(qcNone), m_encoding(QLatin1String("latex")), m_protectCasing(false), cancelFlag(false)
{
// nothing
}

FileExporterBibTeX::~FileExporterBibTeX()
{
// nothing
}

void FileExporterBibTeX::setEncoding(const QString& encoding)
{
    m_encoding = encoding;
}

void FileExporterBibTeX::setStringDelimiters(const QChar& stringOpenDelimiter, const QChar& stringCloseDelimiter)
{
    m_stringOpenDelimiter = stringOpenDelimiter;
    m_stringCloseDelimiter = stringCloseDelimiter;
}

void FileExporterBibTeX::setKeywordCasing(KBibTeX::Casing keywordCasing)
{
    m_keywordCasing = keywordCasing;
}

void FileExporterBibTeX::setQuoteComment(QuoteComment quoteComment)
{
    m_quoteComment = quoteComment;
}

void FileExporterBibTeX::setProtectCasing(bool protectCasing)
{
    m_protectCasing = protectCasing;
}

bool FileExporterBibTeX::save(QIODevice* iodevice, const File* bibtexfile, QStringList * /*errorLog*/)
{
    // m_mutex.lock(); // FIXME: required?
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
                    if (comment == NULL || !comment->text().startsWith("x-kbibtex-encoding="))
                        remainingList.append(*it);
                }
            }
        }
    }

    int totalElements = (int) bibtexfile->count();
    int currentPos = 0;

    QTextStream stream(iodevice);
    stream.setCodec(m_encoding == QLatin1String("latex") ? "UTF-8" : m_encoding.toAscii());
    if (m_encoding != QLatin1String("latex"))
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

    // m_mutex.unlock(); // FIXME: required?
    return result && !cancelFlag;
}

bool FileExporterBibTeX::save(QIODevice* iodevice, const Element* element, QStringList * /*errorLog*/)
{
    // m_mutex.lock(); // FIXME: required?
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

    //m_mutex.unlock(); // FIXME: required?
    return result && !cancelFlag;
}

void FileExporterBibTeX::cancel()
{
    cancelFlag = TRUE;
}

bool FileExporterBibTeX::writeEntry(QTextStream &stream, const Entry& entry)
{
    BibTeXEntries *be = BibTeXEntries::self();
    BibTeXFields *bf = BibTeXFields::self();

    stream << "@" << be->format(entry.type(), m_keywordCasing) << "{" << entry.id();

    for (Entry::ConstIterator it = entry.begin(); it != entry.end(); ++it) {
        QString key = it.key();
        Value value = it.value();
        QString text = valueToBibTeX(value, key);
        if (text.isEmpty()) kWarning() << "Value for field " << key << " is empty" << endl;
        if (m_protectCasing && dynamic_cast<PlainText*>(value.first()) != NULL && (key == Entry::ftTitle || key == Entry::ftBookTitle || key == Entry::ftSeries))
            addProtectiveCasing(text);
        stream << "," << endl << "\t" << bf->format(key, m_keywordCasing) << " = " << text;
    }
    stream << endl << "}" << endl << endl;
    return TRUE;
}

bool FileExporterBibTeX::writeMacro(QTextStream &stream, const Macro& macro)
{
    BibTeXEntries *be = BibTeXEntries::self();

    QString text = valueToBibTeX(macro.value());
    if (m_protectCasing)
        addProtectiveCasing(text);

    stream << "@" << be->format(QLatin1String("String"), m_keywordCasing) << "{ " << macro.key() << " = " << text << " }" << endl << endl;

    return TRUE;
}

bool FileExporterBibTeX::writeComment(QTextStream &stream, const Comment& comment)
{
    BibTeXEntries *be = BibTeXEntries::self();

    QString text = comment.text() ;
    escapeLaTeXChars(text);
    if (m_encoding == "latex")
        text = EncoderLaTeX::currentEncoderLaTeX() ->encode(text);

    if (comment.useCommand() || m_quoteComment == qcCommand)
        stream << "@" << be->format(QLatin1String("Comment"), m_keywordCasing) << "{" << text << "}" << endl << endl;
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
    BibTeXEntries *be = BibTeXEntries::self();
    stream << "@" << be->format(QLatin1String("Preamble"), m_keywordCasing) << "{" << valueToBibTeX(preamble.value()) << "}" << endl << endl;

    return TRUE;
}

QString FileExporterBibTeX::valueToBibTeX(const Value& value, const QString& key)
{
    if (value.isEmpty())
        return "";

    EncoderLaTeX *encoder = EncoderLaTeX::currentEncoderLaTeX();
    QString result = "";
    bool isOpen = false;
    const ValueItem* prev = NULL;
    for (QList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
        const MacroKey *macroKey = dynamic_cast<const MacroKey*>(*it);
        if (macroKey != NULL) {
            if (isOpen) result.append('}');
            isOpen = false;
            if (!result.isEmpty()) result.append(" # ");
            result.append(macroKey->text());
            prev = macroKey;
        } else {
            const PlainText *plainText = dynamic_cast<const PlainText*>(*it);
            if (plainText != NULL) {
                if (!isOpen) {
                    if (!result.isEmpty()) result.append(" # ");
                    result.append('{');
                } else if (prev != NULL && typeid(*prev) == typeid(PlainText))
                    result.append(' ');
                else
                    result.append("} # {");
                isOpen = true;
                result.append(encoder->encode(plainText->text()));
                prev = plainText;
            } else {
                const VerbatimText *verbatimText = dynamic_cast<const VerbatimText*>(*it);
                if (verbatimText != NULL) {
                    if (!isOpen) {
                        if (!result.isEmpty()) result.append(" # ");
                        result.append('{');
                    } else if (prev != NULL && typeid(*prev) == typeid(VerbatimText)) {
                        if (key.toLower().startsWith(Entry::ftUrl) || key.toLower().startsWith(Entry::ftLocalFile) || key.toLower().startsWith(Entry::ftDOI))
                            result.append("; ");
                        else
                            result.append(' ');
                    } else
                        result.append("} # {");
                    isOpen = true;
                    result.append(verbatimText->text());
                    prev = verbatimText;
                } else {
                    const Person *person = dynamic_cast<const Person*>(*it);
                    if (person != NULL) {
                        if (!isOpen) {
                            if (!result.isEmpty()) result.append(" # ");
                            result.append('{');
                        } else if (prev != NULL && typeid(*prev) == typeid(Person))
                            result.append(" and ");
                        else
                            result.append("} # {");
                        isOpen = true;

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

                        result.append(encoder->encode(thisName));
                        prev = person;
                    } else {
                        const Keyword *keyword = dynamic_cast<const Keyword*>(*it);
                        if (keyword != NULL) {
                            if (!isOpen) {
                                if (!result.isEmpty()) result.append(" # ");
                                result.append('{');
                            } else if (prev != NULL && typeid(*prev) == typeid(Keyword))
                                result.append("; ");
                            else
                                result.append("} # {");
                            isOpen = true;

                            result.append(encoder->encode(keyword->text()));
                            prev = keyword;
                        }
                    }
                }
            }
        }
        prev = *it;
    }

    if (isOpen) result.append('}');
    return result;
}

QString FileExporterBibTeX::escapeLaTeXChars(QString &text)
{
    return text.replace("&", "\\&"); // FIXME more?
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
