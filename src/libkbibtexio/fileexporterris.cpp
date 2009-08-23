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

#include <entry.h>

#include "fileexporterris.h"

using namespace KBibTeX::IO;

FileExporterRIS::FileExporterRIS() : FileExporter()
{
    // nothing
}

FileExporterRIS::~FileExporterRIS()
{
    // nothing
}

bool FileExporterRIS::save(QIODevice* iodevice, const Element* element, QStringList* /*errorLog*/)
{
    m_mutex.lock();
    bool result = false;
    QTextStream stream(iodevice);

    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL)
        result = writeEntry(stream, entry);

    m_mutex.unlock();
    return result && !m_cancelFlag;
}

bool FileExporterRIS::save(QIODevice* iodevice, const File* bibtexfile, QStringList* /*errorLog*/)
{
    m_mutex.lock();
    bool result = true;
    m_cancelFlag = false;
    QTextStream stream(iodevice);

    for (File::ConstIterator it = bibtexfile->begin(); it != bibtexfile->end() && result && !m_cancelFlag; it++) {
        const Entry *entry = dynamic_cast<const Entry*>(*it);
        if (entry != NULL) {
//                 FIXME Entry *myEntry = bibtexfile->completeReferencedFieldsConst( entry );
            Entry *myEntry = new Entry(entry);
            result &= writeEntry(stream, myEntry);
            delete myEntry;
        }
    }

    m_mutex.unlock();
    return result && !m_cancelFlag;
}

void FileExporterRIS::cancel()
{
    m_cancelFlag = true;
}

bool FileExporterRIS::writeEntry(QTextStream &stream, const Entry* entry, const File* bibtexfile)
{
    bool result = true;

    switch (entry->entryType()) {
    case Entry::etBook:
        writeKeyValue(stream, "TY", "BOOK");
        break;
    case Entry::etInBook:
        writeKeyValue(stream, "TY", "CHAP");
        break;
    case Entry::etInProceedings:
        writeKeyValue(stream, "TY", "CONF");
        break;
    case Entry::etArticle:
        writeKeyValue(stream, "TY", "JOUR");
        break;
    case Entry::etTechReport:
        writeKeyValue(stream, "TY", "RPRT");
        break;
    case Entry::etPhDThesis:
        writeKeyValue(stream, "TY", "THES");
        break;
    default:
        writeKeyValue(stream, "TY", "GEN");
    }

    QString year = "";
    QString month = "";

    for (Entry::Fields::ConstIterator it = entry->begin(); result && it != entry->end(); it++) {
        Field *field = *it;
        QString plainText = PlainTextValue::text(field->value(), bibtexfile);
        Value value = field->value();

        if (field->fieldType() == Field::ftUnknown && field->fieldTypeName().startsWith("RISfield_"))
            result &= writeKeyValue(stream, field->fieldTypeName().right(2), plainText);
        else if (field->fieldType() == Field::ftAuthor) {
            for (QLinkedList<ValueItem*>::ConstIterator it = value.begin();  result && it != value.end(); ++it) {
                PersonContainer *pc = dynamic_cast<PersonContainer*>(*it);
                if (pc == NULL) continue;
                for (PersonContainer::Iterator pit = pc->begin(); result && pit != pc->end(); ++pit)
                    result &= writeKeyValue(stream, "AU", PlainTextValue::text(**it, bibtexfile));
            }
        } else if (field->fieldType() == Field::ftEditor) {
            for (QLinkedList<ValueItem*>::ConstIterator it = value.begin();  result && it != value.end(); ++it) {
                PersonContainer *pc = dynamic_cast<PersonContainer*>(*it);
                if (pc == NULL) continue;
                for (PersonContainer::Iterator pit = pc->begin(); result && pit != pc->end(); ++pit)
                    result &= writeKeyValue(stream, "ED", PlainTextValue::text(**it, bibtexfile));
            }
        } else if (field->fieldType() == Field::ftTitle)
            result &= writeKeyValue(stream, "TI", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftJournal)
            result &= writeKeyValue(stream, "JO", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftChapter)
            result &= writeKeyValue(stream, "CP", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftISSN)
            result &= writeKeyValue(stream, "SN", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftISBN)
            result &= writeKeyValue(stream, "SN", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftVolume)
            result &= writeKeyValue(stream, "VL", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftNumber)
            result &= writeKeyValue(stream, "IS", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftNote)
            result &= writeKeyValue(stream, "N1", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftAbstract)
            result &= writeKeyValue(stream, "N2", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftPublisher)
            result &= writeKeyValue(stream, "PB", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftLocation)
            result &= writeKeyValue(stream, "CY", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftKeywords)
            result &= writeKeyValue(stream, "KW", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftYear)
            year = PlainTextValue::text(value, bibtexfile);
        else if (field->fieldType() == Field::ftMonth)
            month = PlainTextValue::text(value, bibtexfile);
        else if (field->fieldType() == Field::ftAddress)
            result &= writeKeyValue(stream, "AD", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftURL)
            result &= writeKeyValue(stream, "UR", PlainTextValue::text(value, bibtexfile));
        else if (field->fieldType() == Field::ftPages) {
            QStringList pageRange = PlainTextValue::text(value, bibtexfile).split(QRegExp(QString("--|-|%1").arg(QChar(0x2013))));
            if (pageRange.count() == 2) {
                result &= writeKeyValue(stream, "SP", pageRange[ 0 ]);
                result &= writeKeyValue(stream, "EP", pageRange[ 1 ]);
            }
        } else if (field->fieldTypeName().toLower() == "doi")
            result &= writeKeyValue(stream, "UR", PlainTextValue::text(value, bibtexfile));
    }

    if (!year.isEmpty() || !month.isEmpty()) {
        result &= writeKeyValue(stream, "PY", QString("%1/%2//").arg(year).arg(month));
    }

    result &= writeKeyValue(stream, "ER", QString());
    stream << endl;

    return result;
}

bool FileExporterRIS::writeKeyValue(QTextStream &stream, const QString& key, const QString& value)
{
    stream << key << "  - ";
    if (!value.isEmpty())
        stream << value;
    stream << endl;

    return true;
}
