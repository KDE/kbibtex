/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <QVector>
#include <QTextStream>
#include <QRegExp>
#include <QCoreApplication>
#include <QStringList>

#include <KDebug>

#include <kbibtexnamespace.h>
#include "entry.h"
#include "value.h"

#include "fileimporterris.h"

#define appendValue(entry, fieldname, newvalue) { Value value = (entry)->value((fieldname)); value.append((newvalue)); (entry)->insert((fieldname), value); }

class FileImporterRIS::FileImporterRISPrivate
{
private:
    FileImporterRIS *p;

public:
    int referenceCounter;
    bool cancelFlag;

    typedef struct {
        QString key;
        QString value;
    }
    RISitem;
    typedef QVector<RISitem> RISitemList;

    FileImporterRISPrivate(FileImporterRIS *parent)
            : p(parent), referenceCounter(0), cancelFlag(false) {
        // nothing
    }

    RISitemList readElement(QTextStream &textStream) {
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
                value = line.mid(6).simplified();
            } else {
                line = line.simplified();
                if (line.length() > 1) {
                    /// multi-line field are joined to one long line
                    value += ' ' + line;
                }
            }

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

    Element *nextElement(QTextStream &textStream) {
        RISitemList list = readElement(textStream);
        if (list.empty())
            return NULL;

        QString entryType = Entry::etMisc;
        Entry *entry = new Entry(entryType, QString("RIS_%1").arg(referenceCounter++));
        QString journalName, startPage, endPage, date;
        int fieldCounter = 0;

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
                else if ((*it).value.startsWith("UNPB"))
                    entryType = Entry::etUnpublished;
                entry->setType(entryType);
            } else if ((*it).key == "AU" || (*it).key == "A1") {
                Person *person = splitName((*it).value);
                if (person != NULL)
                    appendValue(entry, Entry::ftAuthor, QSharedPointer<Person>(person));
            } else if ((*it).key == "ED" || (*it).key == "A2") {
                Person *person = splitName((*it).value);
                if (person != NULL)
                    appendValue(entry, Entry::ftEditor, QSharedPointer<Person>(person));
            } else if ((*it).key == "ID") {
                entry->setId((*it).value);
            } else if ((*it).key == "Y1" || (*it).key == "PY") {
                date = (*it).value;
            } else if ((*it).key == "Y2") {
                if (date.isEmpty())
                    date = (*it).value;
            } else if ((*it).key == "AB" || (*it).key == "N2") {
                appendValue(entry, Entry::ftAbstract, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "N1") {
                appendValue(entry, Entry::ftNote, QSharedPointer<PlainText>(new PlainText((*it).value)));
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
                    appendValue(entry, Entry::ftKeywords, QSharedPointer<Keyword>(new Keyword(*it)));
            } else if ((*it).key == "TI" || (*it).key == "T1") {
                appendValue(entry, Entry::ftTitle, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "T3") {
                appendValue(entry, Entry::ftSeries, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "JO" || (*it).key == "J1" || (*it).key == "J2") {
                if (journalName.isEmpty())
                    journalName = (*it).value;
            } else if ((*it).key == "JF" || (*it).key == "JA") {
                journalName = (*it).value;
            } else if ((*it).key == "VL") {
                appendValue(entry, Entry::ftVolume, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "CP") {
                appendValue(entry, Entry::ftChapter, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "IS") {
                appendValue(entry, Entry::ftNumber, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "PB") {
                appendValue(entry, Entry::ftPublisher, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "SN") {
                const QString fieldName = entryType == Entry::etBook || entryType == Entry::etInBook ? Entry::ftISBN : Entry::ftISSN;
                appendValue(entry, fieldName, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "CY") {
                appendValue(entry, Entry::ftLocation, QSharedPointer<PlainText>(new PlainText((*it).value)));
            }  else if ((*it).key == "AD") {
                appendValue(entry, Entry::ftAddress, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "L1" || (*it).key == "L2" || (*it).key == "L3" || (*it).key == "UR") {
                const QString fieldName = KBibTeX::doiRegExp.indexIn((*it).value) >= 0 ? Entry::ftDOI : Entry::ftUrl;
                appendValue(entry, fieldName, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == "SP") {
                startPage = (*it).value;
            } else if ((*it).key == "EP") {
                endPage = (*it).value;
            } else {
                const QString fieldName = QString("RISfield_%1_%2").arg(fieldCounter++).arg((*it).key.left(2));
                appendValue(entry, fieldName, QSharedPointer<PlainText>(new PlainText((*it).value)));
            }
        }

        if (!journalName.isEmpty()) {
            const QString fieldName = entryType == Entry::etInBook || entryType == Entry::etInProceedings ? Entry::ftBookTitle : Entry::ftJournal;
            Value value = entry->value(fieldName);
            value.append(QSharedPointer<PlainText>(new PlainText(journalName)));
            entry->insert(fieldName, value);
        }

        if (!startPage.isEmpty() || !endPage.isEmpty()) {
            QString page;
            if (startPage.isEmpty())
                page = endPage;
            else if (endPage.isEmpty())
                page = startPage;
            else
                page = startPage + QChar(0x2013) + endPage;

            Value value;
            value.append(QSharedPointer<PlainText>(new PlainText(page)));
            entry->insert(Entry::ftPages, value);
        }

        QStringList dateFragments = date.split("/", QString::SkipEmptyParts);
        if (dateFragments.count() > 0) {
            bool ok;
            int year = dateFragments[0].toInt(&ok);
            if (ok && year > 1000 && year < 3000) {
                Value value = entry->value(Entry::ftYear);
                value.append(QSharedPointer<PlainText>(new PlainText(QString::number(year))));
                entry->insert(Entry::ftYear, value);
            } else
                kDebug() << "invalid year: " << year;
        }
        if (dateFragments.count() > 1) {
            bool ok;
            int month = dateFragments[1].toInt(&ok);
            if (ok && month > 0 && month < 13) {
                Value value = entry->value(Entry::ftMonth);
                value.append(QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[month-1])));
                entry->insert(Entry::ftMonth, value);
            } else
                kDebug() << "invalid month: " << month;
        }

        return entry;
    }

};

FileImporterRIS::FileImporterRIS()
        : FileImporter(), d(new FileImporterRISPrivate(this))
{
// nothing
}


FileImporterRIS::~FileImporterRIS()
{
// nothing
}

File* FileImporterRIS::load(QIODevice *iodevice)
{
    d->cancelFlag = false;
    d->referenceCounter = 0;
    QTextStream textStream(iodevice);

    File *result = new File();
    while (!d->cancelFlag && !textStream.atEnd()) {
        emit progress(textStream.pos(), iodevice->size());
        QCoreApplication::instance()->processEvents();
        Element * element = d->nextElement(textStream);
        if (element != NULL)
            result->append(element);
        QCoreApplication::instance()->processEvents();
    }
    emit progress(100, 100);

    if (d->cancelFlag) {
        delete result;
        result = NULL;
    }

    return result;
}

bool FileImporterRIS::guessCanDecode(const QString & text)
{
    return text.indexOf("TY  - ") >= 0;
}

void FileImporterRIS::cancel()
{
    d->cancelFlag = true;
}
