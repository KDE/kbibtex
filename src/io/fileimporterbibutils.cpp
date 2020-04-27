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

#include "fileimporterbibutils.h"

#include <QBuffer>

#include <File>
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
    } else if (iodevice->atEnd() || iodevice->size() <= 0) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device at end or does not contain any data";
        emit message(MessageSeverity::Warning, QStringLiteral("Input device at end or does not contain any data"));
        return new File();
    }

    QBuffer buffer;
    const bool result = convert(*iodevice, format(), buffer, BibUtils::Format::BibTeX);
    iodevice->close();

    if (result)
        return d->bibtexImporter->load(&buffer);
    else
        return nullptr;
}
