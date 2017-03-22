/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileexporterbibutils.h"

#include <QBuffer>

#include "fileexporterbibtex.h"
#include "logging_io.h"

class FileExporterBibUtils::Private
{
private:
    // UNUSED FileExporterBibUtils *p;

public:
    FileExporterBibTeX *bibtexExporter;

    Private(FileExporterBibUtils *parent)
    // UNUSED : p(parent)
    {
        bibtexExporter = new FileExporterBibTeX(parent);
        bibtexExporter->setEncoding(QStringLiteral("utf-8"));
    }

    ~Private() {
        delete bibtexExporter;
    }
};

FileExporterBibUtils::FileExporterBibUtils(QObject *parent)
        : FileExporter(parent), BibUtils(), d(new FileExporterBibUtils::Private(this))
{
    // TODO
}

FileExporterBibUtils::~FileExporterBibUtils()
{
    delete d;
}

bool FileExporterBibUtils::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    QBuffer buffer;
    bool result = d->bibtexExporter->save(&buffer, bibtexfile, errorLog);
    if (result)
        result = convert(buffer, BibUtils::BibTeX, *iodevice, format());

    iodevice->close();
    return result;
}

bool FileExporterBibUtils::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly))
        return false;

    QBuffer buffer;
    bool result = d->bibtexExporter->save(&buffer, element, bibtexfile, errorLog);
    if (result)
        result = convert(buffer, BibUtils::BibTeX, *iodevice, format());

    iodevice->close();
    return result;
}
