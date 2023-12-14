/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileexporterris.h"

#include <QIODevice>
#include <QRegularExpression>
#include <QStringList>

#include <Entry>
#include <KBibTeX>
#include "fileexporter_p.h"
#include "logging_io.h"

class FileExporterRIS::Private
{
private:
    FileExporterRIS *parent;

public:
    Private(FileExporterRIS *p)
            : parent(p)
    {
        // nothing
    }

    bool writeEntry(QTextStream &stream, const QSharedPointer<const Entry> entry)
    {
        bool result = true;
        QString type = entry->type();

        if (type == Entry::etBook)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("BOOK"));
        else if (type == Entry::etInBook)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("CHAP"));
        else if (type == Entry::etInProceedings)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("CONF"));
        else if (type == Entry::etArticle)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("JOUR"));
        else if (type == Entry::etTechReport)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("RPRT"));
        else if (type == Entry::etPhDThesis || type == Entry::etMastersThesis)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("THES"));
        else if (type == Entry::etUnpublished)
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("UNPB"));
        else
            writeKeyValue(stream, QStringLiteral("TY"), QStringLiteral("GEN"));

        writeKeyValue(stream, QStringLiteral("ID"), entry->id());

        QString year, month;

        for (Entry::ConstIterator it = entry->constBegin(); result && it != entry->constEnd(); ++it) {
            const QString &key = it.key();
            const Value &value = it.value();

            if (key.startsWith(QStringLiteral("RISfield_")))
                result &= writeKeyValue(stream, key.right(2), PlainTextValue::text(value));
            else if (key == Entry::ftAuthor) {
                for (Value::ConstIterator it = value.constBegin(); result && it != value.constEnd(); ++it) {
                    QSharedPointer<const Person> person = (*it).dynamicCast<const Person>();
                    if (!person.isNull())
                        result &= writeKeyValue(stream, QStringLiteral("AU"), PlainTextValue::text(**it));
                    else
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot write value " << PlainTextValue::text(**it) << " for field AU (author), not supported by RIS format";
                }
            } else if (key.toLower() == Entry::ftEditor) {
                for (Value::ConstIterator it = value.constBegin(); result && it != value.constEnd(); ++it) {
                    QSharedPointer<const Person> person = (*it).dynamicCast<const Person>();
                    if (!person.isNull())
                        result &= writeKeyValue(stream, QStringLiteral("ED"), PlainTextValue::text(**it));
                    else
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot write value " << PlainTextValue::text(**it) << " for field ED (editor), not supported by RIS format";
                }
            } else if (key == Entry::ftTitle)
                result &= writeKeyValue(stream, QStringLiteral("TI"), PlainTextValue::text(value));
            else if (key == Entry::ftBookTitle)
                result &= writeKeyValue(stream, QStringLiteral("BT"), PlainTextValue::text(value));
            else if (key == Entry::ftSeries)
                result &= writeKeyValue(stream, QStringLiteral("T3"), PlainTextValue::text(value));
            else if (key == Entry::ftJournal)
                result &= writeKeyValue(stream, QStringLiteral("JO"), PlainTextValue::text(value)); ///< "JF" instead?
            else if (key == Entry::ftChapter)
                result &= writeKeyValue(stream, QStringLiteral("CP"), PlainTextValue::text(value));
            else if (key == Entry::ftISSN)
                result &= writeKeyValue(stream, QStringLiteral("SN"), PlainTextValue::text(value));
            else if (key == Entry::ftISBN)
                result &= writeKeyValue(stream, QStringLiteral("SN"), PlainTextValue::text(value));
            else if (key == Entry::ftSchool) /// == "institution"
                result &= writeKeyValue(stream, QStringLiteral("IN"), PlainTextValue::text(value));
            else if (key == Entry::ftVolume)
                result &= writeKeyValue(stream, QStringLiteral("VL"), PlainTextValue::text(value));
            else if (key == Entry::ftNumber) /// == "issue"
                result &= writeKeyValue(stream, QStringLiteral("IS"), PlainTextValue::text(value));
            else if (key == Entry::ftNote)
                result &= writeKeyValue(stream, QStringLiteral("N1"), PlainTextValue::text(value));
            else if (key == Entry::ftAbstract)
                result &= writeKeyValue(stream, QStringLiteral("N2"), PlainTextValue::text(value)); ///< "AB" instead?
            else if (key == Entry::ftPublisher)
                result &= writeKeyValue(stream, QStringLiteral("PB"), PlainTextValue::text(value));
            else if (key == Entry::ftLocation)
                result &= writeKeyValue(stream, QStringLiteral("CY"), PlainTextValue::text(value));
            else if (key == Entry::ftDOI)
                result &= writeKeyValue(stream, QStringLiteral("DO"), PlainTextValue::text(value));
            else if (key == Entry::ftKeywords)
                result &= writeKeyValue(stream, QStringLiteral("KW"), PlainTextValue::text(value));
            else if (key == Entry::ftYear)
                year = PlainTextValue::text(value);
            else if (key == Entry::ftMonth)
                month = PlainTextValue::text(value);
            else if (key == Entry::ftAddress)
                result &= writeKeyValue(stream, QStringLiteral("AD"), PlainTextValue::text(value));
            else if (key == Entry::ftUrl) {
                // FIXME one "UR" line per URL
                // FIXME for local files, use "L1"
                result &= writeKeyValue(stream, QStringLiteral("UR"), PlainTextValue::text(value));
            } else if (key == Entry::ftPages) {
                static const QRegularExpression splitRegExp(QString(QStringLiteral("-{1,2}|%1")).arg(QChar(0x2013)));
                QStringList pageRange = PlainTextValue::text(value).split(splitRegExp);
                if (pageRange.count() == 2) {
                    result &= writeKeyValue(stream, QStringLiteral("SP"), pageRange[ 0 ]);
                    result &= writeKeyValue(stream, QStringLiteral("EP"), pageRange[ 1 ]);
                }
            }
        }

        if (!year.isEmpty() || !month.isEmpty()) {
            int monthAsInt = -1;
            for (int i = 0; monthAsInt < 0 && i < 12; ++i)
                if (KBibTeX::MonthsTriple[i] == month)
                    monthAsInt = i + 1;
            if (monthAsInt > 0)
                result &= writeKeyValue(stream, QStringLiteral("PY"), QString(QStringLiteral("%1/%2//")).arg(year).arg(monthAsInt, 2, 10, QLatin1Char('0')));
            else
                result &= writeKeyValue(stream, QStringLiteral("PY"), QString(QStringLiteral("%1///%2")).arg(year, month));
        }

        result &= writeKeyValue(stream, QStringLiteral("ER"), QString());
#if QT_VERSION >= 0x050e00
        stream << Qt::endl;
#else // QT_VERSION < 0x050e00
        stream << endl;
#endif // QT_VERSION >= 0x050e00

        return result;
    }

    bool writeKeyValue(QTextStream &stream, const QString &key, const QString &value)
    {
        stream << key << "  - ";
        if (!value.isEmpty())
            stream << value;
#if QT_VERSION >= 0x050e00
        stream << Qt::endl;
#else // QT_VERSION < 0x050e00
        stream << endl;
#endif // QT_VERSION >= 0x050e00

        return true;
    }
};

FileExporterRIS::FileExporterRIS(QObject *parent)
        : FileExporter(parent), d(new Private(this))
{
    /// nothing
}

FileExporterRIS::~FileExporterRIS()
{
    delete d;
}

bool FileExporterRIS::save(QIODevice *iodevice, const QSharedPointer<const Element> &element, const File *bibtexfile)
{
    Q_UNUSED(bibtexfile)

    check_if_iodevice_invalid(iodevice);

    bool result = false;
    QTextStream stream(iodevice);

    const QSharedPointer<const Entry> &entry = element.dynamicCast<const Entry>();
    if (!entry.isNull())
        result = d->writeEntry(stream, entry);

    return result;
}

bool FileExporterRIS::save(QIODevice *iodevice, const File *bibtexfile)
{
    check_if_bibtexfile_or_iodevice_invalid(bibtexfile, iodevice);

    bool result = true;
    QTextStream stream(iodevice);

    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd() && result; ++it) {
        const QSharedPointer<const Entry> &entry = (*it).dynamicCast<Entry>();
        if (!entry.isNull()) {
            const QSharedPointer<const Entry> resolvedEntry(entry->resolveCrossref(bibtexfile));
            result |= d->writeEntry(stream, resolvedEntry);
        }
    }

    return result;
}
