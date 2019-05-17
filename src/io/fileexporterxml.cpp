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

#include "fileexporterxml.h"

#include <QRegularExpression>
#include <QStringList>

#include <KBibTeX>
#include <File>
#include <Entry>
#include <Macro>
#include <Comment>
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

bool FileExporterXML::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    Q_UNUSED(errorLog)

    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = true;
    m_cancelFlag = false;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << endl;
    stream << "<bibliography>" << endl;

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !m_cancelFlag; ++it)
        write(stream, (*it).data(), bibtexfile);

    stream << "</bibliography>" << endl;

    iodevice->close();
    return result && !m_cancelFlag;
}

bool FileExporterXML::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    Q_UNUSED(bibtexfile)
    Q_UNUSED(errorLog)

    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << endl;

    const bool result = write(stream, element.data());

    iodevice->close();
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
    stream << " <entry id=\"" << EncoderXML::instance().encode(entry->id(), Encoder::TargetEncodingUTF8) << "\" type=\"" << entry->type().toLower() << "\">" << endl;
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
            stream << ">" << endl;
            stream << valueToXML(internal, key) << endl;
            stream << "  </" << key << "s>" << endl;
        } else if (key == Entry::ftAbstract) {
            static const QRegularExpression abstractRegExp(QStringLiteral("\\bAbstract[:]?([ ]|&nbsp;|&amp;nbsp;)*"), QRegularExpression::CaseInsensitiveOption);
            /// clean up HTML artifacts
            QString text = valueToXML(value);
            text = text.remove(abstractRegExp);
            stream << "  <" << key << ">" << text << "</" << key << ">" << endl;
        } else if (key == Entry::ftMonth) {
            stream << "  <month";
            bool ok = false;

            int month = -1;
            QString tag;
            QString content;
            for (const auto &valueItem : value) {
                QSharedPointer<const MacroKey> macro = valueItem.dynamicCast<const MacroKey>();
                if (!macro.isNull())
                    for (int i = 0; i < 12; i++) {
                        if (QString::compare(macro->text(), KBibTeX::MonthsTriple[ i ]) == 0) {
                            if (month < 1) {
                                tag = KBibTeX::MonthsTriple[ i ];
                                month = i + 1;
                            }
                            content.append(KBibTeX::Months[ i ]);
                            ok = true;
                            break;
                        }
                    }
                else
                    content.append(PlainTextValue::text(valueItem));
            }

            if (!ok)
                content = valueToXML(value) ;
            if (!tag.isEmpty())
                stream << " tag=\"" << key << "\"";
            if (month > 0)
                stream << " month=\"" << month << "\"";
            stream << '>' << content;
            stream << "</month>" << endl;
        } else {
            stream << "  <" << key << ">" << valueToXML(value) << "</" << key << ">" << endl;
        }

    }
    stream << " </entry>" << endl;

    return true;
}

bool FileExporterXML::writeMacro(QTextStream &stream, const Macro *macro)
{
    stream << " <string key=\"" << macro->key() << "\">";
    stream << valueToXML(macro->value());
    stream << "</string>" << endl;

    return true;
}

bool FileExporterXML::writeComment(QTextStream &stream, const Comment *comment)
{
    stream << " <comment>" ;
    stream << EncoderXML::instance().encode(comment->text(), Encoder::TargetEncodingUTF8);
    stream << "</comment>" << endl;

    return true;
}

QString FileExporterXML::valueToXML(const Value &value, const QString &)
{
    QString result;
    bool isFirst = true;

    for (const auto &valueItem : value) {
        if (!isFirst)
            result.append(' ');
        isFirst = false;

        QSharedPointer<const PlainText> plainText = valueItem.dynamicCast<const PlainText>();
        if (!plainText.isNull())
            result.append("<text>" +  cleanXML(EncoderXML::instance().encode(PlainTextValue::text(valueItem), Encoder::TargetEncodingUTF8)) + "</text>");
        else {
            QSharedPointer<const Person> p = valueItem.dynamicCast<const Person>();
            if (!p.isNull()) {
                result.append("<person>");
                if (!p->firstName().isEmpty())
                    result.append("<firstname>" +  cleanXML(EncoderXML::instance().encode(p->firstName(), Encoder::TargetEncodingUTF8)) + "</firstname>");
                if (!p->lastName().isEmpty())
                    result.append("<lastname>" +  cleanXML(EncoderXML::instance().encode(p->lastName(), Encoder::TargetEncodingUTF8)) + "</lastname>");
                if (!p->suffix().isEmpty())
                    result.append("<suffix>" +  cleanXML(EncoderXML::instance().encode(p->suffix(), Encoder::TargetEncodingUTF8)) + "</suffix>");
                result.append("</person>");
            }
            // TODO: Other data types
            else
                result.append("<text>" + cleanXML(EncoderXML::instance().encode(PlainTextValue::text(valueItem), Encoder::TargetEncodingUTF8)) + "</text>");
        }
    }

    return result;
}

QString FileExporterXML::cleanXML(const QString &text)
{
    static const QRegularExpression removal(QStringLiteral("[{}]+"));
    static const QRegularExpression lineBreaksRegExp(QStringLiteral("[ \\t]*[\\n\\r]"));
    QString result = text;
    result = result.replace(lineBreaksRegExp, QStringLiteral("<br/>")).remove(removal).remove(QStringLiteral("\\ensuremath"));
    return result;
}
