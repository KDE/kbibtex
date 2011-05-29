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
#include <bibtexentries.h>
#include <bibtexfields.h>
#include <iconvlatex.h>

#include "fileexporterbibtex.h"

#define encodercheck(encoder, text) ((encoder)?(encoder)->encode((text)):(text))

class ExportWidget : public QWidget
{
public:
    KComboBox *listOfEncodings;
    static const QString defaultEncoding;

    ExportWidget(QWidget *parent)
            : QWidget(parent) {
        QFormLayout *layout = new QFormLayout(this);
        setLayout(layout);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        listOfEncodings = new KComboBox(true, this);
        layout->addRow(i18n("Encoding:"), listOfEncodings);

        listOfEncodings->addItem(QLatin1String("LaTeX"));
        listOfEncodings->insertSeparator(1);
        listOfEncodings->addItems(IConvLaTeX::encodings());
        setProposedEncoding(defaultEncoding);
    }

    void setProposedEncoding(const QString &proposedEncoding) {
        QAbstractItemModel *model = listOfEncodings->model();
        int row = 0;
        QModelIndex index;
        const QString lowerPE = proposedEncoding.toLower();
        while ((index = model->index(row, 0, QModelIndex())) != QModelIndex()) {
            QString line = model->data(index).toString();
            if (line.toLower() == lowerPE) {
                listOfEncodings->lineEdit()->setText(line);
                listOfEncodings->setCurrentIndex(row);
                break;
            }
            ++row;
        }
    }
};

const QString ExportWidget::defaultEncoding = QLatin1String("LaTeX");

class FileExporterBibTeX::FileExporterBibTeXPrivate
{
private:
    FileExporterBibTeX *p;

public:
    QChar stringOpenDelimiter;
    QChar stringCloseDelimiter;
    KBibTeX::Casing keywordCasing;
    QuoteComment quoteComment;
    QString encoding;
    bool protectCasing;
    bool cancelFlag;
    IConvLaTeX *iconvLaTeX;

    FileExporterBibTeXPrivate(FileExporterBibTeX *parent)
            : p(parent), stringOpenDelimiter(QChar('"')), stringCloseDelimiter(QChar('"')), keywordCasing(KBibTeX::cLowerCase), quoteComment(qcNone), encoding(QLatin1String("latex")), protectCasing(false), cancelFlag(false) {
        iconvLaTeX = new IConvLaTeX(encoding == QLatin1String("latex") ? QLatin1String("us-ascii") : encoding);
    }

    ~FileExporterBibTeXPrivate() {
        delete iconvLaTeX;
    }

    bool writeEntry(QIODevice* iodevice, const Entry& entry) {
        BibTeXEntries *be = BibTeXEntries::self();
        BibTeXFields *bf = BibTeXFields::self();

        iodevice->putChar('@');
        iodevice->write(be->format(entry.type(), keywordCasing).toAscii().data());
        iodevice->putChar('{');
        iodevice->write(iconvLaTeX->encode(entry.id()));

        for (Entry::ConstIterator it = entry.begin(); it != entry.end(); ++it) {
            const QString key = it.key();
            Value value = it.value();
            QString text = valueToBibTeX(value, key, leUTF8);
            if (text.isEmpty()) kWarning() << "Value for field " << key << " is empty" << endl;
            if (protectCasing && dynamic_cast<PlainText*>(value.first()) != NULL && (key == Entry::ftTitle || key == Entry::ftBookTitle || key == Entry::ftSeries))
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

        QString text = valueToBibTeX(macro.value(), QString::null, leUTF8);
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
        iodevice->write(iconvLaTeX->encode(valueToBibTeX(preamble.value(), QString::null, leUTF8)));
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
};

FileExporterBibTeX::FileExporterBibTeX()
        : FileExporter(), d(new FileExporterBibTeXPrivate(this))
{
// nothing
}

FileExporterBibTeX::~FileExporterBibTeX()
{
// nothing
}

void FileExporterBibTeX::setEncoding(const QString& encoding)
{
    QString normalizedEncoding = encoding.isNull() ? QLatin1String("latex") : encoding.toLower();
    if (normalizedEncoding != d->encoding) {
        d->encoding = normalizedEncoding;
        delete d->iconvLaTeX;
        d->iconvLaTeX = new IConvLaTeX(d->encoding == QLatin1String("latex") ?  QLatin1String("us-ascii") : encoding);
    }
}

QString FileExporterBibTeX::encoding() const
{
    return d->encoding;
}

void FileExporterBibTeX::setStringDelimiters(const QChar& stringOpenDelimiter, const QChar& stringCloseDelimiter)
{
    d->stringOpenDelimiter = stringOpenDelimiter;
    d->stringCloseDelimiter = stringCloseDelimiter;
}

void FileExporterBibTeX::setKeywordCasing(KBibTeX::Casing keywordCasing)
{
    d->keywordCasing = keywordCasing;
}

void FileExporterBibTeX::setQuoteComment(QuoteComment quoteComment)
{
    d->quoteComment = quoteComment;
}

void FileExporterBibTeX::setProtectCasing(bool protectCasing)
{
    d->protectCasing = protectCasing;
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
                    if (comment == NULL || !comment->text().startsWith("x-kbibtex-encoding="))
                        remainingList.append(*it);
                }
            }
        }
    }

    int totalElements = (int) bibtexfile->count();
    int currentPos = 0;

    QString effectiveEncoding = ExportWidget::defaultEncoding;
    if (!d->encoding.isEmpty())
        effectiveEncoding = d->encoding;
    if (bibtexfile->hasProperty(File::Encoding))
        effectiveEncoding = bibtexfile->property(File::Encoding).toString();
    setEncoding(effectiveEncoding);

    if (effectiveEncoding != QLatin1String("latex"))
        parameterCommentsList << new Comment("x-kbibtex-encoding=" + effectiveEncoding, true);

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

void FileExporterBibTeX::showExportDialog(QWidget *parent, File *bibtexfile) const
{
    Q_ASSERT(bibtexfile != NULL);

    KDialog dialog(parent);
    dialog.setButtons(KDialog::Ok);

    /// default encoding is LaTeX
    QString proposedEncoding = ExportWidget::defaultEncoding;
    /// check encoding set to this exporter
    if (!d->encoding.isEmpty())
        proposedEncoding = d->encoding;
    /// encoding as stored in the File has highest precendence
    if (bibtexfile->hasProperty(File::Encoding))
        proposedEncoding = bibtexfile->property(File::Encoding).toString();

    ExportWidget exportWidget(&dialog);
    exportWidget.setProposedEncoding(proposedEncoding);
    dialog.setMainWidget(&exportWidget);

    if (dialog.exec() == QDialog::Accepted)
        bibtexfile->setProperty(File::Encoding, exportWidget.listOfEncodings->lineEdit()->text());
}

QString FileExporterBibTeX::valueToBibTeX(const Value& value, const QString& key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (value.isEmpty())
        return "";

    EncoderLaTeX *encoder = useLaTeXEncoding == leLaTeX ? EncoderLaTeX::currentEncoderLaTeX() : NULL;

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
                else if (prev != NULL && typeid(*prev) == typeid(Person)) {
                    /// handle "et al." i.e. "and others"
                    result.append(" and ");
                } else
                    result.append("} # {");
                isOpen = true;
                result.append(encodercheck(encoder, escapeLaTeXChars(plainText->text())));
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

                        result.append(encodercheck(encoder, escapeLaTeXChars(thisName)));
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

                            result.append(encodercheck(encoder, escapeLaTeXChars(keyword->text())));
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

QString FileExporterBibTeX::escapeLaTeXChars(const QString &text)
{
    QString result = text;

    const QRegExp regExpEscape("[^\\\\][&#_%]");
    int p = -1;
    while ((p = regExpEscape.indexIn(result, p + 1)) != -1)
        result = result.left(p + 1) + '\\' + result.mid(p + 1);
    return result;
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
    return false;
}
