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
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QAbstractItemModel>

#include <KDebug>
#include <KDialog>
#include <KLocale>
#include <KComboBox>

#include <file.h>
#include <element.h>
#include <entry.h>
#include <macro.h>
#include <preamble.h>
#include <value.h>
#include <comment.h>
#include <encoderlatex.h>
#include <encoderutf8.h>
#include <bibtexentries.h>
#include <bibtexfields.h>
#include <iconvlatex.h>

#include "fileexporterbibtex.h"

#define encodercheck(encoder, text) ((encoder)?(encoder)->encode((text)):(text))

const QString FileExporterBibTeX::keyEncoding = QLatin1String("encoding");
const QString FileExporterBibTeX::defaultEncoding = QLatin1String("LaTeX");
const QString FileExporterBibTeX::keyStringDelimiter = QLatin1String("stringDelimiter");
const QString FileExporterBibTeX::defaultStringDelimiter = QLatin1String("\"\"");
const QString FileExporterBibTeX::keyQuoteComment = QLatin1String("quoteComment");
const FileExporterBibTeX::QuoteComment FileExporterBibTeX::defaultQuoteComment = FileExporterBibTeX::qcNone;
const QString FileExporterBibTeX::keyKeywordCasing = QLatin1String("keywordCasing");
const KBibTeX::Casing FileExporterBibTeX::defaultKeywordCasing = KBibTeX::cLowerCase;
const QString FileExporterBibTeX::keyProtectCasing = QLatin1String("protectCasing");
const bool FileExporterBibTeX::defaultProtectCasing = true;

FileExporterBibTeX *FileExporterBibTeX::staticFileExporterBibTeX = NULL;

class FileExporterBibTeX::FileExporterBibTeXPrivate
{
private:
    FileExporterBibTeX *p;

public:
    QChar stringOpenDelimiter;
    QChar stringCloseDelimiter;
    KBibTeX::Casing keywordCasing;
    QuoteComment quoteComment;
    QString encoding, forcedEncoding;
    bool protectCasing;
    QString personNameFormatting;
    bool cancelFlag;
    IConvLaTeX *iconvLaTeX;
    KSharedConfigPtr config;
    const QString configGroupName, configGroupNameGeneral;

    FileExporterBibTeXPrivate(FileExporterBibTeX *parent)
            : p(parent), cancelFlag(false), iconvLaTeX(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName("FileExporterBibTeX"), configGroupNameGeneral("General") {
        forcedEncoding = QString::null;
        loadState();
    }

    ~FileExporterBibTeXPrivate() {
        delete iconvLaTeX;
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        encoding = configGroup.readEntry(p->keyEncoding, p->defaultEncoding);
        QString stringDelimiter = configGroup.readEntry(p->keyStringDelimiter, p->defaultStringDelimiter);
        stringOpenDelimiter = stringDelimiter[0];
        stringCloseDelimiter = stringDelimiter[1];
        keywordCasing = (KBibTeX::Casing)configGroup.readEntry(p->keyKeywordCasing, (int)p->defaultKeywordCasing);
        quoteComment = (QuoteComment)configGroup.readEntry(p->keyQuoteComment, (int)p->defaultQuoteComment);
        protectCasing = configGroup.readEntry(p->keyProtectCasing, p->defaultProtectCasing);
        personNameFormatting = configGroup.readEntry(Person::keyPersonNameFormatting, "");

        if (personNameFormatting.isEmpty()) {
            /// no person name formatting is specified for BibTeX, fall back to general setting
            KConfigGroup configGroupGeneral(config, configGroupNameGeneral);
            personNameFormatting = configGroupGeneral.readEntry(Person::keyPersonNameFormatting, Person::defaultPersonNameFormatting);
        }
    }

    bool writeEntry(QIODevice* iodevice, const Entry& entry) {
        BibTeXEntries *be = BibTeXEntries::self();
        BibTeXFields *bf = BibTeXFields::self();

        /// write start of a entry (entry type and id) in plain ASCII
        iodevice->putChar('@');
        iodevice->write(be->format(entry.type(), keywordCasing).toAscii().data());
        iodevice->putChar('{');
        iodevice->write(iconvLaTeX->encode(entry.id()));

        for (Entry::ConstIterator it = entry.begin(); it != entry.end(); ++it) {
            const QString key = it.key();
            Value value = it.value();
            if (value.isEmpty()) continue; ///< ignore empty key-value pairs

            QString text = p->internalValueToBibTeX(value, key, leUTF8);
            if (text.isEmpty()) {
                /// ignore empty key-value pairs
                kWarning() << "Value for field " << key << " is empty" << endl;
                continue;
            }

            // FIXME hack!
            if (protectCasing && typeid(*value.first()) == typeid(PlainText) && (key == Entry::ftTitle || key == Entry::ftBookTitle || key == Entry::ftSeries))
                addProtectiveCasing(text);

            iodevice->putChar(',');
            iodevice->putChar('\n');
            iodevice->putChar('\t');
            iodevice->write(iconvLaTeX->encode(bf->format(key, keywordCasing)));
            iodevice->putChar(' ');
            iodevice->putChar('=');
            iodevice->putChar(' ');
            iodevice->write(iconvLaTeX->encode(text));
        }
        iodevice->putChar('\n');
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    bool writeMacro(QIODevice* iodevice, const Macro& macro) {
        BibTeXEntries *be = BibTeXEntries::self();

        QString text = p->internalValueToBibTeX(macro.value(), QString::null, leUTF8);
        if (protectCasing)
            addProtectiveCasing(text);

        iodevice->putChar('@');
        iodevice->write(be->format(QLatin1String("String"), keywordCasing).toAscii().data());
        iodevice->putChar('{');
        iodevice->write(iconvLaTeX->encode(macro.key()));
        iodevice->putChar(' ');
        iodevice->putChar('=');
        iodevice->putChar(' ');
        iodevice->write(iconvLaTeX->encode(text));
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    bool writeComment(QIODevice* iodevice, const Comment& comment) {
        BibTeXEntries *be = BibTeXEntries::self();

        QString text = comment.text() ;

        if (comment.useCommand() || quoteComment == qcCommand) {
            iodevice->putChar('@');
            iodevice->write(be->format(QLatin1String("Comment"), keywordCasing).toAscii().data());
            iodevice->putChar('{');
            iodevice->write(iconvLaTeX->encode(text));
            iodevice->putChar('}');
            iodevice->putChar('\n');
            iodevice->putChar('\n');
        } else if (quoteComment == qcPercentSign) {
            QStringList commentLines = text.split('\n', QString::SkipEmptyParts);
            for (QStringList::Iterator it = commentLines.begin(); it != commentLines.end(); it++) {
                iodevice->putChar('%');
                iodevice->putChar(' ');
                iodevice->write(iconvLaTeX->encode(*it));
                iodevice->putChar('\n');
            }
            iodevice->putChar('\n');
        } else {
            iodevice->write(iconvLaTeX->encode(text));
            iodevice->putChar('\n');
            iodevice->putChar('\n');
        }

        return true;
    }

    bool writePreamble(QIODevice* iodevice, const Preamble& preamble) {
        BibTeXEntries *be = BibTeXEntries::self();

        iodevice->putChar('@');
        iodevice->write(be->format(QLatin1String("Preamble"), keywordCasing).toAscii().data());
        iodevice->putChar('{');
        iodevice->write(iconvLaTeX->encode(p->internalValueToBibTeX(preamble.value(), QString::null, leUTF8)));
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    void addProtectiveCasing(QString &text) {
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

    void applyEncoding(QString& encoding) {
        encoding = encoding.isEmpty() ? QLatin1String("latex") : encoding.toLower();
        delete iconvLaTeX;
        iconvLaTeX = new IConvLaTeX(encoding == QLatin1String("latex") ? QLatin1String("us-ascii") : encoding);
    }

    bool requiresPersonQuoting(const QString &text, bool isLastName) {
        if (isLastName && !text.contains(" "))
            /** Last name contains NO spaces, no quoting necessary */
            return false;
        else if (!isLastName && !text.contains(" and "))
            /** First name contains no " and " no quoting necessary */
            return false;
        else if (isLastName && !text.isEmpty() && text[0].isLower())
            /** Last name starts with lower-case character (von, van, de, ...) */
            // FIXME does not work yet
            return false;
        else if (text[0] != '{' || text[text.length() - 1] != '}')
            /** as either last name contains spaces or first name contains " and " and there is no protective quoting yet, there must be a protective quoting added */
            return true;

        int bracketCounter = 0;
        for (int i = text.length() - 1; i >= 0; --i) {
            if (text[i] == '{')
                ++bracketCounter;
            else if (text[i] == '}')
                --bracketCounter;
            if (bracketCounter == 0 && i > 0)
                return true;
        }
        return false;
    }
};


FileExporterBibTeX::FileExporterBibTeX()
        : FileExporter(), d(new FileExporterBibTeXPrivate(this))
{
// nothing
}

FileExporterBibTeX::~FileExporterBibTeX()
{
    delete d;
}

void FileExporterBibTeX::setEncoding(const QString &encoding)
{
    d->forcedEncoding = encoding;
}

bool FileExporterBibTeX::save(QIODevice* iodevice, const File* bibtexfile, QStringList * /*errorLog*/)
{
    bool result = true;

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

    for (File::ConstIterator it = bibtexfile->begin(); it != bibtexfile->end() && result && !d->cancelFlag; it++) {
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
                    if (comment == NULL || !comment->text().startsWith("x-kbibtex-"))
                        remainingList.append(*it);
                }
            }
        }
    }

    int totalElements = (int) bibtexfile->count();
    int currentPos = 0;

    loadState();
    if (bibtexfile->hasProperty(File::Encoding))
        d->encoding = bibtexfile->property(File::Encoding).toString();
    if (!d->forcedEncoding.isEmpty())
        d->encoding = d->forcedEncoding;
    d->applyEncoding(d->encoding);
    if (bibtexfile->hasProperty(File::StringDelimiter)) {
        QString stringDelimiter = bibtexfile->property(File::StringDelimiter).toString();
        d->stringOpenDelimiter = stringDelimiter[0];
        d->stringCloseDelimiter = stringDelimiter[1];
    }
    if (bibtexfile->hasProperty(File::QuoteComment))
        d->quoteComment = (QuoteComment)bibtexfile->property(File::QuoteComment).toInt();
    if (bibtexfile->hasProperty(File::KeywordCasing))
        d->keywordCasing = (KBibTeX::Casing)bibtexfile->property(File::KeywordCasing).toInt();
    if (bibtexfile->hasProperty(File::ProtectCasing))
        d->protectCasing = bibtexfile->property(File::ProtectCasing).toBool();
    if (bibtexfile->hasProperty(File::NameFormatting)) {
        /// if the user set "use global default", this property is an empty string
        /// in this case, keep default value
        const QString buffer = bibtexfile->property(File::NameFormatting).toString();
        d->personNameFormatting = buffer.isEmpty() ? d->personNameFormatting : buffer;
    }

    if (d->encoding != QLatin1String("latex"))
        parameterCommentsList << new Comment("x-kbibtex-encoding=" + d->encoding, true);
    parameterCommentsList << new Comment("x-kbibtex-personnameformatting=" + d->personNameFormatting, true);

    /** before anything else, write parameter comments */
    for (QList<Comment*>::ConstIterator it = parameterCommentsList.begin(); it != parameterCommentsList.end() && result && !d->cancelFlag; it++) {
        result &= d->writeComment(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    /** first, write preambles and strings (macros) at the beginning */
    for (QList<Preamble*>::ConstIterator it = preambleList.begin(); it != preambleList.end() && result && !d->cancelFlag; it++) {
        result &= d->writePreamble(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    for (QList<Macro*>::ConstIterator it = macroList.begin(); it != macroList.end() && result && !d->cancelFlag; it++) {
        result &= d->writeMacro(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    /** second, write cross-referencing elements */
    for (QList<Entry*>::ConstIterator it = crossRefingEntryList.begin(); it != crossRefingEntryList.end() && result && !d->cancelFlag; it++) {
        result &= d->writeEntry(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    /** third, write remaining elements */
    for (QList<Element*>::ConstIterator it = remainingList.begin(); it != remainingList.end() && result && !d->cancelFlag; it++) {
        Entry *entry = dynamic_cast<Entry*>(*it);
        if (entry != NULL)
            result &= d->writeEntry(iodevice, *entry);
        else {
            Comment *comment = dynamic_cast<Comment*>(*it);
            if (comment != NULL)
                result &= d->writeComment(iodevice, *comment);
        }
        emit progress(++currentPos, totalElements);
    }

    return result && !d->cancelFlag;
}

bool FileExporterBibTeX::save(QIODevice* iodevice, const Element* element, QStringList * /*errorLog*/)
{
    bool result = false;

    loadState();
    if (!d->forcedEncoding.isEmpty())
        d->encoding = d->forcedEncoding;
    d->applyEncoding(d->encoding);

    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL)
        result |= d->writeEntry(iodevice, *entry);
    else {
        const Macro *macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL)
            result |= d->writeMacro(iodevice, *macro);
        else {
            const Comment *comment = dynamic_cast<const Comment*>(element);
            if (comment != NULL)
                result |= d->writeComment(iodevice, *comment);
            else {
                const Preamble *preamble = dynamic_cast<const Preamble*>(element);
                if (preamble != NULL)
                    result |= d->writePreamble(iodevice, *preamble);
            }
        }
    }

    return result && !d->cancelFlag;
}

void FileExporterBibTeX::cancel()
{
    d->cancelFlag = true;
}

QString FileExporterBibTeX::valueToBibTeX(const Value& value, const QString& key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (staticFileExporterBibTeX == NULL)
        staticFileExporterBibTeX = new FileExporterBibTeX();
    else
        staticFileExporterBibTeX->loadState();
    return staticFileExporterBibTeX->internalValueToBibTeX(value, key, useLaTeXEncoding);
}

QString FileExporterBibTeX::internalValueToBibTeX(const Value& value, const QString& key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (value.isEmpty())
        return "";

    EncoderLaTeX *encoder = useLaTeXEncoding == leLaTeX ? EncoderLaTeX::instance() : (useLaTeXEncoding == leUTF8 ? EncoderUTF8::instance() : NULL);

    QString result = "";
    bool isOpen = false;
    /// variable to memorize which closing delimiter to use
    QChar stringCloseDelimiter = d->stringCloseDelimiter;
    const ValueItem* prev = NULL;
    for (QList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
        const MacroKey *macroKey = dynamic_cast<const MacroKey*>(*it);
        if (macroKey != NULL) {
            if (isOpen) result.append(stringCloseDelimiter);
            isOpen = false;
            if (!result.isEmpty()) result.append(" # ");
            result.append(macroKey->text());
            prev = macroKey;
        } else {
            const PlainText *plainText = dynamic_cast<const PlainText*>(*it);
            if (plainText != NULL) {
                QString textBody = encodercheck(encoder, plainText->text());
                if (!isOpen) {
                    if (!result.isEmpty()) result.append(" # ");
                    if (textBody.contains("\"")) {
                        /// fall back to {...} delimiters if text contains quotation marks
                        result.append("{");
                        stringCloseDelimiter = '}';
                    } else {
                        result.append(d->stringOpenDelimiter);
                        stringCloseDelimiter = d->stringCloseDelimiter;
                    }
                } else if (prev != NULL && typeid(*prev) == typeid(PlainText))
                    result.append(' ');
                else if (prev != NULL && typeid(*prev) == typeid(Person)) {
                    /// handle "et al." i.e. "and others"
                    result.append(" and ");
                } else {
                    result.append(stringCloseDelimiter).append(" # ");

                    if (textBody.contains("\"")) {
                        /// fall back to {...} delimiters if text contains quotation marks
                        result.append("{");
                        stringCloseDelimiter = '}';
                    } else {
                        result.append(d->stringOpenDelimiter);
                        stringCloseDelimiter = d->stringCloseDelimiter;
                    }
                }
                isOpen = true;
                result.append(textBody);
                prev = plainText;
            } else {
                const VerbatimText *verbatimText = dynamic_cast<const VerbatimText*>(*it);
                if (verbatimText != NULL) {
                    QString textBody = verbatimText->text();
                    if (!isOpen) {
                        if (!result.isEmpty()) result.append(" # ");
                        if (textBody.contains("\"")) {
                            /// fall back to {...} delimiters if text contains quotation marks
                            result.append("{");
                            stringCloseDelimiter = '}';
                        } else {
                            result.append(d->stringOpenDelimiter);
                            stringCloseDelimiter = d->stringCloseDelimiter;
                        }
                    } else if (prev != NULL && typeid(*prev) == typeid(VerbatimText)) {
                        if (key.toLower().startsWith(Entry::ftUrl) || key.toLower().startsWith(Entry::ftLocalFile) || key.toLower().startsWith(Entry::ftDOI))
                            result.append("; ");
                        else
                            result.append(' ');
                    } else {
                        result.append(stringCloseDelimiter).append(" # ");

                        if (textBody.contains("\"")) {
                            /// fall back to {...} delimiters if text contains quotation marks
                            result.append("{");
                            stringCloseDelimiter = '}';
                        } else {
                            result.append(d->stringOpenDelimiter);
                            stringCloseDelimiter = d->stringCloseDelimiter;
                        }
                    }
                    isOpen = true;
                    result.append(textBody);
                    prev = verbatimText;
                } else {
                    const Person *person = dynamic_cast<const Person*>(*it);
                    if (person != NULL) {
                        QString firstName = person->firstName();
                        if (!firstName.isEmpty() && d->requiresPersonQuoting(firstName, false))
                            firstName = firstName.prepend("{").append("}");

                        QString lastName = person->lastName();
                        if (!lastName.isEmpty() && d->requiresPersonQuoting(lastName, true))
                            lastName = lastName.prepend("{").append("}");

                        // TODO: Prefix and suffix

                        QString thisName = encodercheck(encoder, Person::transcribePersonName(d->personNameFormatting, firstName, lastName));

                        if (!isOpen) {
                            if (!result.isEmpty()) result.append(" # ");
                            if (thisName.contains("\"")) {
                                /// fall back to {...} delimiters if text contains quotation marks
                                result.append("{");
                                stringCloseDelimiter = '}';
                            } else {
                                result.append(d->stringOpenDelimiter);
                                stringCloseDelimiter = d->stringCloseDelimiter;
                            }
                        } else if (prev != NULL && typeid(*prev) == typeid(Person))
                            result.append(" and ");
                        else {
                            result.append(stringCloseDelimiter).append(" # ");

                            if (thisName.contains("\"")) {
                                /// fall back to {...} delimiters if text contains quotation marks
                                result.append("{");
                                stringCloseDelimiter = '}';
                            } else {
                                result.append(d->stringOpenDelimiter);
                                stringCloseDelimiter = d->stringCloseDelimiter;
                            }
                        }
                        isOpen = true;

                        result.append(encodercheck(encoder, thisName));
                        prev = person;
                    } else {
                        const Keyword *keyword = dynamic_cast<const Keyword*>(*it);
                        if (keyword != NULL) {
                            QString textBody = encodercheck(encoder, keyword->text());
                            if (!isOpen) {
                                if (!result.isEmpty()) result.append(" # ");
                                if (textBody.contains("\"")) {
                                    /// fall back to {...} delimiters if text contains quotation marks
                                    result.append("{");
                                    stringCloseDelimiter = '}';
                                } else {
                                    result.append(d->stringOpenDelimiter);
                                    stringCloseDelimiter = d->stringCloseDelimiter;
                                }
                            } else if (prev != NULL && typeid(*prev) == typeid(Keyword))
                                result.append("; ");
                            else {
                                result.append(stringCloseDelimiter).append(" # ");

                                if (textBody.contains("\"")) {
                                    /// fall back to {...} delimiters if text contains quotation marks
                                    result.append("{");
                                    stringCloseDelimiter = '}';
                                } else {
                                    result.append(d->stringOpenDelimiter);
                                    stringCloseDelimiter = d->stringCloseDelimiter;
                                }
                            }
                            isOpen = true;

                            result.append(textBody);
                            prev = keyword;
                        }
                    }
                }
            }
        }
        prev = *it;
    }

    if (isOpen) result.append(stringCloseDelimiter);
    return result;
}

QString FileExporterBibTeX::elementToString(const Element* element)
{
    QStringList result;
    const Entry *entry = dynamic_cast< const Entry *>(element);
    if (entry != NULL) {
        result << QString("ID = %1").arg(entry->id());
        for (QMap<QString, Value>::ConstIterator it = entry->begin(); it != entry->end(); ++it)
            result << QString("%1 = {%2}").arg(it.key()).arg(valueToBibTeX(it.value()));
    }
    return result.join("; ");
}

void FileExporterBibTeX::loadState()
{
    d->loadState();
}
