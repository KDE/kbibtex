/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <QRegExp>
#include <QStringList>

#include <file.h>
#include <element.h>
#include <entry.h>
#include <encoderxml.h>
#include <macro.h>
#include <comment.h>
#include "fileexporterxml.h"

using namespace KBibTeX::IO;

FileExporterXML::FileExporterXML() : FileExporter()
{
    // nothing
}


FileExporterXML::~FileExporterXML()
{
    // nothing
}

bool FileExporterXML::save(QIODevice* iodevice, const File* bibtexfile, QStringList * /*errorLog*/)
{
    m_mutex.lock();
    bool result = true;
    m_cancelFlag = FALSE;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX/KDE4 -->" << endl;
    stream << "<!-- http://home.gna.org/kbibtex/ -->" << endl;
    stream << "<bibliography>" << endl;

    for (File::ConstIterator it = bibtexfile->begin(); it != bibtexfile->end() && result && !m_cancelFlag; ++it)
        write(stream, *it, bibtexfile);

    stream << "</bibliography>" << endl;

    m_mutex.unlock();
    return result && !m_cancelFlag;
}

bool FileExporterXML::save(QIODevice* iodevice, const Element* element, QStringList * /*errorLog*/)
{
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    return write(stream, element);
}

void FileExporterXML::cancel()
{
    m_cancelFlag = true;
}

bool FileExporterXML::write(QTextStream& stream, const Element* element, const File* bibtexfile)
{
    bool result = FALSE;

    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL) {
        if (bibtexfile != NULL) {
// FIXME                entry = bibtexfile->completeReferencedFieldsConst( entry );
            entry = new Entry(entry);
        }
        result |= writeEntry(stream, entry);
        if (bibtexfile != NULL)
            delete entry;
    } else {
        const Macro * macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL)
            result |= writeMacro(stream, macro);
        else {
            const Comment * comment = dynamic_cast<const Comment*>(element);
            if (comment != NULL)
                result |= writeComment(stream, comment);
            else {
                // preambles are ignored, make no sense in XML files
            }
        }
    }

    return result;
}

bool FileExporterXML::writeEntry(QTextStream &stream, const Entry* entry)
{
    stream << " <entry id=\"" << EncoderXML::currentEncoderXML() ->encode(entry->id()) << "\" type=\"" << entry->entryTypeString().toLower() << "\">" << endl;
    for (Entry::EntryFields::ConstIterator it = entry->begin(); it != entry->end(); ++it) {
        EntryField *field = *it;
        switch (field->fieldType()) {
        case EntryField::ftAuthor:
        case EntryField::ftEditor: {
            QString tag = field->fieldTypeName().toLower();
            stream << "  <" << tag << "s>" << endl;
            QStringList persons = EncoderXML::currentEncoderXML() ->encode(valueToString(field->value())).split(QRegExp("\\s+(,|and|&)+\\s+", Qt::CaseInsensitive));
            for (QStringList::Iterator it = persons.begin(); it != persons.end(); ++it)
                stream << "   <person>" << *it << "</person>" << endl;
            stream << "  </" << tag << "s>" << endl;
        }
        break;
        case EntryField::ftMonth: {
            stream << "  <month";
            bool ok = FALSE;

            int month = -1;
            QString tag = "";
            QString content = "";
            for (QLinkedList<ValueItem*>::ConstIterator it = field->value()->items.begin(); it != field->value()->items.end(); ++it) {
                if (dynamic_cast<MacroKey*>(*it) != NULL) {
                    for (int i = 0; i < 12; i++)
                        if (QString::compare((*it)->text(), MonthsTriple[ i ]) == 0) {
                            if (month < 1) {
                                tag = MonthsTriple[ i ];
                                month = i + 1;
                            }
                            content.append(Months[ i ]);
                            ok = true;
                            break;
                        }
                } else
                    content.append(EncoderXML::currentEncoderXML() ->encode((*it)->text()));
            }

            if (!ok)
                content = EncoderXML::currentEncoderXML() ->encode(field->value()->simplifiedText()) ;
            if (!tag.isEmpty())
                stream << " tag=\"" << tag << "\"";
            if (month > 0)
                stream << " month=\"" << month << "\"";
            stream << '>' << content;
            stream << "</month>" << endl;
        }
        break;
        default: {
            QString tag = field->fieldTypeName().toLower();
            stream << "  <" << tag << ">" << EncoderXML::currentEncoderXML() ->encode(valueToString(field->value())) << "</" << tag << ">" << endl;
        }
        break;
        }

    }
    stream << " </entry>" << endl;

    return true;
}

bool FileExporterXML::writeMacro(QTextStream &stream, const Macro* macro)
{
    stream << " <string key=\"" << macro->key() << "\">";
    stream << EncoderXML::currentEncoderXML() ->encode(valueToString(macro->value()));
    stream << "</string>" << endl;

    return true;
}

bool FileExporterXML::writeComment(QTextStream &stream, const Comment* comment)
{
    stream << " <comment>" ;
    stream << EncoderXML::currentEncoderXML() ->encode(comment->text());
    stream << "</comment>" << endl;

    return true;
}

QString FileExporterXML::valueToString(Value *value)
{
    QString result;
    bool isFirst = true;

    for (QLinkedList<ValueItem*>::ConstIterator it = value->items.begin(); it != value->items.end(); ++it) {
        if (!isFirst)
            result.append(' ');
        isFirst = FALSE;

        result.append((*it) ->simplifiedText());
    }

    return result;
}
