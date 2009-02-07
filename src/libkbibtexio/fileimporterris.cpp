/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QTextStream>
#include <QRegExp>
#include <QCoreApplication>

#include <entry.h>

#include "fileimporterris.h"

using namespace KBibTeX::IO;

FileImporterRIS::FileImporterRIS() : FileImporter()
{
// nothing
}


FileImporterRIS::~FileImporterRIS()
{
// nothing
}

File* FileImporterRIS::load(QIODevice *iodevice)
{
    m_mutex.lock();
    cancelFlag = FALSE;
    m_refNr = 0;
    QTextStream textStream(iodevice);

    File *result = new File();
    while (!cancelFlag && !textStream.atEnd()) {
        emit progress(textStream.pos(), iodevice->size());
        QCoreApplication::instance()->processEvents();
        Element * element = nextElement(textStream);
        if (element != NULL)
            result->append(element);
        QCoreApplication::instance()->processEvents();
    }
    emit progress(100, 100);

    if (cancelFlag) {
        delete result;
        result = NULL;
    }

    m_mutex.unlock();
    return result;
}

bool FileImporterRIS::guessCanDecode(const QString & text)
{
    return text.indexOf("TY  - ") >= 0;
}

void FileImporterRIS::cancel()
{
    cancelFlag = TRUE;
}

Element *FileImporterRIS::nextElement(QTextStream &textStream)
{
    RISitemList list = readElement(textStream);
    if (list.empty())
        return NULL;

    Entry::EntryType entryType = Entry::etMisc;
    Entry *entry = new Entry(entryType, QString("RIS_%1").arg(m_refNr++));
    QStringList authorList, editorList, keywordList;
    QString journalName, abstract, startPage, endPage, date;
    int fieldNr = 0;

    for (RISitemList::iterator it = list.begin(); it != list.end(); ++it) {
        if ((*it).key == "TY") {
            if ((*it).value.startsWith("BOOK") || (*it).value.startsWith("SER"))
                entryType = Entry::etBook;
            else if ((*it).value.startsWith("CHAP"))
                entryType = Entry::etInBook;
            else if ((*it).value.startsWith("CONF"))
                entryType = Entry::etInProceedings;
            else if ((*it).value.startsWith("JFULL") || (*it).value.startsWith("JOUR") || (*it).value.startsWith("MGZN"))
                entryType = Entry::etArticle;
            else if ((*it).value.startsWith("RPRT"))
                entryType = Entry::etTechReport;
            else if ((*it).value.startsWith("THES"))
                entryType = Entry::etPhDThesis;
            entry->setEntryType(entryType);
        } else if ((*it).key == "AU" || (*it).key == "A1") {
            authorList.append((*it).value);
        } else if ((*it).key == "ED" || (*it).key == "A2") {
            editorList.append((*it).value);
        } else if ((*it).key == "ID") {
            entry->setId((*it).value);
        } else if ((*it).key == "Y1" || (*it).key == "PY") {
            date = (*it).value;
        } else if ((*it).key == "Y2") {
            if (date.isEmpty())
                date = (*it).value;
        } else if ((*it).key == "N1" /*|| ( *it ).key == "N2"*/) {
            EntryField * field = entry->getField(EntryField::ftNote);
            if (field == NULL) {
                field = new EntryField(EntryField::ftNote);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "N2") {
            if (abstract.isEmpty())
                abstract = (*it).value ;
        } else if ((*it).key == "AB") {
            abstract = (*it).value ;
        } else if ((*it).key == "KW") {
            QString text = (*it).value;
            QRegExp splitRegExp;
            if (text.contains(";"))
                splitRegExp = QRegExp("\\s*[;\\n]\\s*");
            else if (text.contains(","))
                splitRegExp = QRegExp("\\s*[,\\n]\\s*");
            else
                splitRegExp = QRegExp("\\n");
            QStringList newKeywords = text.split(splitRegExp, QString::SkipEmptyParts);
            for (QStringList::Iterator it = newKeywords.begin(); it != newKeywords.end();
                    ++it)
                keywordList.append(*it);
        } else if ((*it).key == "TI" || (*it).key == "T1") {
            EntryField * field = entry->getField(EntryField::ftTitle);
            if (field == NULL) {
                field = new EntryField(EntryField::ftTitle);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "T3") {
            EntryField * field = entry->getField(EntryField::ftSeries);
            if (field == NULL) {
                field = new EntryField(EntryField::ftSeries);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "JA" || (*it).key == "J1" || (*it).key == "J2") {
            if (journalName.isEmpty())
                journalName = (*it).value;
        } else if ((*it).key == "JF" || (*it).key == "JO") {
            journalName = (*it).value;
        } else if ((*it).key == "VL") {
            EntryField * field = entry->getField(EntryField::ftVolume);
            if (field == NULL) {
                field = new EntryField(EntryField::ftVolume);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "CP") {
            EntryField * field = entry->getField(EntryField::ftVolume);
            if (field == NULL) {
                field = new EntryField(EntryField::ftChapter);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "IS") {
            EntryField * field = entry->getField(EntryField::ftNumber);
            if (field == NULL) {
                field = new EntryField(EntryField::ftNumber);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "PB") {
            EntryField * field = entry->getField(EntryField::ftPublisher);
            if (field == NULL) {
                field = new EntryField(EntryField::ftPublisher);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "SN") {
            EntryField::FieldType fieldType = entryType == Entry::etBook || entryType == Entry::etInBook ? EntryField::ftISBN : EntryField::ftISSN;
            EntryField * field = entry->getField(fieldType);
            if (field == NULL) {
                field = new EntryField(fieldType);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "CY") {
            EntryField * field = entry->getField(EntryField::ftLocation);
            if (field == NULL) {
                field = new EntryField(EntryField::ftLocation);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "AD") {
            EntryField * field = entry->getField(EntryField::ftAddress);
            if (field == NULL) {
                field = new EntryField(EntryField::ftAddress);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "L1") {
            EntryField * field = entry->getField("PDF");
            if (field == NULL) {
                field = new EntryField("PDF");
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "UR") {
            EntryField * field = NULL;
            if ((*it).value.contains("dx.doi.org")) {
                field = entry->getField("DOI");
                if (field == NULL) {
                    field = new EntryField("DOI");
                    entry->addField(field);
                }
            } else {
                field = entry->getField(EntryField::ftURL);
                if (field == NULL) {
                    field = new EntryField(EntryField::ftURL);
                    entry->addField(field);
                }
            }
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        } else if ((*it).key == "SP") {
            startPage = (*it).value;
        } else if ((*it).key == "EP") {
            endPage = (*it).value;
        } else {
            QString fieldName = QString("RISfield_%1_%2").arg(fieldNr++).arg((*it).key.left(2));
            EntryField * field = new EntryField(fieldName);
            entry->addField(field);
            Value *value = new Value();
            value->items.append(new PlainText((*it).value));
            field->setValue(value);
        }
    }

    if (!authorList.empty()) {
        EntryField * field = entry->getField(EntryField::ftAuthor);
        if (field == NULL) {
            field = new EntryField(EntryField::ftAuthor);
            entry->addField(field);
        }
        Value *value = new Value();
        PersonContainer *container = new PersonContainer();
        value->items.append(container);
        for (QStringList::Iterator pit = authorList.begin(); pit != authorList.end();++pit)
            container->persons.append(new Person(*pit));
        field->setValue(value);
    }

    if (!editorList.empty()) {
        EntryField * field = entry->getField(EntryField::ftEditor);
        if (field == NULL) {
            field = new EntryField(EntryField::ftEditor);
            entry->addField(field);
        }
        Value *value = new Value();
        PersonContainer *container = new PersonContainer();
        value->items.append(container);
        for (QStringList::Iterator pit = authorList.begin(); pit != authorList.end();++pit)
            container->persons.append(new Person(*pit));
        field->setValue(value);
    }

    if (!keywordList.empty()) {
        EntryField * field = entry->getField(EntryField::ftKeywords);
        if (field == NULL) {
            field = new EntryField(EntryField::ftKeywords);
            entry->addField(field);
        }
        Value *value = new Value();
        KeywordContainer *container = new KeywordContainer();
        value->items.append(container);
        for (QStringList::Iterator pit = keywordList.begin(); pit != keywordList.end();++pit)
            container->keywords.append(new Keyword(*pit));
        field->setValue(value);
    }

    if (!journalName.isEmpty()) {
        EntryField * field = entry->getField(entryType == Entry::etInBook || entryType == Entry::etInProceedings ? EntryField::ftBookTitle : EntryField::ftJournal);
        if (field == NULL) {
            field = new EntryField(EntryField::ftJournal);
            entry->addField(field);
        }
        Value *value = new Value();
        value->items.append(new PlainText(journalName));
        field->setValue(value);
    }

    if (!abstract.isEmpty()) {
        EntryField * field = entry->getField(EntryField::ftAbstract);
        if (field == NULL) {
            field = new EntryField(EntryField::ftAbstract);
            entry->addField(field);
        }
        Value *value = new Value();
        value->items.append(new PlainText(abstract));
        field->setValue(value);
    }

    if (!startPage.isEmpty() || !endPage.isEmpty()) {
        EntryField * field = entry->getField(EntryField::ftPages);
        if (field == NULL) {
            field = new EntryField(EntryField::ftPages);
            entry->addField(field);
        }
        QString page;
        if (startPage.isEmpty())
            page = endPage;
        else if (endPage.isEmpty())
            page = startPage;
        else
            page = startPage + QChar(0x2013) + endPage;

        Value *value = new Value();
        value->items.append(new PlainText(page));
        field->setValue(value);
    }

    QStringList dateFragments = date.split("/", QString::SkipEmptyParts);
    if (dateFragments.count() > 0) {
        bool ok;
        int year = dateFragments[ 0 ].toInt(&ok);
        if (ok && year > 1000 && year < 3000) {
            EntryField * field = entry->getField(EntryField::ftYear);
            if (field == NULL) {
                field = new EntryField(EntryField::ftYear);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText(QString::number(year)));
            field->setValue(value);
        }
    }
    if (dateFragments.count() > 1) {
        bool ok;
        int month = dateFragments[ 0 ].toInt(&ok);
        if (ok && month > 0 && month < 13) {
            EntryField * field = entry->getField(EntryField::ftMonth);
            if (field == NULL) {
                field = new EntryField(EntryField::ftMonth);
                entry->addField(field);
            }
            Value *value = new Value();
            value->items.append(new PlainText(QString::number(month)));
            field->setValue(value);
        }
    }

    return entry;
}

FileImporterRIS::RISitemList FileImporterRIS::readElement(QTextStream &textStream)
{
    RISitemList result;
    QString line = textStream.readLine();
    while (!line.startsWith("TY  - ") && !textStream.atEnd())
        line = textStream.readLine();
    if (textStream.atEnd())
        return result;

    QString key, value;
    while (!line.startsWith("ER  -") && !textStream.atEnd()) {
        if (line.mid(2, 3) == "  -") {
            if (!value.isEmpty()) {
                RISitem item;
                item.key = key;
                item.value = value;
                result.append(item);
            }

            key = line.left(2);
            value = line.mid(6).trimmed();
        } else if (line.length() > 1)
            value += "\n" + line.trimmed();

        line = textStream.readLine();
    }
    if (!value.isEmpty()) {
        RISitem item;
        item.key = key;
        item.value = value;
        result.append(item);
    }

    return result;
}
