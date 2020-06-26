/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileimporterris.h"

#include <QVector>
#include <QTextStream>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QStringList>

#include <Preferences>
#include <KBibTeX>
#include <Entry>
#include <Value>
#include "logging_io.h"

#define appendValue(entry, fieldname, newvalue) { Value value = (entry)->value((fieldname)); value.append((newvalue)); (entry)->insert((fieldname), value); }
#define removeDuplicates(entry, fieldname) { Value value = (entry)->value((fieldname)); if (!(value).isEmpty()) removeDuplicateValueItems((value)); if (!(value).isEmpty()) (entry)->insert((fieldname), value); }

class FileImporterRIS::FileImporterRISPrivate
{
public:
    FileImporterRIS *parent;
    int referenceCounter;
    bool cancelFlag;
    bool protectCasing;

    typedef struct {
        QString key;
        QString value;
    }
    RISitem;
    typedef QVector<RISitem> RISitemList;

    FileImporterRISPrivate(FileImporterRIS *_parent)
            : parent(_parent), referenceCounter(0), cancelFlag(false), protectCasing(false) {
        /// nothing
    }

    RISitemList readElement(QTextStream &textStream) {
        RISitemList result;
        QString line = textStream.readLine();
        while (!line.startsWith(QStringLiteral("TY  - ")) && !textStream.atEnd())
            line = textStream.readLine();
        if (textStream.atEnd())
            return result;

        QString key, value;
        while (!line.startsWith(QStringLiteral("ER  -")) && !textStream.atEnd()) {
            if (line.mid(2, 3) == QStringLiteral("  -")) {
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
                    value += QLatin1Char(' ') + line;
                }
            }

            line = textStream.readLine();
        }
        if (!line.startsWith(QStringLiteral("ER  -")) && textStream.atEnd()) {
            qCWarning(LOG_KBIBTEX_IO) << "Expected that entry that starts with 'TY' ends with 'ER' but instead met end of file";
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QStringLiteral("Expected that entry that starts with 'TY' ends with 'ER' but instead met end of file")));
        }
        if (!value.isEmpty()) {
            RISitem item;
            item.key = key;
            item.value = value;
            result.append(item);
        }

        return result;
    }

    inline QString optionallyProtectCasing(const QString &text) const {
        if (protectCasing)
            return QLatin1Char('{') + text + QLatin1Char('}');
        else
            return text;
    }

    Element *nextElement(QTextStream &textStream) {
        RISitemList list = readElement(textStream);
        if (list.empty())
            return nullptr;

        QString entryType = Entry::etMisc;
        Entry *entry = new Entry(entryType, QString(QStringLiteral("RIS_%1")).arg(referenceCounter++));
        QString journalName, startPage, endPage, date;
        int fieldCounter = 0;

        for (RISitemList::iterator it = list.begin(); it != list.end(); ++it) {
            if ((*it).key == QStringLiteral("TY")) {
                if ((*it).value.startsWith(QStringLiteral("BOOK")) || (*it).value.startsWith(QStringLiteral("SER")))
                    entryType = Entry::etBook;
                else if ((*it).value.startsWith(QStringLiteral("CHAP")))
                    entryType = Entry::etInBook;
                else if ((*it).value.startsWith(QStringLiteral("CONF")))
                    entryType = Entry::etInProceedings;
                else if ((*it).value.startsWith(QStringLiteral("JFULL")) || (*it).value.startsWith(QStringLiteral("JOUR")) || (*it).value.startsWith(QStringLiteral("MGZN")))
                    entryType = Entry::etArticle;
                else if ((*it).value.startsWith(QStringLiteral("RPRT")))
                    entryType = Entry::etTechReport;
                else if ((*it).value.startsWith(QStringLiteral("THES")))
                    entryType = Entry::etPhDThesis; // FIXME what about etMastersThesis?
                else if ((*it).value.startsWith(QStringLiteral("UNPB")))
                    entryType = Entry::etUnpublished;
                entry->setType(entryType);
            } else if ((*it).key == QStringLiteral("AU") || (*it).key == QStringLiteral("A1")) {
                Person *person = splitName((*it).value);
                if (person != nullptr)
                    appendValue(entry, Entry::ftAuthor, QSharedPointer<Person>(person));
            } else if ((*it).key == QStringLiteral("ED") || (*it).key == QStringLiteral("A2")) {
                Person *person = splitName((*it).value);
                if (person != nullptr)
                    appendValue(entry, Entry::ftEditor, QSharedPointer<Person>(person));
            } else if ((*it).key == QStringLiteral("ID")) {
                entry->setId((*it).value);
            } else if ((*it).key == QStringLiteral("Y1") || (*it).key == QStringLiteral("PY")) {
                date = (*it).value;
            } else if ((*it).key == QStringLiteral("Y2")) {
                if (date.isEmpty())
                    date = (*it).value;
            } else if ((*it).key == QStringLiteral("AB") || (*it).key == QStringLiteral("N2")) {
                appendValue(entry, Entry::ftAbstract, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("N1")) {
                appendValue(entry, Entry::ftNote, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("KW")) {
                QString text = (*it).value;
                const QRegularExpression splitRegExp(text.contains(QStringLiteral(";")) ? QStringLiteral("\\s*[;\\n]\\s*") : (text.contains(QStringLiteral(",")) ? QStringLiteral("\\s*[,\\n]\\s*") : QStringLiteral("\\n")));
#if QT_VERSION >= 0x050e00
                QStringList newKeywords = text.split(splitRegExp, Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
                QStringList newKeywords = text.split(splitRegExp, QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
                for (QStringList::Iterator it = newKeywords.begin(); it != newKeywords.end(); ++it)
                    appendValue(entry, Entry::ftKeywords, QSharedPointer<Keyword>(new Keyword(*it)));
            } else if ((*it).key == QStringLiteral("TI") || (*it).key == QStringLiteral("T1")) {
                appendValue(entry, Entry::ftTitle, QSharedPointer<PlainText>(new PlainText(optionallyProtectCasing((*it).value))));
            } else if ((*it).key == QStringLiteral("T3")) {
                appendValue(entry, Entry::ftSeries, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("JO") || (*it).key == QStringLiteral("J1") || (*it).key == QStringLiteral("J2")) {
                if (journalName.isEmpty())
                    journalName = (*it).value;
            } else if ((*it).key == QStringLiteral("JF") || (*it).key == QStringLiteral("JA")) {
                journalName = (*it).value;
            } else if ((*it).key == QStringLiteral("VL")) {
                appendValue(entry, Entry::ftVolume, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("CP")) {
                appendValue(entry, Entry::ftChapter, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("IS")) {
                appendValue(entry, Entry::ftNumber, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("DO") || (*it).key == QStringLiteral("M3")) {
                const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match((*it).value);
                if (doiRegExpMatch.hasMatch())
                    appendValue(entry, Entry::ftDOI, QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured())));
            } else if ((*it).key == QStringLiteral("PB")) {
                appendValue(entry, Entry::ftPublisher, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("IN")) {
                appendValue(entry, Entry::ftSchool, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("SN")) {
                const QString fieldName = entryType == Entry::etBook || entryType == Entry::etInBook ? Entry::ftISBN : Entry::ftISSN;
                appendValue(entry, fieldName, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("CY")) {
                appendValue(entry, Entry::ftLocation, QSharedPointer<PlainText>(new PlainText((*it).value)));
            }  else if ((*it).key == QStringLiteral("AD")) {
                appendValue(entry, Entry::ftAddress, QSharedPointer<PlainText>(new PlainText((*it).value)));
            } else if ((*it).key == QStringLiteral("L1") || (*it).key == QStringLiteral("L2") || (*it).key == QStringLiteral("L3") || (*it).key == QStringLiteral("UR")) {
                QString fieldValue = (*it).value;
                fieldValue.replace(QStringLiteral("<Go to ISI>://"), QStringLiteral("isi://"));
                const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(fieldValue);
                const QRegularExpressionMatch urlRegExpMatch = KBibTeX::urlRegExp.match(fieldValue);
                const QString fieldName = doiRegExpMatch.hasMatch() ? Entry::ftDOI : (KBibTeX::urlRegExp.match((*it).value).hasMatch() ? Entry::ftUrl : (Preferences::instance().bibliographySystem() == Preferences::BibliographySystem::BibTeX ? Entry::ftLocalFile : Entry::ftFile));
                fieldValue = doiRegExpMatch.hasMatch() ? doiRegExpMatch.captured() : (urlRegExpMatch.hasMatch() ? urlRegExpMatch.captured() : fieldValue);
                if (fieldValue.startsWith(QStringLiteral("file:///"))) fieldValue = fieldValue.mid(7);
                appendValue(entry, fieldName, QSharedPointer<VerbatimText>(new VerbatimText(fieldValue)));
            } else if ((*it).key == QStringLiteral("SP")) {
                startPage = (*it).value;
            } else if ((*it).key == QStringLiteral("EP")) {
                endPage = (*it).value;
            } else {
                const QString fieldName = QString(QStringLiteral("RISfield_%1_%2")).arg(fieldCounter++).arg((*it).key.left(2));
                appendValue(entry, fieldName, QSharedPointer<PlainText>(new PlainText((*it).value)));
            }
        }

        if (!journalName.isEmpty()) {
            const QString fieldName = entryType == Entry::etInBook || entryType == Entry::etInProceedings ? Entry::ftBookTitle : Entry::ftJournal;
            Value value = entry->value(fieldName);
            value.append(QSharedPointer<PlainText>(new PlainText(optionallyProtectCasing(journalName))));
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

#if QT_VERSION >= 0x050e00
        QStringList dateFragments = date.split(QStringLiteral("/"), Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
        QStringList dateFragments = date.split(QStringLiteral("/"), QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
        if (dateFragments.count() > 0) {
            bool ok;
            int year = dateFragments[0].toInt(&ok);
            if (ok && year > 1000 && year < 3000) {
                Value value = entry->value(Entry::ftYear);
                value.append(QSharedPointer<PlainText>(new PlainText(QString::number(year))));
                entry->insert(Entry::ftYear, value);
            } else {
                qCWarning(LOG_KBIBTEX_IO) << "Invalid year: " << dateFragments[0];
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Invalid year: '%1'")).arg(dateFragments[0])));
            }
        }
        if (dateFragments.count() > 1) {
            bool ok;
            int month = dateFragments[1].toInt(&ok);
            if (ok && month >= 1 && month <= 12) {
                Value value = entry->value(Entry::ftMonth);
                value.append(QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[month - 1])));
                entry->insert(Entry::ftMonth, value);
            } else {
                qCWarning(LOG_KBIBTEX_IO) << "Invalid month: " << dateFragments[1];
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(parent, "message", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(FileImporter::MessageSeverity, MessageSeverity::Warning), Q_ARG(QString, QString(QStringLiteral("Invalid month: '%1'")).arg(dateFragments[1])));
            }
        }

        removeDuplicates(entry, Entry::ftDOI);
        removeDuplicates(entry, Entry::ftUrl);

        return entry;
    }

    void removeDuplicateValueItems(Value &value) {
        if (value.count() < 2) return; /// Values with one or no ValueItem cannot have duplicates

        QSet<QString> uniqueStrings;
        for (Value::Iterator it = value.begin(); it != value.end();) {
            const QString itemString = PlainTextValue::text(*it);
            if (uniqueStrings.contains(itemString))
                it = value.erase(it);
            else {
                uniqueStrings.insert(itemString);
                ++it;
            }
        }
    }
};

FileImporterRIS::FileImporterRIS(QObject *parent)
        : FileImporter(parent), d(new FileImporterRISPrivate(this))
{
// nothing
}


FileImporterRIS::~FileImporterRIS()
{
    delete d;
}

File *FileImporterRIS::load(QIODevice *iodevice)
{
    if (!iodevice->isReadable() && !iodevice->open(QIODevice::ReadOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device not readable";
        emit message(MessageSeverity::Error, QStringLiteral("Input device not readable"));
        return nullptr;
    } else if (iodevice->atEnd() || iodevice->size() <= 0) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device at end or does not contain any data";
        emit message(MessageSeverity::Warning, QStringLiteral("Input device at end or does not contain any data"));
        return new File();
    }

    d->cancelFlag = false;
    d->referenceCounter = 0;
    QTextStream textStream(iodevice);

    File *result = new File();
    while (!d->cancelFlag && !textStream.atEnd()) {
        emit progress(textStream.pos(), iodevice->size());
        QCoreApplication::instance()->processEvents();
        Element *element = d->nextElement(textStream);
        if (element != nullptr)
            result->append(QSharedPointer<Element>(element));
        QCoreApplication::instance()->processEvents();
    }
    emit progress(100, 100);

    if (d->cancelFlag) {
        delete result;
        result = nullptr;
    }

    iodevice->close();

    if (result != nullptr)
        result->setProperty(File::ProtectCasing, static_cast<int>(d->protectCasing ? Qt::Checked : Qt::Unchecked));

    return result;
}

bool FileImporterRIS::guessCanDecode(const QString &text)
{
    return text.indexOf(QStringLiteral("TY  - ")) >= 0;
}

void FileImporterRIS::setProtectCasing(bool protectCasing)
{
    d->protectCasing = protectCasing;
}

void FileImporterRIS::cancel()
{
    d->cancelFlag = true;
}
