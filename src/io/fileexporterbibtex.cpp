/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileexporterbibtex.h"

#include <typeinfo>

#include <QTextCodec>
#include <QTextStream>
#include <QStringList>

#ifdef HAVE_KF5
#include <KSharedConfig>
#include <KConfigGroup>
#endif // HAVE_KF5

#include "preferences.h"
#include "file.h"
#include "element.h"
#include "entry.h"
#include "macro.h"
#include "preamble.h"
#include "value.h"
#include "comment.h"
#include "encoderlatex.h"
#include "bibtexentries.h"
#include "bibtexfields.h"
#include "textencoder.h"
#include "logging_io.h"

FileExporterBibTeX *FileExporterBibTeX::staticFileExporterBibTeX = nullptr;

class FileExporterBibTeX::FileExporterBibTeXPrivate
{
private:
    FileExporterBibTeX *p;

public:
    QChar stringOpenDelimiter;
    QChar stringCloseDelimiter;
    KBibTeX::Casing keywordCasing;
    Preferences::QuoteComment quoteComment;
    QString encoding, forcedEncoding;
    Qt::CheckState protectCasing;
    QString personNameFormatting;
    QString listSeparator;
    bool cancelFlag;
    QTextCodec *destinationCodec;
#ifdef HAVE_KF5
    KSharedConfigPtr config;
    const QString configGroupName, configGroupNameGeneral;
#endif // HAVE_KF5

    FileExporterBibTeXPrivate(FileExporterBibTeX *parent)
            : p(parent), keywordCasing(KBibTeX::cLowerCase), quoteComment(Preferences::qcNone), protectCasing(Qt::PartiallyChecked), cancelFlag(false), destinationCodec(nullptr), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("FileExporterBibTeX")), configGroupNameGeneral(QStringLiteral("General")) {
        // nothing
    }

    void loadState() {
#ifdef HAVE_KF5
        KConfigGroup configGroup(config, configGroupName);
        encoding = configGroup.readEntry(Preferences::keyEncoding, Preferences::defaultEncoding);
        QString stringDelimiter = configGroup.readEntry(Preferences::keyStringDelimiter, Preferences::defaultStringDelimiter);
        if (stringDelimiter.length() != 2)
            stringDelimiter = Preferences::defaultStringDelimiter;
#else // HAVE_KF5
        encoding = QStringLiteral("LaTeX");
        const QString stringDelimiter = QStringLiteral("{}");
#endif // HAVE_KF5
        stringOpenDelimiter = stringDelimiter[0];
        stringCloseDelimiter = stringDelimiter[1];
#ifdef HAVE_KF5
        keywordCasing = (KBibTeX::Casing)configGroup.readEntry(Preferences::keyKeywordCasing, (int)Preferences::defaultKeywordCasing);
        quoteComment = (Preferences::QuoteComment)configGroup.readEntry(Preferences::keyQuoteComment, (int)Preferences::defaultQuoteComment);
        protectCasing = (Qt::CheckState)configGroup.readEntry(Preferences::keyProtectCasing, (int)Preferences::defaultProtectCasing);
        personNameFormatting = configGroup.readEntry(Preferences::keyPersonNameFormatting, QString());
        listSeparator = configGroup.readEntry(Preferences::keyListSeparator, Preferences::defaultListSeparator);

        if (personNameFormatting.isEmpty()) {
            /// no person name formatting is specified for BibTeX, fall back to general setting
            KConfigGroup configGroupGeneral(config, configGroupNameGeneral);
            personNameFormatting = configGroupGeneral.readEntry(Preferences::keyPersonNameFormatting, Preferences::defaultPersonNameFormatting);
        }
#else // HAVE_KF5
        keywordCasing = KBibTeX::cLowerCase;
        quoteComment = qcNone;
        protectCasing = Qt::PartiallyChecked;
        personNameFormatting = QStringLiteral("<%l><, %s><, %f>");
        listSeparator = QStringLiteral("; ");
#endif // HAVE_KF5
    }

    void loadStateFromFile(const File *bibtexfile) {
        if (bibtexfile == nullptr) return;

        if (bibtexfile->hasProperty(File::Encoding))
            encoding = bibtexfile->property(File::Encoding).toString();
        if (!forcedEncoding.isEmpty())
            encoding = forcedEncoding;
        applyEncoding(encoding);
        if (bibtexfile->hasProperty(File::StringDelimiter)) {
            QString stringDelimiter = bibtexfile->property(File::StringDelimiter).toString();
            if (stringDelimiter.length() != 2)
#ifdef HAVE_KF5
                stringDelimiter = Preferences::defaultStringDelimiter;
#else // HAVE_KF5
                stringDelimiter = QStringLiteral("{}");
#endif // HAVE_KF5
            stringOpenDelimiter = stringDelimiter[0];
            stringCloseDelimiter = stringDelimiter[1];
        }
        if (bibtexfile->hasProperty(File::QuoteComment))
            quoteComment = (Preferences::QuoteComment)bibtexfile->property(File::QuoteComment).toInt();
        if (bibtexfile->hasProperty(File::KeywordCasing))
            keywordCasing = (KBibTeX::Casing)bibtexfile->property(File::KeywordCasing).toInt();
        if (bibtexfile->hasProperty(File::ProtectCasing))
            protectCasing = (Qt::CheckState)bibtexfile->property(File::ProtectCasing).toInt();
        if (bibtexfile->hasProperty(File::NameFormatting)) {
            /// if the user set "use global default", this property is an empty string
            /// in this case, keep default value
            const QString buffer = bibtexfile->property(File::NameFormatting).toString();
            personNameFormatting = buffer.isEmpty() ? personNameFormatting : buffer;
        }
        if (bibtexfile->hasProperty(File::ListSeparator))
            listSeparator = bibtexfile->property(File::ListSeparator).toString();
    }

    bool writeEntry(QIODevice *iodevice, const Entry &entry) {
        const BibTeXEntries *be = BibTeXEntries::self();
        const BibTeXFields *bf = BibTeXFields::self();
        const EncoderLaTeX &laTeXEncoder = EncoderLaTeX::instance();

        /// write start of a entry (entry type and id) in plain ASCII
        iodevice->putChar('@');
        iodevice->write(be->format(entry.type(), keywordCasing).toLatin1().data());
        iodevice->putChar('{');
        iodevice->write(laTeXEncoder.convertToPlainAscii(entry.id()).toLatin1());

        for (Entry::ConstIterator it = entry.constBegin(); it != entry.constEnd(); ++it) {
            const QString key = it.key();
            Value value = it.value();
            if (value.isEmpty()) continue; ///< ignore empty key-value pairs

            QString text = p->internalValueToBibTeX(value, key, leUTF8);
            if (text.isEmpty()) {
                /// ignore empty key-value pairs
                qCWarning(LOG_KBIBTEX_IO) << "Value for field " << key << " is empty" << endl;
                continue;
            }

            // FIXME hack!
            const QSharedPointer<ValueItem> first = *value.constBegin();
            if (PlainText::isPlainText(*first) && (key == Entry::ftTitle || key == Entry::ftBookTitle || key == Entry::ftSeries)) {
                if (protectCasing == Qt::Checked)
                    addProtectiveCasing(text);
                else if (protectCasing == Qt::Unchecked)
                    removeProtectiveCasing(text);
            }

            iodevice->putChar(',');
            iodevice->putChar('\n');
            iodevice->putChar('\t');
            iodevice->write(laTeXEncoder.convertToPlainAscii(bf->format(key, keywordCasing)).toLatin1());
            iodevice->putChar(' ');
            iodevice->putChar('=');
            iodevice->putChar(' ');
            iodevice->write(TextEncoder::encode(text, destinationCodec));
        }
        iodevice->putChar('\n');
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    bool writeMacro(QIODevice *iodevice, const Macro &macro) {
        const BibTeXEntries *be = BibTeXEntries::self();

        QString text = p->internalValueToBibTeX(macro.value(), QString(), leUTF8);
        if (protectCasing == Qt::Checked)
            addProtectiveCasing(text);
        else if (protectCasing == Qt::Unchecked)
            removeProtectiveCasing(text);

        iodevice->putChar('@');
        iodevice->write(be->format(QStringLiteral("String"), keywordCasing).toLatin1().data());
        iodevice->putChar('{');
        iodevice->write(TextEncoder::encode(macro.key(), destinationCodec));
        iodevice->putChar(' ');
        iodevice->putChar('=');
        iodevice->putChar(' ');
        iodevice->write(TextEncoder::encode(text, destinationCodec));
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    bool writeComment(QIODevice *iodevice, const Comment &comment) {
        const BibTeXEntries *be = BibTeXEntries::self();

        QString text = comment.text() ;

        if (comment.useCommand() || quoteComment == Preferences::qcCommand) {
            iodevice->putChar('@');
            iodevice->write(be->format(QStringLiteral("Comment"), keywordCasing).toLatin1().data());
            iodevice->putChar('{');
            iodevice->write(TextEncoder::encode(text, destinationCodec));
            iodevice->putChar('}');
            iodevice->putChar('\n');
            iodevice->putChar('\n');
        } else if (quoteComment == Preferences::qcPercentSign) {
            QStringList commentLines = text.split('\n', QString::SkipEmptyParts);
            for (QStringList::Iterator it = commentLines.begin(); it != commentLines.end(); ++it) {
                const QByteArray line = TextEncoder::encode(*it, destinationCodec);
                if (line.length() == 0 || line[0] != QLatin1Char('%')) {
                    /// Guarantee that every line starts with
                    /// a percent sign
                    iodevice->putChar('%');
                }
                iodevice->write(line);
                iodevice->putChar('\n');
            }
            iodevice->putChar('\n');
        } else {
            iodevice->write(TextEncoder::encode(text, destinationCodec));
            iodevice->putChar('\n');
            iodevice->putChar('\n');
        }

        return true;
    }

    bool writePreamble(QIODevice *iodevice, const Preamble &preamble) {
        const BibTeXEntries *be = BibTeXEntries::self();

        iodevice->putChar('@');
        iodevice->write(be->format(QStringLiteral("Preamble"), keywordCasing).toLatin1().data());
        iodevice->putChar('{');
        /// Remember: strings from preamble do not get encoded,
        /// may contain raw LaTeX commands and code
        iodevice->write(TextEncoder::encode(p->internalValueToBibTeX(preamble.value(), QString(), leRaw), destinationCodec));
        iodevice->putChar('}');
        iodevice->putChar('\n');
        iodevice->putChar('\n');

        return true;
    }

    QString addProtectiveCasing(QString &text) {
        if (text.length() < 2 && (text[0] != QLatin1Char('"') || text[text.length() - 1] != QLatin1Char('"')) && (text[0] != QLatin1Char('{') || text[text.length() - 1] != QLatin1Char('}'))) {
            /// Nothing to protect, as this is no text string
            return text;
        }

        bool addBrackets = true;

        if (text[1] == QLatin1Char('{') && text[text.length() - 2] == QLatin1Char('}')) {
            /// If the given text looks like this:  {{...}}
            /// still check that it is not like this: {{..}..{..}}
            addBrackets = false;
            int count = 0;
            for (int i = text.length() - 2; !addBrackets && i > 1; --i) {
                if (text[i] == QLatin1Char('{')) ++count;
                else if (text[i] == QLatin1Char('}')) --count;
                if (count == 0) addBrackets = true;
            }
        }

        if (addBrackets)
            text.insert(1, QStringLiteral("{")).insert(text.length(),  QStringLiteral("}"));

        return text;
    }

    QString removeProtectiveCasing(QString &text) {
        if (text.length() < 2 && (text[0] != QLatin1Char('"') || text[text.length() - 1] != QLatin1Char('"')) && (text[0] != QLatin1Char('{') || text[text.length() - 1] != QLatin1Char('}'))) {
            /// Nothing to protect, as this is no text string
            return text;
        }

        if (text[1] != QLatin1Char('{') || text[text.length() - 2] != QLatin1Char('}'))
            /// Nothing to remove
            return text;

        bool removeBrackets = true;
        int count = 0;
        for (int i = text.length() - 2; removeBrackets && i > 1; --i) {
            if (text[i] == QLatin1Char('{')) ++count;
            else if (text[i] == QLatin1Char('}')) --count;
            if (count == 0) removeBrackets = false;
        }

        if (removeBrackets)
            text.remove(text.length() - 2, 1).remove(1, 1);

        return text;
    }

    QString &protectQuotationMarks(QString &text) {
        int p = -1;
        while ((p = text.indexOf(QLatin1Char('"'), p + 1)) > 0)
            if (p == 0 || text[p - 1] != QLatin1Char('\\')) {
                text.insert(p + 1, QStringLiteral("}")).insert(p, QStringLiteral("{"));
                ++p;
            }
        return text;
    }

    void applyEncoding(QString &encoding) {
        encoding = encoding.isEmpty() ? QStringLiteral("latex") : encoding.toLower();
        destinationCodec = QTextCodec::codecForName(encoding == QStringLiteral("latex") ? "us-ascii" : encoding.toLatin1());
    }

    bool requiresPersonQuoting(const QString &text, bool isLastName) {
        if (isLastName && !text.contains(QChar(' ')))
            /** Last name contains NO spaces, no quoting necessary */
            return false;
        else if (!isLastName && !text.contains(QStringLiteral(" and ")))
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


FileExporterBibTeX::FileExporterBibTeX(QObject *parent)
        : FileExporter(parent), d(new FileExporterBibTeXPrivate(this))
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

bool FileExporterBibTeX::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    Q_UNUSED(errorLog)

    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = true;
    int totalElements = (int) bibtexfile->count();
    int currentPos = 0;

    d->loadState();
    d->loadStateFromFile(bibtexfile);

    if (d->encoding != QStringLiteral("latex")) {
        Comment encodingComment(QStringLiteral("x-kbibtex-encoding=") + d->encoding, true);
        result &= d->writeComment(iodevice, encodingComment);
    }

    /// Memorize which entries are used in a crossref field
    QStringList crossRefIdList;
    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !d->cancelFlag; ++it) {
        QSharedPointer<const Entry> entry = (*it).dynamicCast<const Entry>();
        if (!entry.isNull()) {
            const QString crossRef = PlainTextValue::text(entry->value(Entry::ftCrossRef));
            if (!crossRef.isEmpty())
                crossRefIdList << crossRef;
        }
    }

    bool allPreamblesAndMacrosProcessed = false;
    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !d->cancelFlag; ++it) {
        QSharedPointer<const Element> element = (*it);
        QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();

        if (!entry.isNull()) {
            /// Postpone entries that are crossref'ed
            if (crossRefIdList.contains(entry->id())) continue;

            if (!allPreamblesAndMacrosProcessed) {
                /// Guarantee that all macros and the preamble are written
                /// before the first entry (@article, ...) is written
                for (File::ConstIterator msit = it + 1; msit != bibtexfile->constEnd() && result && !d->cancelFlag; ++msit) {
                    QSharedPointer<const Preamble> preamble = (*msit).dynamicCast<const Preamble>();
                    if (!preamble.isNull()) {
                        result &= d->writePreamble(iodevice, *preamble);
                        emit progress(++currentPos, totalElements);
                    } else {
                        QSharedPointer<const Macro> macro = (*msit).dynamicCast<const Macro>();
                        if (!macro.isNull()) {
                            result &= d->writeMacro(iodevice, *macro);
                            emit progress(++currentPos, totalElements);
                        }
                    }
                }
                allPreamblesAndMacrosProcessed = true;
            }

            result &= d->writeEntry(iodevice, *entry);
            emit progress(++currentPos, totalElements);
        } else {
            QSharedPointer<const Comment> comment = element.dynamicCast<const Comment>();
            if (!comment.isNull() && !comment->text().startsWith(QStringLiteral("x-kbibtex-"))) {
                result &= d->writeComment(iodevice, *comment);
                emit progress(++currentPos, totalElements);
            } else if (!allPreamblesAndMacrosProcessed) {
                QSharedPointer<const Preamble> preamble = element.dynamicCast<const Preamble>();
                if (!preamble.isNull()) {
                    result &= d->writePreamble(iodevice, *preamble);
                    emit progress(++currentPos, totalElements);
                } else {
                    QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
                    if (!macro.isNull()) {
                        result &= d->writeMacro(iodevice, *macro);
                        emit progress(++currentPos, totalElements);
                    }
                }
            }
        }
    }

    /// Crossref'ed entries are written last
    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !d->cancelFlag; ++it) {
        QSharedPointer<const Entry> entry = (*it).dynamicCast<const Entry>();
        if (entry.isNull()) continue;
        if (!crossRefIdList.contains(entry->id())) continue;

        result &= d->writeEntry(iodevice, *entry);
        emit progress(++currentPos, totalElements);
    }

    iodevice->close();
    return result && !d->cancelFlag;
}

bool FileExporterBibTeX::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    Q_UNUSED(errorLog)

    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    d->loadState();
    d->loadStateFromFile(bibtexfile);

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

    iodevice->close();
    return result && !d->cancelFlag;
}

void FileExporterBibTeX::cancel()
{
    d->cancelFlag = true;
}

QString FileExporterBibTeX::valueToBibTeX(const Value &value, const QString &key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (staticFileExporterBibTeX == nullptr) {
        staticFileExporterBibTeX = new FileExporterBibTeX(nullptr);
        staticFileExporterBibTeX->d->loadState();
    }
    return staticFileExporterBibTeX->internalValueToBibTeX(value, key, useLaTeXEncoding);
}

QString FileExporterBibTeX::applyEncoder(const QString &input, UseLaTeXEncoding useLaTeXEncoding) const {
    switch (useLaTeXEncoding) {
    case leLaTeX: return EncoderLaTeX::instance().encode(input, Encoder::TargetEncodingASCII);
    case leUTF8: return EncoderLaTeX::instance().encode(input, Encoder::TargetEncodingUTF8);
    default: return input;
    }
}

QString FileExporterBibTeX::internalValueToBibTeX(const Value &value, const QString &key, UseLaTeXEncoding useLaTeXEncoding)
{
    if (value.isEmpty())
        return QString();

    QString result;
    bool isOpen = false;
    QSharedPointer<const ValueItem> prev;
    for (const auto &valueItem : value) {
        QSharedPointer<const MacroKey> macroKey = valueItem.dynamicCast<const MacroKey>();
        if (!macroKey.isNull()) {
            if (isOpen) result.append(d->stringCloseDelimiter);
            isOpen = false;
            if (!result.isEmpty()) result.append(" # ");
            result.append(macroKey->text());
            prev = macroKey;
        } else {
            QSharedPointer<const PlainText> plainText = valueItem.dynamicCast<const PlainText>();
            if (!plainText.isNull()) {
                QString textBody = applyEncoder(plainText->text(), useLaTeXEncoding);
                if (!isOpen) {
                    if (!result.isEmpty()) result.append(" # ");
                    result.append(d->stringOpenDelimiter);
                } else if (!prev.dynamicCast<const PlainText>().isNull())
                    result.append(' ');
                else if (!prev.dynamicCast<const Person>().isNull()) {
                    /// handle "et al." i.e. "and others"
                    result.append(" and ");
                } else {
                    result.append(d->stringCloseDelimiter).append(" # ").append(d->stringOpenDelimiter);
                }
                isOpen = true;

                if (d->stringOpenDelimiter == QLatin1Char('"'))
                    d->protectQuotationMarks(textBody);
                result.append(textBody);
                prev = plainText;
            } else {
                QSharedPointer<const VerbatimText> verbatimText = valueItem.dynamicCast<const VerbatimText>();
                if (!verbatimText.isNull()) {
                    QString textBody = verbatimText->text();
                    if (!isOpen) {
                        if (!result.isEmpty()) result.append(" # ");
                        result.append(d->stringOpenDelimiter);
                    } else if (!prev.dynamicCast<const VerbatimText>().isNull()) {
                        const QString keyToLower(key.toLower());
                        if (keyToLower.startsWith(Entry::ftUrl) || keyToLower.startsWith(Entry::ftLocalFile) || keyToLower.startsWith(Entry::ftDOI))
                            /// Filenames and alike have be separated by a semicolon,
                            /// as a plain comma may be part of the filename or URL
                            result.append(QStringLiteral("; "));
                        else
                            result.append(' ');
                    } else {
                        result.append(d->stringCloseDelimiter).append(" # ").append(d->stringOpenDelimiter);
                    }
                    isOpen = true;

                    if (d->stringOpenDelimiter == QLatin1Char('"'))
                        d->protectQuotationMarks(textBody);
                    result.append(textBody);
                    prev = verbatimText;
                } else {
                    QSharedPointer<const Person> person = valueItem.dynamicCast<const Person>();
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
                        const QString pnf = suffix.isEmpty() ? d->personNameFormatting : Preferences::personNameFormatLastFirst;
                        QString thisName = applyEncoder(Person::transcribePersonName(pnf, firstName, lastName, suffix), useLaTeXEncoding);

                        if (!isOpen) {
                            if (!result.isEmpty()) result.append(" # ");
                            result.append(d->stringOpenDelimiter);
                        } else if (!prev.dynamicCast<const Person>().isNull())
                            result.append(" and ");
                        else {
                            result.append(d->stringCloseDelimiter).append(" # ").append(d->stringOpenDelimiter);
                        }
                        isOpen = true;

                        if (d->stringOpenDelimiter == QLatin1Char('"'))
                            d->protectQuotationMarks(thisName);
                        result.append(thisName);
                        prev = person;
                    } else {
                        QSharedPointer<const Keyword> keyword = valueItem.dynamicCast<const Keyword>();
                        if (!keyword.isNull()) {
                            QString textBody = applyEncoder(keyword->text(), useLaTeXEncoding);
                            if (!isOpen) {
                                if (!result.isEmpty()) result.append(" # ");
                                result.append(d->stringOpenDelimiter);
                            } else if (!prev.dynamicCast<const Keyword>().isNull())
                                result.append(d->listSeparator);
                            else {
                                result.append(d->stringCloseDelimiter).append(" # ").append(d->stringOpenDelimiter);
                            }
                            isOpen = true;

                            if (d->stringOpenDelimiter == QLatin1Char('"'))
                                d->protectQuotationMarks(textBody);
                            result.append(textBody);
                            prev = keyword;
                        }
                    }
                }
            }
        }
        prev = valueItem;
    }

    if (isOpen) result.append(d->stringCloseDelimiter);

    return result;
}

bool FileExporterBibTeX::isFileExporterBibTeX(const FileExporter &other) {
    return typeid(other) == typeid(FileExporterBibTeX);
}
