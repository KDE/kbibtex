/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileexporterwordbibxml.h"

#include <QIODevice>
#include <QTextStream>
#include <QRegularExpression>

#include <File>
#include <Entry>
#include "encoderxml.h"
#include "logging_io.h"

class FileExporterWordBibXML::Private
{
public:
    bool cancelFlag;

    Private(FileExporterWordBibXML *)
        : cancelFlag(false)
    {
        // nothing
    }

    ~Private() {
        // nothing
    }

    bool insideProtectiveCurleyBrackets(const QString &input) {
        if (input.length() < 3 || input[0] != QLatin1Char('{') || input[input.length() - 1] != QLatin1Char('}'))
            return false;

        int depth = 0;
        QChar prev;
        for (const QChar &c : input) {
            if (c == QLatin1Char('{') && prev != QLatin1Char('\\'))
                ++depth;
            else if (c == QLatin1Char('}') && prev != QLatin1Char('\\'))
                --depth;
            prev = c;
        }

        return depth == 0;
    }

    QString removeUnwantedChars(const QString &input) {
        QString result;
        result.reserve(input.length());
        static const QSet<QChar> skip{QLatin1Char('{'), QLatin1Char('}'), QLatin1Char('<'), QLatin1Char('>'), QLatin1Char('&')};
        static const QHash<QChar, QString> replace{{QLatin1Char('~'), QStringLiteral(" ")}};
        for (const QChar &c : input)
            if (skip.contains(c))
                continue;
            else if (replace.contains(c))
                result.append(replace[c]);
            else
                result.append(c);
        return result;
    }

    QString entryTypeToSourceType(const QString &entryType) {
        if (entryType == Entry::etBook)
            return QStringLiteral("Book");
        else if (entryType == Entry::etInBook || entryType == QStringLiteral("incollection"))
            return QStringLiteral("BookSection");
        else if (entryType == Entry::etArticle)
            return QStringLiteral("JournalArticle");
        else if (entryType == Entry::etInProceedings || entryType == Entry::etProceedings || entryType == QStringLiteral("conference"))
            return QStringLiteral("ConferenceProceedings");
        else if (entryType == Entry::etUnpublished || entryType == Entry::etMastersThesis || entryType == Entry::etPhDThesis || entryType == Entry::etTechReport || entryType == QStringLiteral("manual"))
            return QStringLiteral("Report");
        else if (entryType == Entry::etMisc)
            return QStringLiteral("Misc");
        else {
            qCDebug(LOG_KBIBTEX_IO) << "Unsupported entry type:" << entryType;
            return QStringLiteral("Misc");
        }
    }

    QString fieldTypeToXMLkey(const QString &fieldType) {
        if (fieldType == Entry::ftTitle)
            return QStringLiteral("Title");
        else if (fieldType == Entry::ftPublisher)
            return QStringLiteral("Publisher");
        else if (fieldType == Entry::ftJournal || fieldType == QStringLiteral("journaltitle"))
            return QStringLiteral("JournalName");
        else if (fieldType == Entry::ftVolume)
            return QStringLiteral("Volume");
        else if (fieldType == Entry::ftNote)
            return QStringLiteral("Comments");
        else if (fieldType == Entry::ftEdition)
            return QStringLiteral("Edititon");
        else if (fieldType == Entry::ftBookTitle)
            return QStringLiteral("BookTitle");
        else if (fieldType == Entry::ftChapter)
            return QStringLiteral("ChapterNumber");
        else if (fieldType == Entry::ftNumber)
            return QStringLiteral("Issue");
        else if (fieldType == Entry::ftSchool)
            return QStringLiteral("Department");
        else if (fieldType == Entry::ftDOI)
            return QStringLiteral("DOI");
        else if (fieldType == Entry::ftUrl)
            return QStringLiteral("URL");
        else if (fieldType == Entry::ftPages)
            return QStringLiteral("Pages");
        else if (fieldType == Entry::ftLocation)
            return QStringLiteral("City");
        else {
            qCDebug(LOG_KBIBTEX_IO) << "Unsupported field type:" << fieldType;
            return QString();
        }
    }


    bool writeEntry(QTextStream &stream, const QSharedPointer<const Entry> &entry)
    {
        // Documentation of Word XML Bibliography:
        //  - https://docs.jabref.org/advanced/knowledge/msofficebibfieldmapping
        stream << "<b:Source><b:Tag>" << EncoderXML::instance().encode(entry->id(), Encoder::TargetEncoding::UTF8) << "</b:Tag><b:SourceType>" << entryTypeToSourceType(entry->type().toLower()) << "</b:SourceType>";

        static const QSet<QString> standardNumberKeys{Entry::ftISBN, Entry::ftISSN, QStringLiteral("lccn")};
        QString standardNumber;

        // Authors and editors are grouped
        static const QHash<QString, QString> personFields{{QStringLiteral("Author"), Entry::ftAuthor}, {QStringLiteral("Editor"), Entry::ftEditor}, {QStringLiteral("Translator"), QStringLiteral("translator")}, {QStringLiteral("BookAuthor"), QStringLiteral("bookauthor")}};
        if (entry->contains(Entry::ftAuthor) || entry->contains(Entry::ftEditor)) {
            stream << "<b:Author>";
            for (auto it = personFields.constBegin(); it != personFields.constEnd(); ++it) {
                if (entry->contains(it.value())) {
                    stream << "<b:" << it.key() << ">";

                    bool nameListOpened = false;
                    const Value value = entry->value(it.value());
                    for (const auto &valueItem : value) {
                        const QSharedPointer<const Person> p = valueItem.dynamicCast<const Person>();
                        if (!p.isNull()) {
                            if (!nameListOpened && p->firstName().isEmpty() && insideProtectiveCurleyBrackets(p->lastName())) {
                                // Person's last name looks like  {KDE e.V.}  so treat as organization name instead of a person's name
                                stream << "<b:Corporate>" << removeUnwantedChars(p->lastName()) << "</b:Corporate>";
                                break; //< only one corporate, nothing more
                            } else {
                                if (!nameListOpened) {
                                    stream << "<b:NameList>";
                                    nameListOpened = true;
                                }
                                stream << "<b:Person><b:Last>" << removeUnwantedChars(p->lastName()) << "</b:Last><b:First>" << removeUnwantedChars(p->firstName()) << "</b:First></b:Person>";
                            }
                        } else {
                            qCWarning(LOG_KBIBTEX_IO) << it.value() << "field contains something else than a Person:" << PlainTextValue::text(value);
                        }
                    }

                    if (nameListOpened)
                        stream << "</b:NameList>";
                    stream << "</b:" << it.key() << ">";
                }
            }
            stream << "</b:Author>";
        }

        for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
            const QString &key = it.key().toLower();
            if (personFields.values().contains(key)) {
                // Authors, editors, etc. were processed above
                continue;
            }
            const Value &value = it.value();

            static const QSet<QString> fieldsKeptAsIs{Entry::ftTitle, Entry::ftPublisher, Entry::ftJournal, Entry::ftVolume, Entry::ftNote, Entry::ftEdition, Entry::ftBookTitle, Entry::ftChapter, Entry::ftNumber, Entry::ftSchool, Entry::ftDOI, Entry::ftUrl, Entry::ftPages, Entry::ftLocation};
            static const QSet<QString> ignoredFields{Entry::ftAbstract, Entry::ftLocalFile, Entry::ftSeries, Entry::ftKeywords, Entry::ftCrossRef, Entry::ftAddress, QStringLiteral("acmid"), QStringLiteral("articleno"), QStringLiteral("numpages"), QStringLiteral("added-at"), QStringLiteral("biburl"), QStringLiteral("organization"), QStringLiteral("ee"), QStringLiteral("interhash"), QStringLiteral("intrahash"), QStringLiteral("howpublished"), QStringLiteral("key"), QStringLiteral("type"), QStringLiteral("institution"), QStringLiteral("issue"), QStringLiteral("eprint"), QStringLiteral("affiliation"), QStringLiteral("keyword"), QStringLiteral("urldate"), QStringLiteral("date"), QStringLiteral("shortauthor")};

            if (ignoredFields.contains(key) || key.startsWith(QStringLiteral("x-"))) {
                // qCDebug(LOG_KBIBTEX_IO) << "Ignoring field" << key << "for entry" << entry->id();
            } else if (key == Entry::ftYear) {
                const QString textualRepresentation = PlainTextValue::text(value);
                static const QRegularExpression yearRegExp(QStringLiteral("\\b(1[2-9]|2[01])\\d{2}\\b"));
                const auto m = yearRegExp.match(textualRepresentation);
                if (m.hasMatch())
                    stream << "<b:Year>" << m.captured() << "</b:Year>";
            } else if (standardNumberKeys.contains(key)) {
                standardNumber = PlainTextValue::text(value);
            } else if (key == Entry::ftMonth) {
                // TODO
            } else if (fieldsKeptAsIs.contains(key)) {
                const QString xmlKey{fieldTypeToXMLkey(key)};
                const QString textualRepresentation{removeUnwantedChars(PlainTextValue::text(value))};
                stream << "<b:" << xmlKey << ">" << textualRepresentation << "</b:" << xmlKey << ">";
            } else {
                qCDebug(LOG_KBIBTEX_IO) << "Field not supported by Word XML exporter:" << key;
            }
        }

        if (!standardNumber.isEmpty()) {
            stream << "<b:StandardNumber>" << removeUnwantedChars(standardNumber) << "</b:StandardNumber>";
        }

        stream << "</b:Source>";

        return true;
    }


    bool write(QTextStream &stream, const QSharedPointer<const Element> &element, const File *bibtexfile = nullptr) {
        bool result = false;

        const QSharedPointer<const Entry> &entry = element.dynamicCast<const Entry>();
        if (!entry.isNull()) {
            if (bibtexfile == nullptr)
                result |= writeEntry(stream, entry);
            else {
                const QSharedPointer<const Entry> resolvedEntry(entry->resolveCrossref(bibtexfile));
                result |= writeEntry(stream, resolvedEntry);
            }
        } else {
            // not (yet) supported
            return true;
        }

        return result;
    }
};

FileExporterWordBibXML::FileExporterWordBibXML(QObject *parent)
    : FileExporter(parent), d(new FileExporterWordBibXML::Private(this))
{
    /// nothing
}

FileExporterWordBibXML::~FileExporterWordBibXML()
{
    delete d;
}

bool FileExporterWordBibXML::save(QIODevice *iodevice, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = true;
    d->cancelFlag = false;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

#if QT_VERSION >= 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Qt::endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << Qt::endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << Qt::endl;
    stream << "<b:Sources xmlns:b=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\" xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\">" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << endl;
    stream << "<b:Sources xmlns:b=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\" xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\">" << endl;
#endif // QT_VERSION >= 0x050e00

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result && !d->cancelFlag; ++it)
        result &= d->write(stream, *it, bibtexfile);

#if QT_VERSION >= 0x050e00
    stream << "</b:Sources>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "</b:Sources>" << endl;
#endif // QT_VERSION >= 0x050e00

    return result && !d->cancelFlag;
}

bool FileExporterWordBibXML::save(QIODevice *iodevice, const QSharedPointer<const Element> &element, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    d->cancelFlag = false;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

#if QT_VERSION >= 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Qt::endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << Qt::endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << Qt::endl;
    stream << "<b:Sources xmlns:b=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\" xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\">" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX -->" << endl;
    stream << "<!-- https://userbase.kde.org/KBibTeX -->" << endl;
    stream << "<b:Sources xmlns:b=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\" xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/bibliography\">" << endl;
#endif // QT_VERSION >= 0x050e00

    const bool result = d->write(stream, element, bibtexfile);

#if QT_VERSION >= 0x050e00
    stream << "</b:Sources>" << Qt::endl;
#else // QT_VERSION < 0x050e00
    stream << "</b:Sources>" << endl;
#endif // QT_VERSION >= 0x050e00

    return result && !d->cancelFlag;
}

void FileExporterWordBibXML::cancel()
{
    d->cancelFlag = true;
}

