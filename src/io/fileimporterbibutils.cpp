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

#include "fileimporterbibutils.h"

#include <QBuffer>
#include <QDebug>

#include "fileimporterbibtex.h"

class FileImporterBibUtils::Private
{
private:
    // UNUSED FileImporterBibUtils *p;

public:
    FileImporterBibTeX bibtexImporter;

    Private(FileImporterBibUtils */* UNUSED parent*/)
    // UNUSED : p(parent)
    {
        /// nothing
    }
};

FileImporterBibUtils::FileImporterBibUtils()
        : FileImporter(), BibUtils(), d(new FileImporterBibUtils::Private(this))
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
        qWarning() << "Input device not readable";
        return NULL;
    }

    QBuffer buffer;
    const bool result = convert(*iodevice, format(), buffer, BibUtils::BibTeX);
    iodevice->close();

    if (result)
        return d->bibtexImporter.load(&buffer);
    else
        return NULL;
}
