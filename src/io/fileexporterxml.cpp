/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileexporterxml.h"

#include <QIODevice>
#include <QRegularExpression>
#include <QStringList>

#include <KBibTeX>
#include <File>
#include <Entry>
#include <Macro>
#include <Comment>
#include "fileimporterbibtex.h"
#include "fileexporter.h"
#include "encoderxml.h"
#include "logging_io.h"

#if QT_VERSION >= 0x050e00
#define ENDL Qt::endl
#else // QT_VERSION < 0x050e00
#define ENDL endl
#endif // QT_VERSION >= 0x050e00


QString htmlify(const QString &input)
{
    QString output;
    output.reserve(input.length() * 4 / 3 + 128);
    QChar prev_c;
    for (const QChar &c : input) {
        if (c.unicode() < 128) {
            static const QSet<QChar> skipChar{QLatin1Char('{'), QLatin1Char('}')};
            if (!skipChar.contains(c) || prev_c == QLatin1Char('\\'))
                output.append(c);
        } else
            output.append(QString(QStringLiteral("&#x%1;")).arg(c.unicode(), 4, 16, QLatin1Char('0')));
        prev_c = c;
    }
    return output;
}

QString cleanXML(const QString &text)
{
    static const QRegularExpression removal(QStringLiteral("[{}]+"));
    static const QRegularExpression lineBreaksRegExp(QStringLiteral("[ \\t]*[\\n\\r]"));
    QString result = text;
    result = result.replace(lineBreaksRegExp, QStringLiteral("<br/>")).remove(removal).remove(QStringLiteral("\\ensuremath"));
    return result;
}

QString valueItemToXML(const QSharedPointer<const ValueItem> &valueItem)
{
    QSharedPointer<const PlainText> plainText = valueItem.dynamicCast<const PlainText>();
    if (!plainText.isNull())
        return QStringLiteral("<text>") + cleanXML(EncoderXML::instance().encode(PlainTextValue::text(valueItem), Encoder::TargetEncoding::UTF8)) + QStringLiteral("</text>");
    else {
        QSharedPointer<const Person> p = valueItem.dynamicCast<const Person>();
        if (!p.isNull()) {
            QString result(QStringLiteral("<person>"));
            if (!p->firstName().isEmpty())
                result.append(QStringLiteral("<firstname>") + cleanXML(EncoderXML::instance().encode(p->firstName(), Encoder::TargetEncoding::UTF8)) + QStringLiteral("</firstname>"));
            if (!p->lastName().isEmpty())
                result.append(QStringLiteral("<lastname>") + cleanXML(EncoderXML::instance().encode(p->lastName(), Encoder::TargetEncoding::UTF8)) + QStringLiteral("</lastname>"));
            if (!p->suffix().isEmpty())
                result.append(QStringLiteral("<suffix>") + cleanXML(EncoderXML::instance().encode(p->suffix(), Encoder::TargetEncoding::UTF8)) + QStringLiteral("</suffix>"));
            result.append(QStringLiteral("</person>"));
            return result;
        }
        // TODO: Other data types
        else
            return QStringLiteral("<text>") + cleanXML(EncoderXML::instance().encode(PlainTextValue::text(valueItem), Encoder::TargetEncoding::UTF8)) + QStringLiteral("</text>");
    }
}

QString valueToXML(const Value &value)
{
    QString result;

    for (const auto &valueItem : value)
        result.append(valueItemToXML(valueItem));

    return result;
}

struct RewriteFunctions {
    std::function<bool(QTextStream &)> header, footer;
    std::function<bool(QTextStream &, const QSharedPointer<const Entry> &)> writeEntry;
    std::function<bool(QTextStream &, const QSharedPointer<const Macro> &)> writeMacro;
    std::function<bool(QTextStream &, const QSharedPointer<const Comment> &)> writeComment;
};

QHash<FileExporterXML::OutputStyle, struct RewriteFunctions> rewriteFunctions =
{
    {
        FileExporterXML::OutputStyle::XML_KBibTeX, {
            /*.header =*/ [](QTextStream &stream) {
                stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << ENDL;
                stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << ENDL;
                stream << "<!-- https://userbase.kde.org/KBibTeX -->" << ENDL;
                stream << "<bibliography>" << ENDL;
                return true;
            },
            /*.footer =*/ [](QTextStream &stream) {
                stream << "</bibliography>" << ENDL;
                return true;
            },
            /*.writeEntry =*/ [](QTextStream &stream, const QSharedPointer<const Entry> &entry) {
                stream << " <entry id=\"" << EncoderXML::instance().encode(entry->id(), Encoder::TargetEncoding::UTF8) << "\" type=\"" << entry->type().toLower() << "\">" << ENDL;
                for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
                    const QString key = it.key().toLower();
                    const Value value = it.value();

                    if (key == Entry::ftAuthor || key == Entry::ftEditor) {
                        Value internal = value;
                        Value::ConstIterator lastIt = internal.constEnd();
                        --lastIt;
                        const QSharedPointer<const ValueItem> &last = *lastIt;
                        stream << "  <" << key << "s";

                        if (!value.isEmpty() && PlainText::isPlainText(*last)) {
                            const QSharedPointer<const PlainText> pt = internal.last().staticCast<const PlainText>();
                            if (pt->text() == QStringLiteral("others")) {
                                internal.erase(internal.end() - 1);
                                stream << " etal=\"true\"";
                            }
                        }
                        stream << ">" << ENDL;
                        stream << valueToXML(internal) << ENDL;
                        stream << "  </" << key << "s>" << ENDL;
                    } else if (key == Entry::ftAbstract) {
                        static const QRegularExpression abstractRegExp(QStringLiteral("\\bAbstract[:]?([ ]|&nbsp;|&amp;nbsp;)*"), QRegularExpression::CaseInsensitiveOption);
                        /// clean up HTML artifacts
                        QString text = valueToXML(value);
                        text = text.remove(abstractRegExp);
                        stream << "  <" << key << ">" << text << "</" << key << ">" << ENDL;
                    } else if (key == Entry::ftPages) {
                        // Guess a ints representing first and last page
                        const QString textualRepresentation = PlainTextValue::text(value);
                        static const QRegularExpression fromPageRegExp(QStringLiteral("^\\s*([1-9]\\d*)\\b"));
                        static const QRegularExpression toPageRegExp(QStringLiteral("\\b([1-9]\\d*)\\s*$"));
                        const QRegularExpressionMatch fromPageMatch = fromPageRegExp.match(textualRepresentation);
                        const QRegularExpressionMatch toPageMatch = toPageRegExp.match(textualRepresentation);
                        bool okFromPage = false, okToPage = false;
                        const int fromPage = fromPageMatch.hasMatch() ? fromPageMatch.captured(1).toInt(&okFromPage) : -1;
                        const int toPage = toPageMatch.hasMatch() ? toPageMatch.captured(1).toInt(&okToPage) : -1;

                        stream << "  <pages";
                        if (okFromPage && fromPage > 0)
                            stream << " firstpage=\"" << fromPage << "\"";
                        if (okToPage && toPage > 0)
                            stream << " lastpage=\"" << toPage << "\"";
                        stream << '>' << valueToXML(value) << "</pages>" << ENDL;
                    } else if (key == Entry::ftEdition) {
                        const QString textualRepresentation = PlainTextValue::text(value);
                        bool ok = false;
                        const int asInt = FileExporter::editionStringToNumber(textualRepresentation, &ok);
                        const QString asText = ok && asInt > 0 ? QStringLiteral("<text>") + FileExporter::numberToOrdinal(asInt) + QStringLiteral("</text>") : valueToXML(value);

                        stream << "  <edition";
                        if (ok && asInt > 0)
                            stream << " number=\"" << asInt << "\"";
                        stream << '>' << asText << "</edition>" << ENDL;
                    } else if (key == Entry::ftYear) {
                        // Guess an int representing the year
                        const QString textualRepresentation = value.count() > 0 ? PlainTextValue::text(value.first()) : QString();
                        static const QRegularExpression yearRegExp(QStringLiteral("^(1[2-9]|2[01])\\d{2}$"));
                        bool ok = false;
                        const int asInt = yearRegExp.match(textualRepresentation).hasMatch() ? textualRepresentation.toInt(&ok) : -1;

                        stream << "  <year";
                        if (ok && asInt > 0)
                            stream << " number=\"" << asInt << "\"";
                        stream << '>' << valueToXML(value) << "</year>" << ENDL;
                    } else if (key == Entry::ftMonth) {
                        int asInt = -1;
                        QString content;
                        for (const auto &valueItem : value) {
                            bool ok = false;
                            if (asInt < 0)
                                asInt = FileExporter::monthStringToNumber(PlainTextValue::text(valueItem), &ok);
                            if (ok)
                                content.append(QStringLiteral("<text>") + KBibTeX::Months[asInt - 1] + QStringLiteral("</text>"));
                            else {
                                content.append(valueItemToXML(valueItem));
                                asInt = -1;
                            }
                        }

                        stream << "  <month";
                        if (asInt >= 1 && asInt <= 12)
                            stream << " triple=\"" << KBibTeX::MonthsTriple[asInt - 1] << "\" number=\"" << asInt << "\"";
                        stream << '>' << content << "</month>" << ENDL;
                    } else {
                        // Guess an int representing of this value
                        const QString textualRepresentation = value.count() > 0 ? PlainTextValue::text(value.first()) : QString();
                        static const QRegularExpression numberRegExp(QStringLiteral("^[1-9]\\d*$"));
                        bool ok = false;
                        const int asInt = numberRegExp.match(textualRepresentation).hasMatch() ? textualRepresentation.toInt(&ok) : -1;

                        stream << "  <" << key;
                        if (ok && asInt > 0)
                            stream << " number=\"" << asInt << "\"";
                        stream << '>' << valueToXML(value) << "</" << key << ">" << ENDL;
                    }
                }
                stream << " </entry>" << ENDL;

                return true;
            },
            /*.writeMacro =*/ [](QTextStream &stream, const QSharedPointer<const Macro> &macro) {
                stream << " <string key=\"" << macro->key() << "\">";
                stream << valueToXML(macro->value());
                stream << "</string>" << ENDL;
                return true;
            },
            /*.writeComment =*/ [](QTextStream &stream, const QSharedPointer<const Comment> &comment) {
                stream << " <comment>" ;
                stream << EncoderXML::instance().encode(comment->text(), Encoder::TargetEncoding::UTF8);
                stream << "</comment>" << ENDL;
                return true;
            }
        }
    }, {
        FileExporterXML::OutputStyle::HTML_Standard, {
            /*.header =*/ [](QTextStream &stream) {
                stream << "<!DOCTYPE html>" << ENDL;
                stream << "<!-- HTML document written by KBibTeXIO as part of KBibTeX -->" << ENDL;
                stream << "<!-- https://userbase.kde.org/KBibTeX -->" << ENDL;
                stream << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << ENDL;
                stream << "<head><title>Bibliography</title><meta charset=\"utf-8\"></head>" << ENDL;
                stream << "<body>" << ENDL;
                return true;
            },
            /*.footer =*/ [](QTextStream &stream) {
                stream << "</body></html>" << ENDL << ENDL;
                return true;
            },
            /*.writeEntry =*/ [](QTextStream &stream, const QSharedPointer<const Entry> &entry) {
                static const auto formatPersons = [](QTextStream &stream, const Value & value) {
                    bool firstPerson = true;
                    for (const QSharedPointer<ValueItem> &vi : value) {
                        const QSharedPointer<Person> &p = vi.dynamicCast<Person>();
                        if (!p.isNull()) {
                            if (!firstPerson)
                                stream << ", ";
                            else
                                firstPerson = false;
                            stream << htmlify(p->lastName()).toUtf8().constData();
                            if (!p->firstName().isEmpty())
                                stream << "<span style=\"opacity:0.75;\">, " << htmlify(p->firstName()).toUtf8().constData() << "</span>";
                        } else {
                            const QString text = PlainTextValue::text(vi);
                            if (text == QStringLiteral("others"))
                                stream << "<span style=\"opacity:0.75;\"><i>et&#160;al.</i></span>";
                        }
                    }
                };

                stream << "<p>";
                const Value &authors = entry->contains(Entry::ftAuthor) ? entry->value(Entry::ftAuthor) : Value();
                formatPersons(stream, authors);
                if (!authors.isEmpty())
                    stream << ": ";

                const QString title = PlainTextValue::text(entry->contains(Entry::ftTitle) ? entry->value(Entry::ftTitle) : Value());
                if (!title.isEmpty())
                    stream << "<strong>" << htmlify(title).toUtf8().constData() << "</strong>";

                const QString booktitle = PlainTextValue::text(entry->contains(Entry::ftBookTitle) ? entry->value(Entry::ftBookTitle) : Value());
                if (!booktitle.isEmpty())
                    stream << ", <i>" << htmlify(booktitle).toUtf8().constData() << "</i>";
                else {
                    const QString journal = PlainTextValue::text(entry->contains(Entry::ftJournal) ? entry->value(Entry::ftJournal) : Value());
                    if (!journal.isEmpty()) {
                        stream << ", <i>" << htmlify(journal).toUtf8().constData() << "</i>";
                        const QString volume = PlainTextValue::text(entry->contains(Entry::ftVolume) ? entry->value(Entry::ftVolume) : Value());
                        if (!volume.isEmpty()) {
                            stream << " " << volume;
                            const QString number = PlainTextValue::text(entry->contains(Entry::ftNumber) ? entry->value(Entry::ftNumber) : Value());
                            if (!number.isEmpty()) {
                                stream << "(" << number << ")";
                            }
                        }
                    }
                }
                const QString series = PlainTextValue::text(entry->contains(Entry::ftSeries) ? entry->value(Entry::ftSeries) : Value());
                if (!series.isEmpty())
                    stream << ", <i>" << htmlify(series).toUtf8().constData() << "</i>";

                const Value &editors = entry->contains(Entry::ftEditor) ? entry->value(Entry::ftEditor) : Value();
                if (!editors.isEmpty()) {
                    stream << (editors.length() > 1 ? ", Eds. " : ", Ed. ");
                    formatPersons(stream, editors);
                }

                const QString school = PlainTextValue::text(entry->contains(Entry::ftSchool) ? entry->value(Entry::ftSchool) : Value());
                if (!school.isEmpty())
                    stream << ", " << htmlify(school).toUtf8().constData();

                QString edition = PlainTextValue::text(entry->contains(Entry::ftEdition) ? entry->value(Entry::ftEdition) : Value());
                if (!edition.isEmpty()) {
                    bool ok = false;
                    int edInt = FileExporter::editionStringToNumber(edition, &ok);
                    if (ok && edInt > 0) {
                        edition = FileExporter::numberToOrdinal(edInt);
                    }
                    stream << ", " << htmlify(edition).toUtf8().constData() << " ed.";
                }

                const QString publisher = PlainTextValue::text(entry->contains(Entry::ftPublisher) ? entry->value(Entry::ftPublisher) : Value());
                if (!publisher.isEmpty())
                    stream << ", " << htmlify(publisher).toUtf8().constData();

                const QString pages = PlainTextValue::text(entry->contains(Entry::ftPages) ? entry->value(Entry::ftPages) : Value());
                if (!pages.isEmpty())
                    stream << ", p. " << htmlify(pages).toUtf8().constData();

                const QString year = PlainTextValue::text(entry->contains(Entry::ftYear) ? entry->value(Entry::ftYear) : Value());
                const QString month = PlainTextValue::text(entry->contains(Entry::ftMonth) ? entry->value(Entry::ftMonth) : Value());
                if (!year.isEmpty()) {
                    stream << ", ";
                    if (!month.isEmpty()) {
                        bool ok = false;
                        int iMonth = FileExporter::monthStringToNumber(month, &ok);
                        if (ok && iMonth >= 1 && iMonth <= 12)
                            stream << KBibTeX::Months[iMonth - 1].toUtf8().constData() << " ";
                        else
                            stream << month.toUtf8().constData() << " ";
                    }
                    stream << year.toUtf8().constData();
                }

                const QString abstract = PlainTextValue::text(entry->contains(Entry::ftAbstract) ? entry->value(Entry::ftAbstract) : Value());
                if (!abstract.isEmpty()) {
                    stream << "<br/><small><i>Abstract</i>: " << htmlify(abstract).toUtf8().constData() << "</small>";
                }

                stream << "</p>" << ENDL;
                return true;
            },
            /*.writeMacro =*/ [](QTextStream &stream, const QSharedPointer<const Macro> &macro) {
                stream << "<p>" << htmlify(PlainTextValue::text(macro->value())).toUtf8().constData() << "</p>" << ENDL;
                return true;
            },
            /*.writeComment =*/ [](QTextStream &stream, const QSharedPointer<const Comment> &comment) {
                stream << "<p>" << htmlify(comment->text()).toUtf8().constData() << "</p>" << ENDL;
                return true;
            }
        }
    }, {
        FileExporterXML::OutputStyle::HTML_AbstractOnly, {
            /*.header =*/ [](QTextStream &stream) {
                stream << "<!DOCTYPE html>" << ENDL;
                stream << "<!-- HTML document written by KBibTeXIO as part of KBibTeX -->" << ENDL;
                stream << "<!-- https://userbase.kde.org/KBibTeX -->" << ENDL;
                stream << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << ENDL;
                stream << "<head><title>Bibliography</title><meta charset=\"utf-8\"></head>" << ENDL;
                stream << "<body>" << ENDL;
                return true;
            },
            /*.footer =*/ [](QTextStream &stream) {
                stream << "</body></html>" << ENDL << ENDL;
                return true;
            },
            /*.writeEntry =*/ [](QTextStream &stream, const QSharedPointer<const Entry> &entry) {
                const QString abstract = PlainTextValue::text(entry->contains(Entry::ftAbstract) ? entry->value(Entry::ftAbstract) : Value());
                if (!abstract.isEmpty()) {
                    stream << "<p>" << htmlify(abstract).toUtf8().constData() << "</p>" << ENDL;
                    return true;
                } else
                    return false;
            },
            /*.writeMacro =*/ [](QTextStream &stream, const QSharedPointer<const Macro> &macro) {
                stream << "<p>" << htmlify(PlainTextValue::text(macro->value())).toUtf8().constData() << "</p>" << ENDL;
                return true;
            },
            /*.writeComment =*/ [](QTextStream &stream, const QSharedPointer<const Comment> &comment) {
                stream << "<p>" << htmlify(comment->text()).toUtf8().constData() << "</p>" << ENDL;
                return true;
            }
        }
    }, {
        FileExporterXML::OutputStyle::Plain_WikipediaCite, {
            /*.header =*/ [](QTextStream &stream) {
                Q_UNUSED(stream);
                // No header for Wikipedia Cite output
                return true;
            },
            /*.footer =*/ [](QTextStream &stream) {
                Q_UNUSED(stream);
                // No footer for Wikipedia Cite output
                return true;
            },
            /*.writeEntry =*/ [](QTextStream &stream, const QSharedPointer<const Entry> &entry) {
                stream << "{{Citation";

                QSet<QString> insertedKeys;
                const QVector<QPair<QString, QString>> values{
                    {QStringLiteral("title"), Entry::ftTitle},
                    {QStringLiteral("year"), Entry::ftYear},
                    {QStringLiteral("journal"), Entry::ftJournal},
                    {QStringLiteral("publisher"), Entry::ftPublisher},
                    {QStringLiteral("volume"), Entry::ftVolume},
                    {QStringLiteral("issue"), Entry::ftNumber},
                    {QStringLiteral("url"), Entry::ftUrl},
                    {QStringLiteral("doi"), Entry::ftDOI},
                    {QStringLiteral("isbn"), Entry::ftISBN},
                    {QStringLiteral("issn"), Entry::ftISSN},
                    {QStringLiteral("pages"), Entry::ftPages},
                    {QStringLiteral("jstor"), QStringLiteral("jstor_id")},
                    {QStringLiteral("pmid"), entry->contains(QStringLiteral("pmid")) ? QStringLiteral("pmid") : (entry->contains(QStringLiteral("pubmed")) ? QStringLiteral("pubmed") : QString())}
                };
                for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
                    const QString value{entry->contains(it->second) ? PlainTextValue::text(entry->value(it->second)) : QString()};
                    if (!value.isEmpty() && !insertedKeys.contains(it->first)) {
                        stream << ENDL << "| " << it->first << " = " << value;
                        insertedKeys.insert(it->first);
                    }
                }

                static const QHash<QRegularExpression, QSet<QString>> substrings{
                    {QRegularExpression(QStringLiteral("jstor[:_-]*(?<jstor>.+)")), {QStringLiteral("^id")}},
                    {QRegularExpression(QStringLiteral("https?://hdl\\.handle\\.net/(?<hdl>.+)")), {Entry::ftUrl}},
                    {QRegularExpression(QStringLiteral("/pubmed/(?<pmid>[^/]+)")), {Entry::ftUrl}},
                    {KBibTeX::doiRegExp, {Entry::ftDOI}},
                    {KBibTeX::arXivRegExp, {QStringLiteral("eprint"), QStringLiteral("arxiv")}}
                };
                for (auto it = substrings.constBegin(); it != substrings.constEnd(); ++it) {
                    const QRegularExpression &re = it.key();
                    const QSet<QString> &bibKeys = it.value();
                    for (const QString &bibKey : bibKeys) {
                        const QString bibText = bibKey == QStringLiteral("^id") ? entry->id() : (entry->contains(bibKey) ? PlainTextValue::text(entry->value(bibKey)) : QString());
                        if (bibText.isEmpty())
                            continue;
                        const QRegularExpressionMatch match = re.match(bibText);
                        if (match.hasMatch()) {
                            for (const QString &namedGroup : re.namedCaptureGroups()) {
                                if (namedGroup.isEmpty() || insertedKeys.contains(namedGroup))
                                    continue;
                                const QString foundText = match.captured(namedGroup);
                                if (!foundText.isEmpty()) {
                                    stream << ENDL << "| " << namedGroup << " = " << foundText;
                                    insertedKeys.insert(namedGroup);
                                }
                            }
                        }
                    }
                }

                int authorCounter = 0;
                const Value &authors = entry->contains(Entry::ftAuthor) ? entry->value(Entry::ftAuthor) : Value();
                for (const QSharedPointer<ValueItem> &vi : authors) {
                    const QSharedPointer<Person> &p = vi.dynamicCast<Person>();
                    if (!p.isNull()) {
                        ++authorCounter;
                        stream << ENDL << "|last" << authorCounter << " = " << p->lastName();
                        if (!p->firstName().isEmpty())
                            stream << ENDL << "|first" << authorCounter << " = " << p->firstName();
                    }
                }

                stream  << ENDL << "}}" << ENDL;
                return true;
            },
            /*.writeMacro =*/ [](QTextStream &stream, const QSharedPointer<const Macro> &macro) {
                Q_UNUSED(stream);
                Q_UNUSED(macro);
                // Wikipedia Cite output does not support BibTeX macros
                return true;
            },
            /*.writeComment =*/ [](QTextStream &stream, const QSharedPointer<const Comment> &comment) {
                Q_UNUSED(stream);
                Q_UNUSED(comment);
                // Wikipedia Cite output does not support BibTeX comments
                return true;
            }
        }
    }
};


class FileExporterXML::Private
{
private:
    FileExporterXML *parent;

public:
    static const QHash<FileExporterXML::OutputStyle, std::function<bool(const QTextStream &)>> outputHeader;

    OutputStyle outputStyle;

    Private(FileExporterXML *p)
            : parent(p), outputStyle(FileExporterXML::OutputStyle::XML_KBibTeX)
    {
        // nothing
    }

    bool writeElement(QTextStream &stream, const QSharedPointer<const Element> element, const File *bibtexfile = nullptr)
    {
        bool result = false;

        const QSharedPointer<const Entry> &entry = element.dynamicCast<const Entry>();
        if (!entry.isNull()) {
            if (bibtexfile == nullptr)
                result |= rewriteFunctions[outputStyle].writeEntry(stream, entry);
            else {
                const QSharedPointer<const Entry> resolvedEntry(entry->resolveCrossref(bibtexfile));
                result |= rewriteFunctions[outputStyle].writeEntry(stream, resolvedEntry);
            }
        } else {
            const QSharedPointer<const Macro> &macro = element.dynamicCast<const Macro>();
            if (!macro.isNull())
                result |= rewriteFunctions[outputStyle].writeMacro(stream, macro);
            else {
                const QSharedPointer<const Comment> &comment = element.dynamicCast<const Comment>();
                if (!comment.isNull())
                    result |= rewriteFunctions[outputStyle].writeComment(stream, comment);
                else {
                    // preambles are ignored, make no sense in XML files
                }
            }
        }

        return result;
    }
};


FileExporterXML::FileExporterXML(QObject *parent)
        : FileExporter(parent), d(new Private(this))
{
    // nothing
}

FileExporterXML::~FileExporterXML()
{
    delete d;
}

void FileExporterXML::setOutputStyle(OutputStyle outputStyle)
{
    d->outputStyle = outputStyle;
}

bool FileExporterXML::save(QIODevice *iodevice, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = true;
    QTextStream stream(iodevice);
    // https://forum.qt.io/topic/135724/qt-6-replacement-for-qtextcodec
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    stream.setCodec("UTF-8");
#else
    stream.setEncoding(QStringConverter::Utf8);
#endif

    result &= rewriteFunctions[d->outputStyle].header(stream);

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result; ++it)
        result &= d->writeElement(stream, *it, bibtexfile);

    result &= rewriteFunctions[d->outputStyle].footer(stream);

    return result;
}

bool FileExporterXML::save(QIODevice *iodevice, const QSharedPointer<const Element> &element, const File *bibtexfile)
{
    Q_UNUSED(bibtexfile)

    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    QTextStream stream(iodevice);
    // https://forum.qt.io/topic/135724/qt-6-replacement-for-qtextcodec
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    stream.setCodec("UTF-8");
#else
    stream.setEncoding(QStringConverter::Utf8);
#endif

    bool result = rewriteFunctions[d->outputStyle].header(stream);

    result &= d->writeElement(stream, element);

    result &= rewriteFunctions[d->outputStyle].footer(stream);

    return result;
}

QDebug operator<<(QDebug dbg, FileExporterXML::OutputStyle outputStyle)
{
    switch (outputStyle) {
    case FileExporterXML::OutputStyle::XML_KBibTeX:
        dbg << "FileExporterXML::OutputStyle::XML_KBibTeX";
        break;
    case FileExporterXML::OutputStyle::HTML_Standard:
        dbg << "FileExporterXML::OutputStyle::HTML_Standard";
        break;
    case FileExporterXML::OutputStyle::HTML_AbstractOnly:
        dbg << "FileExporterXML::OutputStyle::HTML_AbstractOnly";
        break;
    case FileExporterXML::OutputStyle::Plain_WikipediaCite:
        dbg << "FileExporterXML::OutputStyle::Plain_WikipediaCite";
        break;
    default:
        dbg << "FileExporterXML::OutputStyle::???";
        break;
    }
    return dbg;
}
