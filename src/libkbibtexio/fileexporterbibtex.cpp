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
const QString FileExporterBibTeX::defaultStringDelimiter = QLatin1String("{}");
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
        // nothing
    }

    ~FileExporterBibTeXPrivate() {
        delete iconvLaTeX;
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        encoding = configGroup.readEntry(p->keyEncoding, p->defaultEncoding);
        QString stringDelimiter = configGroup.readEntry(p->keyStringDelimiter, p->defaultStringDelimiter);
        if (stringDelimiter.length() != 2) stringDelimiter = p->defaultStringDelimiter;
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

    void loadStateFromFile(const File *bibtexfile) {
        if (bibtexfile == NULL) return;

        if (bibtexfile->hasProperty(File::Encoding))
            encoding = bibtexfile->property(File::Encoding).toString();
        if (!forcedEncoding.isEmpty())
            encoding = forcedEncoding;
        applyEncoding(encoding);
        if (bibtexfile->hasProperty(File::StringDelimiter)) {
            QString stringDelimiter = bibtexfile->property(File::StringDelimiter).toString();
            stringOpenDelimiter = stringDelimiter[0];
            stringCloseDelimiter = stringDelimiter[1];
        }
        // FIXME due to bug in LaTeXEncoder, enforce {...} for now
        stringOpenDelimiter = QLatin1Char('{');
        stringCloseDelimiter = QLatin1Char('}');
        if (bibtexfile->hasProperty(File::QuoteComment))
            quoteComment = (QuoteComment)bibtexfile->property(File::QuoteComment).toInt();
        if (bibtexfile->hasProperty(File::KeywordCasing))
            keywordCasing = (KBibTeX::Casing)bibtexfile->property(File::KeywordCasing).toInt();
        if (bibtexfile->hasProperty(File::ProtectCasing))
            protectCasing = bibtexfile->property(File::ProtectCasing).toBool();
        if (bibtexfile->hasProperty(File::NameFormatting)) {
            /// if the user set "use global default", this property is an empty string
            /// in this case, keep default value
            const QString buffer = bibtexfile->property(File::NameFormatting).toString();
            personNameFormatting = buffer.isEmpty() ? personNameFormatting : buffer;
        }
    }

    bool writeEntry(QIODevice *iodevice, const Entry &entry) {
        BibTeXEntries *be = BibTeXEntries::self();
        BibTeXFields *bf = BibTeXFields::self();
        EncoderLaTeX *laTeXEncoder = EncoderLaTeX::instance();

        /// write start of a entry (entry type and id) in plain ASCII
        iodevice->putChar('@');
        iodevice->write(be->format(entry.type(), keywordCasing).toAscii().data());
        iodevice->putChar('{');
        iodevice->write(laTeXEncoder->convertToPlainAscii(entry.id()).toAscii());

        for (Entry::ConstIterator it = entry.constBegin(); it != entry.constEnd(); ++it) {
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
            iodevice->write(laTeXEncoder->convertToPlainAscii(bf->format(key, keywordCasing)).toAscii());
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

    bool writeMacro(QIODevice *iodevice, const Macro &macro) {
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

    bool writeComment(QIODevice *iodevice, const Comment &comment) {
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

    bool writePreamble(QIODevice *iodevice, const Preamble &preamble) {
        BibTeXEntries *be = BibTeXEntries::self();

        iodevice->putChar('@');
        iodevice->write(be->format(QLatin1String("Preamble"), keywordCasing).toAscii().data());
        iodevice->putChar('{');
        /// Remember: strings from preamble do not get encoded,
        /// may contain raw LaTeX commands and code
        iodevice->write(iconvLaTeX->encode(p->internalValueToBibTeX(preamble.value(), QString::null, leRaw)));
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    void addProtectiveCasing(QString &text) {
        if ((text[0] != '"' || text[text.length() - 1] != '"') && (text[0] != '{' || text[text.length() - 1] != '}')) {
            /** nothing to protect, as this is no text string */
            return;
        }

        bool addBrackets = true;

        if (text[1] == '{' && text[text.length() - 2] == '}') {
            addBrackets = false;
            int count = 0;
            for (int i = text.length() - 2; !addBrackets && i >= 1; --i)
                if (text[i] == '{')++count;
                else if (text[i] == '}')--count;
                else if (count == 0) addBrackets = true;
        }

        if (addBrackets)
            text.insert(1, '{').insert(text.length(), '}');
    }

    void applyEncoding(QString &encoding) {
        encoding = encoding.isEmpty() ? QLatin1String("latex") : encoding.toLower();
        delete iconvLaTeX;
        iconvLaTeX = new IConvLaTeX(encoding == QLatin1String("latex") ? QLatin1String("us-ascii") : encoding);
    }

    bool requiresPersonQuoting(const QString &text, bool isLastName) {
        if (isLastName && !text.contains(QChar(' ')))
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

bool FileExporterBibTeX::save(QIODevice *iodevice, const File *bibtexfile, QStringList * /*errorLog*/)
{
    bool result = true;

    /**
      * Categorize elements from the bib file into four groups,
      * to ensure that BibTeX finds all connected elements
      * in the correct order.
      */

    QList<QSharedPointer<const Comment> > parameterCommentsList;
    QList<QSharedPointer<const Preamble> > preambleList;
    QList<QSharedPointer<const Macro> > macroList;
    QList<QSharedPointer<const Entry> > crossRefingEntryList;
    QList<QSharedPointer<const Element> > remainingList;

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !d->cancelFlag; it++) {
        QSharedPointer<const Preamble> preamble = (*it).dynamicCast<const Preamble>();
        if (!preamble.isNull())
            preambleList.append(preamble);
        else {
            QSharedPointer<const Macro> macro = (*it).dynamicCast<const Macro>();
            if (!macro.isNull())
                macroList.append(macro);
            else {
                QSharedPointer<const Entry> entry = (*it).dynamicCast<const Entry>();
                if (!entry.isNull() && entry->contains(Entry::ftCrossRef))
                    crossRefingEntryList.append(entry);
                else {
                    QSharedPointer<const Comment> comment = (*it).dynamicCast<const Comment>();
                    /** check if this file requests a special encoding */
                    if (comment.isNull() || !comment->text().startsWith("x-kbibtex-"))
                        remainingList.append(*it);
                }
            }
        }
    }

    int totalElements = (int) bibtexfile->count();
    int currentPos = 0;

    d->loadState();
   d->loadStateFromFile(bibtexfile);

    if (d->encoding != QLatin1String("latex"))
        parameterCommentsList << QSharedPointer<const Comment>(new Comment("x-kbibtex-encoding=" + d->encoding, true));
    /// Formatting of person names is now automatically detected in BibTeX Importer module

    /** before anything else, write parameter comments */
    for (QList<QSharedPointer<const Comment> >::ConstIterator it = parameterCommentsList.constBegin(); it != parameterCommentsList.constEnd() && result && !d->cancelFlag; it++) {
        result &= d->writeComment(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    /** first, write preambles and strings (macros) at the beginning */
    for (QList<QSharedPointer<const Preamble> >::ConstIterator it = preambleList.constBegin(); it != preambleList.constEnd() && result && !d->cancelFlag; it++) {
        result &= d->writePreamble(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    for (QList<QSharedPointer<const Macro> >::ConstIterator it = macroList.constBegin(); it != macroList.constEnd() && result && !d->cancelFlag; it++) {
        result &= d->writeMacro(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    /** second, write cross-referencing elements */
    for (QList<QSharedPointer<const Entry> >::ConstIterator it = crossRefingEntryList.constBegin(); it != crossRefingEntryList.constEnd() && result && !d->cancelFlag; it++) {
        result &= d->writeEntry(iodevice, **it);
        emit progress(++currentPos, totalElements);
    }

    /** third, write remaining elements */
    for (QList<QSharedPointer<const Element> >::ConstIterator it = remainingList.constBegin(); it != remainingList.constEnd() && result && !d->cancelFlag; it++) {
        QSharedPointer<const Entry> entry = (*it).dynamicCast<const Entry>();
        if (!entry.isNull())
            result &= d->writeEntry(iodevice, *entry);
        else {
            QSharedPointer<const Comment> comment = (*it).dynamicCast<const Comment>();
            if (!comment.isNull())
                result &= d->writeComment(iodevice, *comment);
        }
        emit progress(++currentPos, totalElements);
    }

    return result && !d->cancelFlag;
}

bool FileExporterBibTeX::save(QIODevice *iodevice, const QSharedPointer<const Element> element, QStringList * /*errorLog*/)
{
    bool result = false;

    d->loadState();

    if (!d->forcedEncoding.isEmpty())
        d->encoding = d->forcedEncoding;
    d->applyEncoding(d->encoding);

    const QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull())
        result |= d->writeEntry(iodevice, *entry);
    else {
        const QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
        if (!macro.isNull())
            result |= d->writeMacro(iodevice, *macro);
        else {
            const QSharedPointer<const Comment> comment = element.dynamicCast<const Comment>();
            if (!comment.isNull())
                result |= d->writeComment(iodevice, *comment);
            else {
                const QSharedPointer<const Preamble> preamble = element.dynamicCast<const Preamble>();
                if (!preamble.isNull())
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

QString FileExporterBibTeX::valueToBibTeX(const Value &value, const QString &key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (staticFileExporterBibTeX == NULL)
        staticFileExporterBibTeX = new FileExporterBibTeX();
    return staticFileExporterBibTeX->internalValueToBibTeX(value, key, useLaTeXEncoding);
}

QString FileExporterBibTeX::internalValueToBibTeX(const Value &value, const QString &key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (value.isEmpty())
        return QString::null;

    EncoderLaTeX *encoder = useLaTeXEncoding == leLaTeX ? EncoderLaTeX::instance() : (useLaTeXEncoding == leUTF8 ? EncoderUTF8::instance() : NULL);

    QString result = "";
    bool isOpen = false;
    /// variable to memorize which closing delimiter to use
    QChar stringCloseDelimiter = d->stringCloseDelimiter;
    QSharedPointer<const ValueItem> prev;
    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        QSharedPointer<const MacroKey> macroKey = (*it).dynamicCast<const MacroKey>();
        if (!macroKey.isNull()) {
            if (isOpen) result.append(stringCloseDelimiter);
            isOpen = false;
            if (!result.isEmpty()) result.append(" # ");
            result.append(macroKey->text());
            prev = macroKey;
        } else {
            QSharedPointer<const PlainText> plainText = (*it).dynamicCast<const PlainText>();
            if (!plainText.isNull()) {
                QString textBody = encodercheck(encoder, plainText->text());
                if (!isOpen) {
                    if (!result.isEmpty()) result.append(" # ");
                    if (textBody.contains("\"")) {
                        /// fall back to {...} delimiters if text contains quotation marks
                        result.append("{");
                        stringCloseDelimiter = QLatin1Char('}');
                    } else {
                        result.append(d->stringOpenDelimiter);
                        stringCloseDelimiter = d->stringCloseDelimiter;
                    }
                } else if (!prev.dynamicCast<const PlainText>().isNull())
                    result.append(' ');
                else if (!prev.dynamicCast<const Person>().isNull()) {
                    /// handle "et al." i.e. "and others"
                    result.append(" and ");
                } else {
                    result.append(stringCloseDelimiter).append(" # ");

                    if (textBody.contains("\"")) {
                        /// fall back to {...} delimiters if text contains quotation marks
                        result.append("{");
                        stringCloseDelimiter = QLatin1Char('}');
                    } else {
                        result.append(d->stringOpenDelimiter);
                        stringCloseDelimiter = d->stringCloseDelimiter;
                    }
                }
                isOpen = true;
                result.append(textBody);
                prev = plainText;
            } else {
                QSharedPointer<const VerbatimText> verbatimText = (*it).dynamicCast<const VerbatimText>();
                if (!verbatimText.isNull()) {
                    QString textBody = verbatimText->text();
                    if (!isOpen) {
                        if (!result.isEmpty()) result.append(" # ");
                        if (textBody.contains("\"")) {
                            /// fall back to {...} delimiters if text contains quotation marks
                            result.append("{");
                            stringCloseDelimiter = QLatin1Char('}');
                        } else {
                            result.append(d->stringOpenDelimiter);
                            stringCloseDelimiter = d->stringCloseDelimiter;
                        }
                    } else if (!prev.dynamicCast<const VerbatimText>().isNull()) {
                        if (key.toLower().startsWith(Entry::ftUrl) || key.toLower().startsWith(Entry::ftLocalFile) || key.toLower().startsWith(Entry::ftDOI))
                            result.append("; ");
                        else
                            result.append(' ');
                    } else {
                        result.append(stringCloseDelimiter).append(" # ");

                        if (textBody.contains("\"")) {
                            /// fall back to {...} delimiters if text contains quotation marks
                            result.append("{");
                            stringCloseDelimiter = QLatin1Char('}');
                        } else {
                            result.append(d->stringOpenDelimiter);
                            stringCloseDelimiter = d->stringCloseDelimiter;
                        }
                    }
                    isOpen = true;
                    result.append(textBody);
                    prev = verbatimText;
                } else {
                    QSharedPointer<const Person> person = (*it).dynamicCast<const Person>();
                    if (!person.isNull()) {
                        QString firstName = person->firstName();
                        if (!firstName.isEmpty() && d->requiresPersonQuoting(firstName, false))
                            firstName = firstName.prepend("{").append("}");

                        QString lastName = person->lastName();
                        if (!lastName.isEmpty() && d->requiresPersonQuoting(lastName, true))
                            lastName = lastName.prepend("{").append("}");

                        QString suffix = person->suffix();

                        /// Fall back and enforce comma-based name formatting
                        /// if name contains a suffix like "Jr."
                        /// Otherwise name could not be parsed again reliable
                        const QString pnf = suffix.isEmpty() ? d->personNameFormatting : QLatin1String("<%l><, %s><, %f>");
                        QString thisName = encodercheck(encoder, Person::transcribePersonName(pnf, firstName, lastName, suffix));

                        if (!isOpen) {
                            if (!result.isEmpty()) result.append(" # ");
                            if (thisName.contains("\"")) {
                                /// fall back to {...} delimiters if text contains quotation marks
                                result.append("{");
                                stringCloseDelimiter = QLatin1Char('}');
                            } else {
                                result.append(d->stringOpenDelimiter);
                                stringCloseDelimiter = d->stringCloseDelimiter;
                            }
                        } else if (!prev.dynamicCast<const Person>().isNull())
                            result.append(" and ");
                        else {
                            result.append(stringCloseDelimiter).append(" # ");

                            if (thisName.contains("\"")) {
                                /// fall back to {...} delimiters if text contains quotation marks
                                result.append("{");
                                stringCloseDelimiter = QLatin1Char('}');
                            } else {
                                result.append(d->stringOpenDelimiter);
                                stringCloseDelimiter = d->stringCloseDelimiter;
                            }
                        }
                        isOpen = true;

                        result.append(thisName);
                        prev = person;
                    } else {
                        QSharedPointer<const Keyword> keyword = (*it).dynamicCast<const Keyword>();
                        if (!keyword.isNull()) {
                            QString textBody = encodercheck(encoder, keyword->text());
                            if (!isOpen) {
                                if (!result.isEmpty()) result.append(" # ");
                                if (textBody.contains("\"")) {
                                    /// fall back to {...} delimiters if text contains quotation marks
                                    result.append("{");
                                    stringCloseDelimiter = QLatin1Char('}');
                                } else {
                                    result.append(d->stringOpenDelimiter);
                                    stringCloseDelimiter = d->stringCloseDelimiter;
                                }
                            } else if (!prev.dynamicCast<const Keyword>().isNull())
                                result.append("; ");
                            else {
                                result.append(stringCloseDelimiter).append(" # ");

                                if (textBody.contains("\"")) {
                                    /// fall back to {...} delimiters if text contains quotation marks
                                    result.append("{");
                                    stringCloseDelimiter = QLatin1Char('}');
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
