/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "fileexporterris.h"

#include <QRegExp>
#include <QStringList>

#include "entry.h"
#include "logging_io.h"

FileExporterRIS::FileExporterRIS()
        : FileExporter(), m_cancelFlag(false)
{
    // nothing
}

FileExporterRIS::~FileExporterRIS()
{
    // nothing
}

bool FileExporterRIS::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File * /*bibtexfile*/, QStringList * /*errorLog*/)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCDebug(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;
    m_cancelFlag = false;
    QTextStream stream(iodevice);

    const QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull())
        result = writeEntry(stream, entry.data());

    iodevice->close();
    return result && !m_cancelFlag;
}

bool FileExporterRIS::save(QIODevice *iodevice, const File *bibtexfile, QStringList * /*errorLog*/)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCDebug(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = true;
    m_cancelFlag = false;
    QTextStream stream(iodevice);

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !m_cancelFlag; ++it) {
        const QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
        if (!entry.isNull()) {
//                 FIXME Entry *myEntry = bibtexfile->completeReferencedFieldsConst( entry );
            //Entry *myEntry = new Entry(*entry);
            result &= writeEntry(stream, entry.data());
            //delete myEntry;
        }
    }

    iodevice->close();
    return result && !m_cancelFlag;
}

void FileExporterRIS::cancel()
{
    m_cancelFlag = true;
}

bool FileExporterRIS::writeEntry(QTextStream &stream, const Entry *entry)
{
    bool result = true;
    QString type = entry->type();

    if (type == Entry::etBook)
        writeKeyValue(stream, "TY", "BOOK");
    else if (type == Entry::etInBook)
        writeKeyValue(stream, "TY", "CHAP");
    else if (type == Entry::etInProceedings)
        writeKeyValue(stream, "TY", "CONF");
    else if (type == Entry::etArticle)
        writeKeyValue(stream, "TY", "JOUR");
    else if (type == Entry::etTechReport)
        writeKeyValue(stream, "TY", "RPRT");
    else if (type == Entry::etPhDThesis || type == Entry::etMastersThesis)
        writeKeyValue(stream, "TY", "THES");
    else if (type == Entry::etUnpublished)
        writeKeyValue(stream, "TY", "UNPB");
    else
        writeKeyValue(stream, "TY", "GEN");

    writeKeyValue(stream, "ID", entry->id());

    QString year, month;

    for (Entry::ConstIterator it = entry->constBegin(); result && it != entry->constEnd(); ++it) {
        const QString key = it.key();
        const Value value = it.value();

        if (key.startsWith(QStringLiteral("RISfield_")))
            result &= writeKeyValue(stream, key.right(2), PlainTextValue::text(value));
        else if (key == Entry::ftAuthor) {
            for (Value::ConstIterator it = value.constBegin(); result && it != value.constEnd(); ++it) {
                QSharedPointer<const Person> person = (*it).dynamicCast<const Person>();
                if (!person.isNull())
                    result &= writeKeyValue(stream, "AU", PlainTextValue::text(**it));
                else
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot write value " << PlainTextValue::text(**it) << " for field AU (author), not supported by RIS format" << endl;
            }
        } else if (key.toLower() == Entry::ftEditor) {
            for (Value::ConstIterator it = value.constBegin(); result && it != value.constEnd(); ++it) {
                QSharedPointer<const Person> person = (*it).dynamicCast<const Person>();
                if (!person.isNull())
                    result &= writeKeyValue(stream, "ED", PlainTextValue::text(**it));
                else
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot write value " << PlainTextValue::text(**it) << " for field ED (editor), not supported by RIS format" << endl;
            }
        } else if (key == Entry::ftTitle)
            result &= writeKeyValue(stream, "TI", PlainTextValue::text(value));
        else if (key == Entry::ftBookTitle)
            result &= writeKeyValue(stream, "BT", PlainTextValue::text(value));
        else if (key == Entry::ftSeries)
            result &= writeKeyValue(stream, "T3", PlainTextValue::text(value));
        else if (key == Entry::ftJournal)
            result &= writeKeyValue(stream, "JO", PlainTextValue::text(value)); ///< "JF" instead?
        else if (key == Entry::ftChapter)
            result &= writeKeyValue(stream, "CP", PlainTextValue::text(value));
        else if (key == Entry::ftISSN)
            result &= writeKeyValue(stream, "SN", PlainTextValue::text(value));
        else if (key == Entry::ftISBN)
            result &= writeKeyValue(stream, "SN", PlainTextValue::text(value));
        else if (key == Entry::ftSchool) /// == "institution"
            result &= writeKeyValue(stream, "IN", PlainTextValue::text(value));
        else if (key == Entry::ftVolume)
            result &= writeKeyValue(stream, "VL", PlainTextValue::text(value));
        else if (key == Entry::ftNumber) /// == "issue"
            result &= writeKeyValue(stream, "IS", PlainTextValue::text(value));
        else if (key == Entry::ftNote)
            result &= writeKeyValue(stream, "N1", PlainTextValue::text(value));
        else if (key == Entry::ftAbstract)
            result &= writeKeyValue(stream, "N2", PlainTextValue::text(value)); ///< "AB" instead?
        else if (key == Entry::ftPublisher)
            result &= writeKeyValue(stream, "PB", PlainTextValue::text(value));
        else if (key == Entry::ftLocation)
            result &= writeKeyValue(stream, "CY", PlainTextValue::text(value));
        else if (key == Entry::ftDOI)
            result &= writeKeyValue(stream, "DO", PlainTextValue::text(value));
        else if (key == Entry::ftKeywords)
            result &= writeKeyValue(stream, "KW", PlainTextValue::text(value));
        else if (key == Entry::ftYear)
            year = PlainTextValue::text(value);
        else if (key == Entry::ftMonth)
            month = PlainTextValue::text(value);
        else if (key == Entry::ftAddress)
            result &= writeKeyValue(stream, "AD", PlainTextValue::text(value));
        else if (key == Entry::ftUrl) {
            // FIXME one "UR" line per URL
            // FIXME for local files, use "L1"
            result &= writeKeyValue(stream, "UR", PlainTextValue::text(value));
        } else if (key == Entry::ftPages) {
            QStringList pageRange = PlainTextValue::text(value).split(QRegExp(QString("--|-|%1").arg(QChar(0x2013))));
            if (pageRange.count() == 2) {
                result &= writeKeyValue(stream, "SP", pageRange[ 0 ]);
                result &= writeKeyValue(stream, "EP", pageRange[ 1 ]);
            }
        }
    }

    if (!year.isEmpty() || !month.isEmpty()) {
        result &= writeKeyValue(stream, "PY", QString("%1/%2//").arg(year).arg(month));
    }

    result &= writeKeyValue(stream, "ER", QString());
    stream << endl;

    return result;
}

bool FileExporterRIS::writeKeyValue(QTextStream &stream, const QString &key, const QString &value)
{
    stream << key << "  - ";
    if (!value.isEmpty())
        stream << value;
    stream << endl;

    return true;
}
