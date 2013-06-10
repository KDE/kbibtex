/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include <QRegExp>
#include <QStringList>

#include "file.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"
#include "encoderxml.h"
#include "iocommon.h"
#include "fileexporterxml.h"

static QRegExp removal("[{}]+");
static QRegExp abstractRegExp("\\bAbstract[:]?([ ]|&nbsp;|&amp;nbsp;)*", Qt::CaseInsensitive);
static QRegExp lineBreaksRegExp("[ \\t]*[\\n\\r]");

FileExporterXML::FileExporterXML()
        : FileExporter()
{
    // nothing
}

FileExporterXML::~FileExporterXML()
{
    // nothing
}

bool FileExporterXML::save(QIODevice *iodevice, const File *bibtexfile, QStringList * /*errorLog*/)
{
    bool result = true;
    m_cancelFlag = false;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX/KDE4 -->" << endl;
    stream << "<!-- http://home.gna.org/kbibtex/ -->" << endl;
    stream << "<bibliography>" << endl;

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !m_cancelFlag; ++it)
        write(stream, (*it).data(), bibtexfile);

    stream << "</bibliography>" << endl;

    return result && !m_cancelFlag;
}

bool FileExporterXML::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File * /*bibtexfile*/, QStringList * /*errorLog*/)
{
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX/KDE4 -->" << endl;
    stream << "<!-- http://home.gna.org/kbibtex/ -->" << endl;
    return write(stream, element.data());
}

void FileExporterXML::cancel()
{
    m_cancelFlag = true;
}

bool FileExporterXML::write(QTextStream &stream, const Element *element, const File *bibtexfile)
{
    bool result = FALSE;

    const Entry *entry = dynamic_cast<const Entry *>(element);
    if (entry != NULL) {
        if (bibtexfile != NULL)
            entry = entry->resolveCrossref(bibtexfile);
        result |= writeEntry(stream, entry);
        if (bibtexfile != NULL)
            delete entry; /// delete artificially created Entry from resolveCrossref(..)
    } else {
        const Macro *macro = dynamic_cast<const Macro *>(element);
        if (macro != NULL)
            result |= writeMacro(stream, macro);
        else {
            const Comment *comment = dynamic_cast<const Comment *>(element);
            if (comment != NULL)
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
    stream << " <entry id=\"" << EncoderXML::currentEncoderXML() ->encode(entry->id()) << "\" type=\"" << entry->type().toLower() << "\">" << endl;
    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        const QString key = it.key().toLower();
        const Value value = it.value();

        if (key == Entry::ftAuthor || key == Entry::ftEditor) {
            Value internal = value;
            stream << "  <" << key << "s";
            if (!value.isEmpty() && typeid(PlainText) == typeid(*internal.last())) {
                QSharedPointer<const PlainText> pt = internal.last().staticCast<const PlainText>();
                if (pt->text() == QLatin1String("others")) {
                    internal.erase(internal.end() - 1);
                    stream << " etal=\"true\"";
                }
            }
            stream << ">" << endl;
            stream << valueToXML(internal, key) << endl;
            stream << "  </" << key << "s>" << endl;
        } else if (key == Entry::ftAbstract) {
            /// clean up HTML artifacts
            QString text = valueToXML(value);
            text = text.replace(abstractRegExp, "");
            stream << "  <" << key << ">" << text << "</" << key << ">" << endl;
        } else if (key == Entry::ftMonth) {
            stream << "  <month";
            bool ok = FALSE;

            int month = -1;
            QString tag = "";
            QString content = "";
            for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
                QSharedPointer<const MacroKey> macro = (*it).dynamicCast<const MacroKey>();
                if (!macro.isNull())
                    for (int i = 0; i < 12; i++) {
                        if (QString::compare(macro->text(), MonthsTriple[ i ]) == 0) {
                            if (month < 1) {
                                tag = MonthsTriple[ i ];
                                month = i + 1;
                            }
                            content.append(Months[ i ]);
                            ok = true;
                            break;
                        }
                    }
                else
                    content.append(PlainTextValue::text(**it));
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
    stream << EncoderXML::currentEncoderXML() ->encode(comment->text());
    stream << "</comment>" << endl;

    return true;
}

QString FileExporterXML::valueToXML(const Value &value, const QString &)
{
    QString result;
    bool isFirst = true;

    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        if (!isFirst)
            result.append(' ');
        isFirst = false;

        QSharedPointer<const ValueItem> item = *it;

        QSharedPointer<const PlainText> plainText = (*it).dynamicCast<const PlainText>();
        if (!plainText.isNull())
            result.append("<text>" +  cleanXML(EncoderXML::currentEncoderXML() ->encode(PlainTextValue::text(*item))) + "</text>");
        else {
            QSharedPointer<const Person> p = (*it).dynamicCast<const Person>();
            if (!p.isNull()) {
                result.append("<person>");
                if (!p->firstName().isEmpty())
                    result.append("<firstname>" +  cleanXML(EncoderXML::currentEncoderXML() ->encode(p->firstName())) + "</firstname>");
                if (!p->lastName().isEmpty())
                    result.append("<lastname>" +  cleanXML(EncoderXML::currentEncoderXML() ->encode(p->lastName())) + "</lastname>");
                if (!p->suffix().isEmpty())
                    result.append("<suffix>" +  cleanXML(EncoderXML::currentEncoderXML() ->encode(p->suffix())) + "</suffix>");
                result.append("</person>");
            }
            // TODO: Other data types
            else
                result.append("<text>" + cleanXML(EncoderXML::currentEncoderXML() ->encode(PlainTextValue::text(*item))) + "</text>");
        }
    }

    return result;
}

QString FileExporterXML::cleanXML(const QString &text)
{
    QString result = text;
    result = result.replace(lineBreaksRegExp, "<br/>").replace(removal, QString()).replace(QLatin1String("\\ensuremath"), QString());
    return result;
}
