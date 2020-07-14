/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <QRegularExpression>
#include <QStringList>

#include <KBibTeX>
#include <File>
#include <Entry>
#include <Macro>
#include <Comment>
#include "fileimporterbibtex.h"
#include "encoderxml.h"
#include "logging_io.h"

FileExporterXML::FileExporterXML(QObject *parent)
        : FileExporter(parent), m_cancelFlag(false)
{
    /// nothing
}

FileExporterXML::~FileExporterXML()
{
    /// nothing
}

bool FileExporterXML::save(QIODevice *iodevice, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = true;
    m_cancelFlag = false;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

#if QT_VERSION >= 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Qt::endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << Qt::endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << Qt::endl;
    stream << "<bibliography>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << endl;
    stream << "<bibliography>" << endl;
#endif // QT_VERSION >= 0x050e00

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !m_cancelFlag; ++it)
        result &= write(stream, (*it).data(), bibtexfile);

#if QT_VERSION >= 0x050e00
    stream << "</bibliography>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "</bibliography>" << endl;
#endif // QT_VERSION >= 0x050e00

    return result && !m_cancelFlag;
}

bool FileExporterXML::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile)
{
    Q_UNUSED(bibtexfile)

    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

#if QT_VERSION >= 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Qt::endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << Qt::endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << Qt::endl;
    stream << "<bibliography>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << endl;
    stream << "<bibliography>" << endl;
#endif // QT_VERSION >= 0x050e00

    const bool result = write(stream, element.data());

#if QT_VERSION >= 0x050e00
    stream << "</bibliography>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "</bibliography>" << endl;
#endif // QT_VERSION >= 0x050e00

    return result;
}

void FileExporterXML::cancel()
{
    m_cancelFlag = true;
}

bool FileExporterXML::write(QTextStream &stream, const Element *element, const File *bibtexfile)
{
    bool result = false;

    const Entry *entry = dynamic_cast<const Entry *>(element);
    if (entry != nullptr) {
        if (bibtexfile == nullptr)
            result |= writeEntry(stream, entry);
        else {
            QScopedPointer<const Entry> resolvedEntry(entry->resolveCrossref(bibtexfile));
            result |= writeEntry(stream, resolvedEntry.data());
        }
    } else {
        const Macro *macro = dynamic_cast<const Macro *>(element);
        if (macro != nullptr)
            result |= writeMacro(stream, macro);
        else {
            const Comment *comment = dynamic_cast<const Comment *>(element);
            if (comment != nullptr)
                result |= writeComment(stream, comment);
            else {
                // preambles are ignored, make no sense in XML files
            }
        }
    }

    return result;
}

bool FileExporterXML::writeEntry(QTextStream &stream, const Entry *entry)
{
#if QT_VERSION >= 0x050e00
    stream << " <entry id=\"" << EncoderXML::instance().encode(entry->id(), Encoder::TargetEncoding::UTF8) << "\" type=\"" << entry->type().toLower() << "\">" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << " <entry id=\"" << EncoderXML::instance().encode(entry->id(), Encoder::TargetEncoding::UTF8) << "\" type=\"" << entry->type().toLower() << "\">" << endl;
#endif // QT_VERSION >= 0x050e00
    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        const QString key = it.key().toLower();
        const Value value = it.value();

        if (key == Entry::ftAuthor || key == Entry::ftEditor) {
            Value internal = value;
            Value::ConstIterator lastIt = internal.constEnd();
            --lastIt;
            const QSharedPointer<ValueItem> last = *lastIt;
            stream << "  <" << key << "s";

            if (!value.isEmpty() && PlainText::isPlainText(*last)) {
                QSharedPointer<const PlainText> pt = internal.last().staticCast<const PlainText>();
                if (pt->text() == QStringLiteral("others")) {
                    internal.erase(internal.end() - 1);
                    stream << " etal=\"true\"";
                }
            }
#if QT_VERSION >= 0x050e00
            stream << ">" << Qt::endl;
            stream << valueToXML(internal) << Qt::endl;
            stream << "  </" << key << "s>" << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << ">" << endl;
            stream << valueToXML(internal) << endl;
            stream << "  </" << key << "s>" << endl;
#endif // QT_VERSION >= 0x050e00
        } else if (key == Entry::ftAbstract) {
            static const QRegularExpression abstractRegExp(QStringLiteral("\\bAbstract[:]?([ ]|&nbsp;|&amp;nbsp;)*"), QRegularExpression::CaseInsensitiveOption);
            /// clean up HTML artifacts
            QString text = valueToXML(value);
            text = text.remove(abstractRegExp);
#if QT_VERSION >= 0x050e00
            stream << "  <" << key << ">" << text << "</" << key << ">" << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << "  <" << key << ">" << text << "</" << key << ">" << endl;
#endif // QT_VERSION >= 0x050e00
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
            stream << '>' << valueToXML(value) << "</pages>";
#if QT_VERSION >= 0x050e00
            stream << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << endl;
#endif // QT_VERSION >= 0x050e00
        } else if (key == Entry::ftEdition) {
            const QString textualRepresentation = PlainTextValue::text(value);
            bool ok = false;
            const int asInt = FileImporterBibTeX::editionStringToNumber(textualRepresentation, &ok);
            const QString asText = ok && asInt > 0 ? QStringLiteral("<text>") + FileExporter::numberToOrdinal(asInt) + QStringLiteral("</text>") : valueToXML(value);

            stream << "  <edition";
            if (ok && asInt > 0)
                stream << " number=\"" << asInt << "\"";
            stream << '>' << asText << "</edition>";
#if QT_VERSION >= 0x050e00
            stream << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << endl;
#endif // QT_VERSION >= 0x050e00
        } else if (key == Entry::ftYear) {
            // Guess an int representing the year
            const QString textualRepresentation = value.count() > 0 ? PlainTextValue::text(value.first()) : QString();
            static const QRegularExpression yearRegExp(QStringLiteral("^(1[2-9]|2[01])\\d{2}$"));
            bool ok = false;
            const int asInt = yearRegExp.match(textualRepresentation).hasMatch() ? textualRepresentation.toInt(&ok) : -1;

            stream << "  <year";
            if (ok && asInt > 0)
                stream << " number=\"" << asInt << "\"";
            stream << '>' << valueToXML(value) << "</year>";
#if QT_VERSION >= 0x050e00
            stream << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << endl;
#endif // QT_VERSION >= 0x050e00
        } else if (key == Entry::ftMonth) {
            int asInt = -1;
            QString triple, content;
            for (const auto &valueItem : value) {
                bool gotMonthFromThisValueItem = false;
                const QString textualRepresentation = PlainTextValue::text(valueItem);
                for (int i = 0; asInt < 0 && i < 12; i++)
                    if (textualRepresentation == KBibTeX::MonthsTriple[ i ]) {
                        triple = KBibTeX::MonthsTriple[ i ];
                        asInt = i + 1;
                        gotMonthFromThisValueItem = true;
                    }
                if (gotMonthFromThisValueItem)
                    content.append(QStringLiteral("<text>") + KBibTeX::Months[asInt - 1] + QStringLiteral("</text>"));
                else
                    content.append(valueItemToXML(valueItem));
            }

            stream << "  <month";
            if (asInt >= 1 && asInt <= 12)
                stream << " triple=\"" << triple << "\" number=\"" << asInt << "\"";
            stream << '>' << content << "</month>";
#if QT_VERSION >= 0x050e00
            stream  << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << endl;
#endif // QT_VERSION >= 0x050e00
        } else {
            // Guess an int representing of this value
            const QString textualRepresentation = value.count() > 0 ? PlainTextValue::text(value.first()) : QString();
            static const QRegularExpression numberRegExp(QStringLiteral("^[1-9]\\d*$"));
            bool ok = false;
            const int asInt = numberRegExp.match(textualRepresentation).hasMatch() ? textualRepresentation.toInt(&ok) : -1;

            stream << "  <" << key;
            if (ok && asInt > 0)
                stream << " number=\"" << asInt << "\"";
            stream << '>' << valueToXML(value) << "</" << key << ">";
#if QT_VERSION >= 0x050e00
            stream << Qt::endl;
#else // QT_VERSION < 0x050e00
            stream << endl;
#endif // QT_VERSION >= 0x050e00
        }

    }
#if QT_VERSION >= 0x050e00
    stream << " </entry>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << " </entry>" << endl;
#endif // QT_VERSION >= 0x050e00

    return true;
}

bool FileExporterXML::writeMacro(QTextStream &stream, const Macro *macro)
{
    stream << " <string key=\"" << macro->key() << "\">";
    stream << valueToXML(macro->value());
#if QT_VERSION >= 0x050e00
    stream << "</string>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "</string>" << endl;
#endif // QT_VERSION >= 0x050e00

    return true;
}

bool FileExporterXML::writeComment(QTextStream &stream, const Comment *comment)
{
    stream << " <comment>" ;
    stream << EncoderXML::instance().encode(comment->text(), Encoder::TargetEncoding::UTF8);
#if QT_VERSION >= 0x050e00
    stream << "</comment>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "</comment>" << endl;
#endif // QT_VERSION >= 0x050e00

    return true;
}

QString FileExporterXML::valueToXML(const Value &value)
{
    QString result;

    for (const auto &valueItem : value)
        result.append(valueItemToXML(valueItem));

    return result;
}

QString FileExporterXML::valueItemToXML(const QSharedPointer<ValueItem> &valueItem)
{

    QSharedPointer<const PlainText> plainText = valueItem.dynamicCast<const PlainText>();
    if (!plainText.isNull())
        return QStringLiteral("<text>") +  cleanXML(EncoderXML::instance().encode(PlainTextValue::text(valueItem), Encoder::TargetEncoding::UTF8)) + QStringLiteral("</text>");
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

QString FileExporterXML::cleanXML(const QString &text)
{
    static const QRegularExpression removal(QStringLiteral("[{}]+"));
    static const QRegularExpression lineBreaksRegExp(QStringLiteral("[ \\t]*[\\n\\r]"));
    QString result = text;
    result = result.replace(lineBreaksRegExp, QStringLiteral("<br/>")).remove(removal).remove(QStringLiteral("\\ensuremath"));
    return result;
}
