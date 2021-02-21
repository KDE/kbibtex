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

#include "fileexporterbibtex.h"

#include <typeinfo>

#include <QTextCodec>
#include <QTextStream>
#include <QStringList>
#include <QBuffer>

#include <BibTeXEntries>
#include <BibTeXFields>
#include <Preferences>
#include <File>
#include <Element>
#include <Entry>
#include <Macro>
#include <Preamble>
#include <Value>
#include <Comment>
#include "encoderlatex.h"
#include "logging_io.h"

#define normalizeText(text) (text).normalized(QString::NormalizationForm_C)

class FileExporterBibTeX::Private
{
private:
    FileExporterBibTeX *parent;

    /**
     * Determine a codec to use based on various settings such as
     * the global preferences, per-file settings, or configuration
     * settings passed to this FileExporterBibTeX instance.
     * @return a valid QTextCodec instance or 'nullptr' if UTF-8 is to be used
     */
    QPair<QString, QTextCodec *> determineTargetCodec() {
        QString encoding = QStringLiteral("utf-8"); ///< default encoding if nothing else is set
        if (!this->encoding.isEmpty())
            /// Encoding as loaded in loadPreferencesAndProperties(..) has low priority
            encoding = this->encoding;
        if (!forcedEncoding.isEmpty())
            /// Encoding as set via setEncoding(..) has high priority
            encoding = forcedEncoding;
        encoding = encoding.toLower();
        if (encoding == QStringLiteral("utf-8") || encoding == QStringLiteral("utf8"))
            return QPair<QString, QTextCodec *>(QStringLiteral("utf-8"), nullptr); ///< a 'nullptr' encoder signifies UTF-8
        else if (encoding == QStringLiteral("latex"))
            return QPair<QString, QTextCodec *>(encoding, nullptr); ///< "LaTeX" encoding is actually just UTF-8
        else
            return QPair<QString, QTextCodec *>(encoding, QTextCodec::codecForName(encoding.toLatin1().constData()));
    }

    inline bool canEncode(const QChar &c, QTextCodec *codec) {
        if (codec == nullptr)
            return true; ///< no codec means 'use UTF-8'; assume that UTF-8 can encode anything

        /// QTextCodec::canEncode has some issues and cannot be relied upon
        QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
        const QByteArray conversionResult = codec->fromUnicode(&c, 1, &state);
        /// Conversion failed if codec gave a single byte back which is 0x00
        /// (due to QTextCodec::ConvertInvalidToNull above)
        return conversionResult.length() != 1 || conversionResult.at(0) != '\0';
    }

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

    Private(FileExporterBibTeX *p)
            : parent(p), cancelFlag(false)
    {
        /// Initialize variables like 'keywordCasing' or 'quoteComment' from Preferences
        loadPreferencesAndProperties(nullptr /** no File object to evaluate properties from */);
    }

    void loadPreferencesAndProperties(const File *bibtexfile) {
#ifdef HAVE_KF5
        encoding = Preferences::instance().bibTeXEncoding();
        QString stringDelimiter = Preferences::instance().bibTeXStringDelimiter();
        if (stringDelimiter.length() != 2)
            stringDelimiter = Preferences::defaultBibTeXStringDelimiter;
#else // HAVE_KF5
        encoding = QStringLiteral("LaTeX");
        const QString stringDelimiter = QStringLiteral("{}");
#endif // HAVE_KF5
        stringOpenDelimiter = stringDelimiter[0];
        stringCloseDelimiter = stringDelimiter[1];
#ifdef HAVE_KF5
        keywordCasing = Preferences::instance().bibTeXKeywordCasing();
        quoteComment = Preferences::instance().bibTeXQuoteComment();
        protectCasing = Preferences::instance().bibTeXProtectCasing() ? Qt::Checked : Qt::Unchecked;
        listSeparator =  Preferences::instance().bibTeXListSeparator();
#else // HAVE_KF5
        keywordCasing = KBibTeX::Casing::LowerCase;
        quoteComment = Preferences::QuoteComment::None;
        protectCasing = Qt::PartiallyChecked;
        listSeparator = QStringLiteral("; ");
#endif // HAVE_KF5
        personNameFormatting = Preferences::instance().personNameFormat();

        /// Check if a valid File object was provided
        if (bibtexfile != nullptr) {
            /// If there is a File object, extract its properties which
            /// overturn the global preferences
            if (bibtexfile->hasProperty(File::Encoding))
                encoding = bibtexfile->property(File::Encoding).toString();
            if (bibtexfile->hasProperty(File::StringDelimiter)) {
                QString stringDelimiter = bibtexfile->property(File::StringDelimiter).toString();
                if (stringDelimiter.length() != 2)
                    stringDelimiter = Preferences::defaultBibTeXStringDelimiter;
                stringOpenDelimiter = stringDelimiter[0];
                stringCloseDelimiter = stringDelimiter[1];
            }
            if (bibtexfile->hasProperty(File::QuoteComment))
                quoteComment = static_cast<Preferences::QuoteComment>(bibtexfile->property(File::QuoteComment).toInt());
            if (bibtexfile->hasProperty(File::KeywordCasing))
                keywordCasing = static_cast<KBibTeX::Casing>(bibtexfile->property(File::KeywordCasing).toInt());
            if (bibtexfile->hasProperty(File::ProtectCasing))
                protectCasing = static_cast<Qt::CheckState>(bibtexfile->property(File::ProtectCasing).toInt());
            if (bibtexfile->hasProperty(File::NameFormatting)) {
                /// if the user set "use global default", this property is an empty string
                /// in this case, keep default value
                const QString buffer = bibtexfile->property(File::NameFormatting).toString();
                personNameFormatting = buffer.isEmpty() ? personNameFormatting : buffer;
            }
            if (bibtexfile->hasProperty(File::ListSeparator))
                listSeparator = bibtexfile->property(File::ListSeparator).toString();
        }
    }

    QString internalValueToBibTeX(const Value &value, const Encoder::TargetEncoding targetEncoding, const QString &key = QString())
    {
        if (value.isEmpty())
            return QString();

        QString result;
        result.reserve(1024);
        bool isOpen = false;
        QSharedPointer<const ValueItem> prev;
        for (const auto &valueItem : value) {
            QSharedPointer<const MacroKey> macroKey = valueItem.dynamicCast<const MacroKey>();
            if (!macroKey.isNull()) {
                if (isOpen) result.append(stringCloseDelimiter);
                isOpen = false;
                if (!result.isEmpty()) result.append(QStringLiteral(" # "));
                result.append(macroKey->text());
                prev = macroKey;
            } else {
                QSharedPointer<const PlainText> plainText = valueItem.dynamicCast<const PlainText>();
                if (!plainText.isNull()) {
                    QString textBody = EncoderLaTeX::instance().encode(plainText->text(), targetEncoding);
                    if (!isOpen) {
                        if (!result.isEmpty()) result.append(" # ");
                        result.append(stringOpenDelimiter);
                    } else if (!prev.dynamicCast<const PlainText>().isNull())
                        result.append(' ');
                    else if (!prev.dynamicCast<const Person>().isNull()) {
                        /// handle "et al." i.e. "and others"
                        result.append(" and ");
                    } else {
                        result.append(stringCloseDelimiter).append(" # ").append(stringOpenDelimiter);
                    }
                    isOpen = true;

                    if (stringOpenDelimiter == QLatin1Char('"'))
                        protectQuotationMarks(textBody);
                    result.append(textBody);
                    prev = plainText;
                } else {
                    QSharedPointer<const VerbatimText> verbatimText = valueItem.dynamicCast<const VerbatimText>();
                    if (!verbatimText.isNull()) {
                        QString textBody = verbatimText->text();
                        if (!isOpen) {
                            if (!result.isEmpty()) result.append(" # ");
                            result.append(stringOpenDelimiter);
                        } else if (!prev.dynamicCast<const VerbatimText>().isNull()) {
                            const QString keyToLower(key.toLower());
                            if (keyToLower.startsWith(Entry::ftUrl) || keyToLower.startsWith(Entry::ftLocalFile) || keyToLower.startsWith(Entry::ftFile) || keyToLower.startsWith(Entry::ftDOI))
                                /// Filenames and alike have be separated by a semicolon,
                                /// as a plain comma may be part of the filename or URL
                                result.append(QStringLiteral("; "));
                            else
                                result.append(' ');
                        } else {
                            result.append(stringCloseDelimiter).append(" # ").append(stringOpenDelimiter);
                        }
                        isOpen = true;

                        if (stringOpenDelimiter == QLatin1Char('"'))
                            protectQuotationMarks(textBody);
                        result.append(textBody);
                        prev = verbatimText;
                    } else {
                        QSharedPointer<const Person> person = valueItem.dynamicCast<const Person>();
                        if (!person.isNull()) {
                            QString firstName = person->firstName();
                            if (!firstName.isEmpty() && requiresPersonQuoting(firstName, false))
                                firstName = firstName.prepend("{").append("}");

                            QString lastName = person->lastName();
                            if (!lastName.isEmpty() && requiresPersonQuoting(lastName, true))
                                lastName = lastName.prepend("{").append("}");

                            QString suffix = person->suffix();

                            /// Fall back and enforce comma-based name formatting
                            /// if name contains a suffix like "Jr."
                            /// Otherwise name could not be parsed again reliable
                            const QString pnf = suffix.isEmpty() ? personNameFormatting : Preferences::personNameFormatLastFirst;
                            QString thisName = EncoderLaTeX::instance().encode(Person::transcribePersonName(pnf, firstName, lastName, suffix), targetEncoding);

                            if (!isOpen) {
                                if (!result.isEmpty()) result.append(" # ");
                                result.append(stringOpenDelimiter);
                            } else if (!prev.dynamicCast<const Person>().isNull())
                                result.append(" and ");
                            else {
                                result.append(stringCloseDelimiter).append(" # ").append(stringOpenDelimiter);
                            }
                            isOpen = true;

                            if (stringOpenDelimiter == QLatin1Char('"'))
                                protectQuotationMarks(thisName);
                            result.append(thisName);
                            prev = person;
                        } else {
                            QSharedPointer<const Keyword> keyword = valueItem.dynamicCast<const Keyword>();
                            if (!keyword.isNull()) {
                                QString textBody = EncoderLaTeX::instance().encode(keyword->text(), targetEncoding);
                                if (!isOpen) {
                                    if (!result.isEmpty()) result.append(" # ");
                                    result.append(stringOpenDelimiter);
                                } else if (!prev.dynamicCast<const Keyword>().isNull())
                                    result.append(listSeparator);
                                else {
                                    result.append(stringCloseDelimiter).append(" # ").append(stringOpenDelimiter);
                                }
                                isOpen = true;

                                if (stringOpenDelimiter == QLatin1Char('"'))
                                    protectQuotationMarks(textBody);
                                result.append(textBody);
                                prev = keyword;
                            }
                        }
                    }
                }
            }
            prev = valueItem;
        }

        if (isOpen) result.append(stringCloseDelimiter);

        result.squeeze();
        return result;
    }

    bool writeEntry(QString &output, const Entry &entry, const Encoder::TargetEncoding &targetEncoding) {
        /// write start of a entry (entry type and id) in plain ASCII
        output.append(QLatin1Char('@')).append(BibTeXEntries::instance().format(entry.type(), keywordCasing));
        output.append(QLatin1Char('{')).append(Encoder::instance().convertToPlainAscii(entry.id()));

        for (Entry::ConstIterator it = entry.constBegin(); it != entry.constEnd(); ++it) {
            const QString key = it.key();
            Value value = it.value();
            if (value.isEmpty()) continue; ///< ignore empty key-value pairs

            QString text = internalValueToBibTeX(value, targetEncoding, key);
            if (text.isEmpty()) {
                /// ignore empty key-value pairs
                qCWarning(LOG_KBIBTEX_IO) << "Value for field " << key << " is empty";
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

            output.append(QStringLiteral(",\n\t"));
            output.append(Encoder::instance().convertToPlainAscii(BibTeXFields::instance().format(key, keywordCasing)));
            output.append(QStringLiteral(" = ")).append(normalizeText(text));
        }
        output.append(QStringLiteral("\n}\n\n"));

        return true;
    }

    bool writeMacro(QString &output, const Macro &macro, const Encoder::TargetEncoding &targetEncoding) {
        QString text = internalValueToBibTeX(macro.value(), targetEncoding);
        if (protectCasing == Qt::Checked)
            addProtectiveCasing(text);
        else if (protectCasing == Qt::Unchecked)
            removeProtectiveCasing(text);

        output.append(QLatin1Char('@')).append(BibTeXEntries::instance().format(QStringLiteral("String"), keywordCasing));
        output.append(QLatin1Char('{')).append(normalizeText(macro.key()));
        output.append(QStringLiteral(" = ")).append(normalizeText(text));
        output.append(QStringLiteral("}\n\n"));

        return true;
    }

    bool writeComment(QString &output, const Comment &comment) {
        QString text = comment.text() ;

        if (comment.useCommand() || quoteComment == Preferences::QuoteComment::Command) {
            output.append(QLatin1Char('@')).append(BibTeXEntries::instance().format(QStringLiteral("Comment"), keywordCasing));
            output.append(QLatin1Char('{')).append(normalizeText(text));
            output.append(QLatin1Char('}')).append(QStringLiteral("\n\n"));
        } else if (quoteComment == Preferences::QuoteComment::PercentSign) {
#if QT_VERSION >= 0x050e00
            QStringList commentLines = text.split('\n', Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
            QStringList commentLines = text.split('\n', QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
            for (QStringList::Iterator it = commentLines.begin(); it != commentLines.end(); ++it) {
                const QString &line = *it;
                if (line.length() == 0 || line[0] != QLatin1Char('%')) {
                    /// Guarantee that every line starts with
                    /// a percent sign
                    output.append(QLatin1Char('%'));
                }
                output.append(normalizeText(text)).append(QStringLiteral("\n"));
            }
            output.append(QLatin1Char('\n'));
        } else {
            output.append(normalizeText(text)).append(QStringLiteral("\n\n"));
        }

        return true;
    }

    bool writePreamble(QString &output, const Preamble &preamble) {
        output.append(QLatin1Char('@')).append(BibTeXEntries::instance().format(QStringLiteral("Preamble"), keywordCasing)).append(QLatin1Char('{'));
        /// Strings from preamble do not get LaTeX-encoded, may contain raw LaTeX commands and code
        output.append(normalizeText(internalValueToBibTeX(preamble.value(), Encoder::TargetEncoding::RAW)));
        output.append(QStringLiteral("}\n\n"));

        return true;
    }

    QString addProtectiveCasing(QString &text) {
        /// Check if either
        ///  - text is too short (less than two characters)  or
        ///  - text neither starts/stops with double quotation marks
        ///    nor starts with { and stops with }
        if (text.length() < 2 || ((text[0] != QLatin1Char('"') || text[text.length() - 1] != QLatin1Char('"')) && (text[0] != QLatin1Char('{') || text[text.length() - 1] != QLatin1Char('}')))) {
            /// Nothing to protect, as this is no text string
            return text;
        }

        bool addBrackets = true;

        if (text[1] == QLatin1Char('{') && text[text.length() - 2] == QLatin1Char('}')) {
            /// If the given text looks like this:  {{...}}  or  "{...}"
            /// still check that it is not like this: {{..}..{..}}
            addBrackets = false;
            for (int i = text.length() - 2, count = 0; !addBrackets && i > 1; --i) {
                if (text[i] == QLatin1Char('{')) ++count;
                else if (text[i] == QLatin1Char('}')) --count;
                if (count == 0) addBrackets = true;
            }
        }

        if (addBrackets)
            text.insert(1, QStringLiteral("{")).insert(text.length() - 1,  QStringLiteral("}"));

        return text;
    }

    QString removeProtectiveCasing(QString &text) {
        /// Check if either
        ///  - text is too short (less than two characters)  or
        ///  - text neither starts/stops with double quotation marks
        ///    nor starts with { and stops with }
        if (text.length() < 2 || ((text[0] != QLatin1Char('"') || text[text.length() - 1] != QLatin1Char('"')) && (text[0] != QLatin1Char('{') || text[text.length() - 1] != QLatin1Char('}')))) {
            /// Nothing to protect, as this is no text string
            return text;
        }

        if (text[1] != QLatin1Char('{') || text[text.length() - 2] != QLatin1Char('}'))
            /// Nothing to remove
            return text;

        /// If the given text looks like this:  {{...}}  or  "{...}"
        /// still check that it is not like this: {{..}..{..}}
        bool removeBrackets = true;
        for (int i = text.length() - 2, count = 0; removeBrackets && i > 1; --i) {
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

    bool saveAsString(QString &output, const File *bibtexfile) {
        const Encoder::TargetEncoding targetEncoding = determineTargetCodec().first == QStringLiteral("latex") || determineTargetCodec().first == QStringLiteral("us-ascii") ? Encoder::TargetEncoding::ASCII : Encoder::TargetEncoding::UTF8;

        /// Memorize which entries are used in a crossref field
        QHash<QString, QStringList> crossRefMap;
        for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && !cancelFlag; ++it) {
            QSharedPointer<const Entry> entry = (*it).dynamicCast<const Entry>();
            if (!entry.isNull()) {
                const QString crossRef = PlainTextValue::text(entry->value(Entry::ftCrossRef));
                if (!crossRef.isEmpty()) {
                    QStringList crossRefList = crossRefMap.value(crossRef, QStringList());
                    crossRefList.append(entry->id());
                    crossRefMap.insert(crossRef, crossRefList);
                }
            }
        }

        int currentPos = 0, totalElements = bibtexfile->count();
        bool result = true;
        bool allPreamblesAndMacrosProcessed = false;
        QSet<QString> processedEntryIds;
        for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !cancelFlag; ++it) {
            QSharedPointer<const Element> element = (*it);
            QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();

            if (!entry.isNull()) {
                processedEntryIds.insert(entry->id());

                /// Postpone entries that are crossref'ed
                const QStringList crossRefList = crossRefMap.value(entry->id(), QStringList());
                if (!crossRefList.isEmpty()) {
                    bool allProcessed = true;
                    for (const QString &origin : crossRefList)
                        allProcessed &= processedEntryIds.contains(origin);
                    if (allProcessed)
                        crossRefMap.remove(entry->id());
                    else
                        continue;
                }

                if (!allPreamblesAndMacrosProcessed) {
                    /// Guarantee that all macros and the preamble are written
                    /// before the first entry (@article, ...) is written
                    for (File::ConstIterator msit = it + 1; msit != bibtexfile->constEnd() && result && !cancelFlag; ++msit) {
                        QSharedPointer<const Preamble> preamble = (*msit).dynamicCast<const Preamble>();
                        if (!preamble.isNull()) {
                            result &= writePreamble(output, *preamble);
                            /// Instead of an 'emit' ...
                            QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
                        } else {
                            QSharedPointer<const Macro> macro = (*msit).dynamicCast<const Macro>();
                            if (!macro.isNull()) {
                                result &= writeMacro(output, *macro, targetEncoding);
                                /// Instead of an 'emit' ...
                                QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
                            }
                        }
                    }
                    allPreamblesAndMacrosProcessed = true;
                }

                result &= writeEntry(output, *entry, targetEncoding);
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
            } else {
                QSharedPointer<const Comment> comment = element.dynamicCast<const Comment>();
                if (!comment.isNull() && !comment->text().startsWith(QStringLiteral("x-kbibtex-"))) {
                    result &= writeComment(output, *comment);
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
                } else if (!allPreamblesAndMacrosProcessed) {
                    QSharedPointer<const Preamble> preamble = element.dynamicCast<const Preamble>();
                    if (!preamble.isNull()) {
                        result &= writePreamble(output, *preamble);
                        /// Instead of an 'emit' ...
                        QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
                    } else {
                        QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
                        if (!macro.isNull()) {
                            result &= writeMacro(output, *macro, targetEncoding);
                            /// Instead of an 'emit' ...
                            QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
                        }
                    }
                }
            }
        }

        /// Crossref'ed entries are written last
        if (!crossRefMap.isEmpty())
            for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !cancelFlag; ++it) {
                QSharedPointer<const Entry> entry = (*it).dynamicCast<const Entry>();
                if (entry.isNull()) continue;
                if (!crossRefMap.contains(entry->id())) continue;

                result &= writeEntry(output, *entry, targetEncoding);
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "progress", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(int, ++currentPos), Q_ARG(int, totalElements));
            }

        return result;
    }

    bool saveAsString(QString &output, const QSharedPointer<const Element> element) {
        const Encoder::TargetEncoding targetEncoding = determineTargetCodec().first == QStringLiteral("latex") ? Encoder::TargetEncoding::ASCII : Encoder::TargetEncoding::UTF8;

        const QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
        if (!entry.isNull())
            return writeEntry(output, *entry, targetEncoding);
        else {
            const QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
            if (!macro.isNull())
                return writeMacro(output, *macro, targetEncoding);
            else {
                const QSharedPointer<const Comment> comment = element.dynamicCast<const Comment>();
                if (!comment.isNull())
                    return writeComment(output, *comment);
                else {
                    const QSharedPointer<const Preamble> preamble = element.dynamicCast<const Preamble>();
                    if (!preamble.isNull())
                        return writePreamble(output, *preamble);
                    else
                        qCWarning(LOG_KBIBTEX_IO) << "Trying to save unsupported Element to BibTeX";
                }
            }
        }

        return false;
    }

    QByteArray applyEncoding(const QString &input) {
        QTextCodec *codec = determineTargetCodec().second;

        QString rewrittenInput;
        rewrittenInput.reserve(input.length() * 12 / 10 /* add 20% */ + 1024 /* plus 1K */);
        const Encoder &laTeXEncoder = EncoderLaTeX::instance();
        for (const QChar &c : input) {
            if (codec == nullptr /** meaning UTF-8, which can encode anything */ || canEncode(c, codec))
                rewrittenInput.append(c);
            else
                rewrittenInput.append(laTeXEncoder.encode(QString(c), Encoder::TargetEncoding::ASCII));
        }

        if (codec == nullptr || (codec->name().toLower() != "utf-16" && codec->name().toLower() != "utf-32")) {
            // Unless encoding is UTF-16 or UTF-32 (those have BOM to detect encoding) ...

            // Determine which (if at all) encoding comment to be included in BibTeX data
            QString encodingForComment; //< empty by default
            if (!forcedEncoding.isEmpty())
                // For this exporter instance, a specific encoding was forced upon
                encodingForComment = forcedEncoding;
            else if (!encoding.isEmpty())
                // File had an encoding in its properties
                // (variable 'encoding' was set in 'loadPreferencesAndProperties')
                encodingForComment = encoding;

            if (!encodingForComment.isEmpty()) {
                // Verify that 'encodingForComment', which labels an encoding,
                // is compatible with the target codec
#define normalizeCodecName(codecname) codecname.toLower().remove(QLatin1Char(' ')).remove(QLatin1Char('-')).remove(QLatin1Char('_')).replace(QStringLiteral("euckr"),QStringLiteral("windows949"))
                const QString lowerNormalizedEncodingForComment = normalizeCodecName(encodingForComment);
                const QString lowerNormalizedCodecName = codec != nullptr ? normalizeCodecName(QString::fromLatin1(codec->name())) : QString();
                if (codec == nullptr) {
                    if (lowerNormalizedEncodingForComment != QStringLiteral("utf8") && lowerNormalizedEncodingForComment != QStringLiteral("latex")) {
                        qCWarning(LOG_KBIBTEX_IO) << "No codec (means UTF-8 encoded output) does not match with encoding" << encodingForComment;
                        return QByteArray();
                    }
                } else if (lowerNormalizedCodecName != lowerNormalizedEncodingForComment) {
                    qCWarning(LOG_KBIBTEX_IO) << "Codec with name" << codec->name() << "does not match with encoding" << encodingForComment;
                    return QByteArray();
                }
            }

            if (!encodingForComment.isEmpty() && encodingForComment.toLower() != QStringLiteral("latex") && encodingForComment.toLower() != QStringLiteral("us-ascii"))
                // Only if encoding is not pure ASCII (i.e. 'LaTeX' or 'US-ASCII') add
                // a comment at the beginning of the file to tell which encoding was used
                rewrittenInput.prepend(QString(QStringLiteral("@comment{x-kbibtex-encoding=%1}\n\n")).arg(encodingForComment));
        } else {
            // For UTF-16 and UTF-32, no special comment needs to be added:
            // Those encodings are recognized by their BOM or the regular
            // occurrence of 0x00 bytes which is typically if encoding
            // ASCII text.
        }

        rewrittenInput.squeeze();

        return codec == nullptr ? rewrittenInput.toUtf8() : codec->fromUnicode(rewrittenInput);
    }

    bool writeOutString(const QString &outputString, QIODevice *iodevice) {
        bool result = outputString.length() > 0;

        if (result) {
            const QByteArray outputData = applyEncoding(outputString);
            result &= outputData.length() > 0;
            if (!result)
                qCWarning(LOG_KBIBTEX_IO) << "outputData.length() is" << outputData.length();
            if (result)
                result &= iodevice->write(outputData) == outputData.length();
            if (!result)
                qCWarning(LOG_KBIBTEX_IO) << "Writing data to IO device failed, not everything was written";
        } else
            qCWarning(LOG_KBIBTEX_IO) << "outputString.length() is" << outputString.length();

        return result;
    }
};


FileExporterBibTeX::FileExporterBibTeX(QObject *parent)
        : FileExporter(parent), d(new Private(this))
{
    /// nothing
}

FileExporterBibTeX::~FileExporterBibTeX()
{
    delete d;
}

void FileExporterBibTeX::setEncoding(const QString &encoding)
{
    d->forcedEncoding = encoding;
}

QString FileExporterBibTeX::toString(const QSharedPointer<const Element> element, const File *bibtexfile)
{
    d->cancelFlag = false;

    d->loadPreferencesAndProperties(bibtexfile);

    QString outputString;
    outputString.reserve(1024);
    bool result = d->saveAsString(outputString, element);
    if (!result) {
        qCWarning(LOG_KBIBTEX_IO) << "saveInString(..) failed";
        return QString();
    }

    outputString.squeeze();
    return outputString.normalized(QString::NormalizationForm_C);
}

QString FileExporterBibTeX::toString(const File *bibtexfile)
{
    d->cancelFlag = false;

    if (bibtexfile == nullptr) {
        qCWarning(LOG_KBIBTEX_IO) << "No bibliography to write given";
        return QString();
    } else if (bibtexfile->isEmpty()) {
        qCDebug(LOG_KBIBTEX_IO) << "Bibliography is empty";
        return QString();
    }

    d->loadPreferencesAndProperties(bibtexfile);

    QString outputString;
    outputString.reserve(bibtexfile->length() * 1024); //< reserve 1K per element
    bool result = d->saveAsString(outputString, bibtexfile);
    if (!result) {
        qCWarning(LOG_KBIBTEX_IO) << "saveInString(..) failed";
        return QString();
    }

    outputString.squeeze();
    return outputString.normalized(QString::NormalizationForm_C);
}


bool FileExporterBibTeX::save(QIODevice *iodevice, const File *bibtexfile)
{
    d->cancelFlag = false;

    if (iodevice == nullptr || !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    } else if (bibtexfile == nullptr) {
        qCWarning(LOG_KBIBTEX_IO) << "No bibliography to write given";
        return false;
    } else if (bibtexfile->isEmpty()) {
        qCDebug(LOG_KBIBTEX_IO) << "Bibliography is empty";
        iodevice->close();
        return true;
    }

    // Call 'toString' to get an in-memory representation of the BibTeX data,
    // then rewrite the output either protect only sensitive text (e.g. '&')
    // or rewrite all known non-ASCII characters to their LaTeX equivalents
    // (e.g. U+00E4 to '{\"a}')
    const bool result = d->writeOutString(toString(bibtexfile), iodevice);

    return result && !d->cancelFlag;
}

bool FileExporterBibTeX::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile)
{
    d->cancelFlag = false;

    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    const bool result = d->writeOutString(toString(element, bibtexfile), iodevice);

    iodevice->close();
    return result && !d->cancelFlag;
}

void FileExporterBibTeX::cancel()
{
    d->cancelFlag = true;
}

QString FileExporterBibTeX::valueToBibTeX(const Value &value, Encoder::TargetEncoding targetEncoding, const QString &key)
{
    FileExporterBibTeX staticFileExporterBibTeX(nullptr);
    staticFileExporterBibTeX.d->cancelFlag = false;
    return staticFileExporterBibTeX.d->internalValueToBibTeX(value, targetEncoding, key);
}

QString FileExporterBibTeX::editionNumberToString(const int edition, const Preferences::BibliographySystem bibliographySystem)
{
    if (edition <= 0) {
        qCWarning(LOG_KBIBTEX_IO) << "Cannot convert a non-positive number (" << edition << ") into a textual representation";
        return QString();
    }

    // According to http://mirrors.ctan.org/biblio/bibtex/contrib/doc/btxFAQ.pdf,
    // edition values should look like this:
    //  - for first to fifth, write "First" to "Fifth"
    //  - starting from sixth, use numeric form like "17th"
    // According to http://mirrors.ctan.org/macros/latex/contrib/biblatex/doc/biblatex.pdf,
    // edition values should by just numbers (digits) without text,
    // such as '1' in a @sa PlainText.

    if (bibliographySystem == Preferences::BibliographySystem::BibLaTeX)
        return QString::number(edition);
    else if (bibliographySystem == Preferences::BibliographySystem::BibTeX)
        // BibTeX uses ordinals
        return numberToOrdinal(edition);
    else
        return QString();
}

bool FileExporterBibTeX::isFileExporterBibTeX(const FileExporter &other) {
    return typeid(other) == typeid(FileExporterBibTeX);
}
