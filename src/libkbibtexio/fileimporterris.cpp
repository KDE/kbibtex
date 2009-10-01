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
#include <QTextStream>
#include <QRegExp>
#include <QCoreApplication>
#include <QStringList>

#include <KDebug>

#include "entry.h"
#include "value.h"

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

    QString entryType = Entry::etMisc;
    Entry *entry = new Entry(entryType, QString("RIS_%1").arg(m_refNr++));
    QStringList authorList, editorList, keywordList;
    QString journalName, abstract, startPage, endPage, date;

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
            entry->setType(entryType);
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
            Value value = entry->value(Entry::ftNote);
            value.append(new PlainText((*it).value));
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
            Value value = entry->value(Entry::ftTitle);
            value.append(new PlainText((*it).value));
        } else if ((*it).key == "T3") {
            Value value = entry->value(Entry::ftSeries);
            value.append(new PlainText((*it).value));
        } else if ((*it).key == "JA" || (*it).key == "J1" || (*it).key == "J2") {
            if (journalName.isEmpty())
                journalName = (*it).value;
        } else if ((*it).key == "JF" || (*it).key == "JO") {
            journalName = (*it).value;
        } else if ((*it).key == "VL") {
            Value value = entry->value(Entry::ftVolume);
            value.append(new PlainText((*it).value));
        } else if ((*it).key == "CP") {
            Value value = entry->value(Entry::ftVolume);
            if (value.isEmpty())
                value = entry->value(Entry::ftChapter);
            value.append(new PlainText((*it).value));
        } else if ((*it).key == "IS") {
            Value value = entry->value(Entry::ftNumber);
            value.append(new PlainText((*it).value));
        } else if ((*it).key == "PB") {
            Value value = entry->value(Entry::ftPublisher);
            value.append(new PlainText((*it).value));
            /*} else if ((*it).key == "SN") {
                QString fieldType = entryType == Entry::etBook || entryType == Entry::etInBook ? Entry::ftISBN : Entry::ftISSN;
                Field * field = entry->getField(fieldType);
                if (field == NULL) {
                    field = new Field(fieldType);
                    entry->addField(field);
                }
                Value value;
                value.append(new PlainText((*it).value));
                field->setValue(value);
            } else if ((*it).key == "CY") {
                Field * field = entry->getField(Entry::ftLocation);
                if (field == NULL) {
                    field = new Field(Entry::ftLocation);
                    entry->addField(field);
                }
                Value value;
                value.append(new PlainText((*it).value));
                field->setValue(value);
            } else if ((*it).key == "AD") {
                Field * field = entry->getField(Entry::ftAddress);
                if (field == NULL) {
                    field = new Field(Entry::ftAddress);
                    entry->addField(field);
                }
                Value value;
                value.append(new PlainText((*it).value));
                field->setValue(value);
            } else if ((*it).key == "L1") {
                Field * field = entry->getField("PDF");
                if (field == NULL) {
                    field = new Field("PDF");
                    entry->addField(field);
                }
                Value value;
                value.append(new PlainText((*it).value));
                field->setValue(value);
            } else if ((*it).key == "UR") {
                Field * field = NULL;
                if ((*it).value.contains("dx.doi.org")) {
                    field = entry->getField("DOI");
                    if (field == NULL) {
                        field = new Field("DOI");
                        entry->addField(field);
                    }
                } else {
                    field = entry->getField(Entry::ftUrl);
                    if (field == NULL) {
                        field = new Field(Entry::ftUrl);
                        entry->addField(field);
                    }
                }
                Value value;
                value.append(new PlainText((*it).value));
                field->setValue(value);
                */
        } else if ((*it).key == "SP") {
            startPage = (*it).value;
        } else if ((*it).key == "EP") {
            endPage = (*it).value;
            /*} else {
                QString fieldName = QString("RISfield_%1_%2").arg(fieldNr++).arg((*it).key.left(2));
                Field * field = new Field(fieldName);
                entry->addField(field);
                Value value;
                value.append(new PlainText((*it).value));
                field->setValue(value);
            */
        }
    }


    if (!authorList.empty()) {
        Value value = entry->value(Entry::ftAuthor);
        for (QStringList::Iterator pit = authorList.begin(); pit != authorList.end(); ++pit) {
            Person *person = splitName(*pit);
            kDebug() << "pit=" << *pit << endl;
            kDebug() << "person=" << (person == NULL ? "NULL" : "ok") << endl;
            if (person != NULL)
                value.append(person);
        }
        kDebug() << "value size " << value.size() << endl;
        kDebug() << "Author 1 count= " << entry->value(KBibTeX::IO::Entry::ftAuthor).size() << "  " << entry->size() << endl;
        entry->insert(Entry::ftAuthor, value);
        kDebug() << "Author 2 count= " << entry->value(KBibTeX::IO::Entry::ftAuthor).size() << "  " << entry->size() << endl;
        for (Entry::Iterator it = entry->begin(); it != entry->end(); ++it) {
            kDebug() << "key = " << it.key() << endl;
        }
    }

    /*
    if (!editorList.empty()) {
        Field * field = entry->getField(Entry::ftEditor);
        if (field == NULL) {
            field = new Field(Entry::ftEditor);
            entry->addField(field);
        }
        Value value;
        for (QStringList::Iterator pit = authorList.begin(); pit != authorList.end(); ++pit) {
            Person *person = splitName(*pit);
            if (person != NULL)
                value.append(person);
        }
        field->setValue(value);
    }

    if (!keywordList.empty()) {
        Field * field = entry->getField(Entry::ftKeywords);
        if (field == NULL) {
            field = new Field(Entry::ftKeywords);
            entry->addField(field);
        }
        Value value;
        for (QStringList::Iterator pit = keywordList.begin(); pit != keywordList.end();++pit)
            value.append(new Keyword(*pit));
        field->setValue(value);
    }

    if (!journalName.isEmpty()) {
        Field * field = entry->getField(entryType == Entry::etInBook || entryType == Entry::etInProceedings ? Entry::ftBookTitle : Entry::ftJournal);
        if (field == NULL) {
            field = new Field(Entry::ftJournal);
            entry->addField(field);
        }
        Value value;
        value.append(new PlainText(journalName));
        field->setValue(value);
    }

    if (!abstract.isEmpty()) {
        Field * field = entry->getField(Entry::ftAbstract);
        if (field == NULL) {
            field = new Field(Entry::ftAbstract);
            entry->addField(field);
        }
        Value value;
        value.append(new PlainText(abstract));
        field->setValue(value);
    }
    */
    if (!startPage.isEmpty() || !endPage.isEmpty()) {
        QString page;
        if (startPage.isEmpty())
            page = endPage;
        else if (endPage.isEmpty())
            page = startPage;
        else
            page = startPage + QChar(0x2013) + endPage;

        Value value;
        value.append(new PlainText(page));
        entry->insert(Entry::ftPages, value);
    }


    QStringList dateFragments = date.split("/", QString::SkipEmptyParts);
    if (dateFragments.count() > 0) {
        bool ok;
        int year = dateFragments[ 0 ].toInt(&ok);
        if (ok && year > 1000 && year < 3000) {
            Value value = entry->value(Entry::ftYear);
            value.append(new PlainText(QString::number(year)));
            entry->insert(Entry::ftYear, value);
        }
    }
    if (dateFragments.count() > 1) {
        bool ok;
        int month = dateFragments[ 0 ].toInt(&ok);
        if (ok && month > 0 && month < 13) {
            Value value = entry->value(Entry::ftMonth);
            value.append(new PlainText(QString::number(month)));
            entry->insert(Entry::ftMonth, value);
        }
    }

    kDebug() << "Author count= " << entry->value(KBibTeX::IO::Entry::ftAuthor).size() << endl;

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
