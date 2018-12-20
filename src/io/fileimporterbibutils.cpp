/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileimporterbibutils.h"

#include <QBuffer>

#include "fileimporterbibtex.h"
#include "logging_io.h"

class FileImporterBibUtils::Private
{
private:
    // UNUSED FileImporterBibUtils *p;

public:
    FileImporterBibTeX *bibtexImporter;

    Private(FileImporterBibUtils *parent)
    // UNUSED : p(parent)
    {
        bibtexImporter = new FileImporterBibTeX(parent);
        connect(bibtexImporter, &FileImporterBibTeX::message, parent, &FileImporterBibUtils::message);
    }

    ~Private() {
        delete bibtexImporter;
    }
};

FileImporterBibUtils::FileImporterBibUtils(QObject *parent)
        : FileImporter(parent), BibUtils(), d(new FileImporterBibUtils::Private(this))
{
    /// nothing
}

FileImporterBibUtils::~FileImporterBibUtils()
{
    delete d;
}

File *FileImporterBibUtils::load(QIODevice *iodevice)
{
    if (!iodevice->isReadable() && !iodevice->open(QIODevice::ReadOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device not readable";
        return nullptr;
    }

    QBuffer buffer;
    const bool result = convert(*iodevice, format(), buffer, BibUtils::BibTeX);
    iodevice->close();

    if (result)
        return d->bibtexImporter->load(&buffer);
    else
        return nullptr;
}
