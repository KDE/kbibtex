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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "fileimporterpdf.h"

#include <QBuffer>
#include <QFile>

// FIXME #include <poppler-qt4.h>

#include "file.h"
#include "fileimporterbibtex.h"
#include "logging_io.h"

FileImporterPDF::FileImporterPDF()
        : FileImporter(), m_cancelFlag(false)
{
    m_bibTeXimporter = new FileImporterBibTeX();
}

FileImporterPDF::~FileImporterPDF()
{
    delete m_bibTeXimporter;
}

File *FileImporterPDF::load(QIODevice *iodevice)
{
    if (!iodevice->isReadable() && !iodevice->open(QIODevice::ReadOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Input device not readable";
        return NULL;
    }

    m_cancelFlag = false;
    File *result = NULL;
    QByteArray buffer = iodevice->readAll();

    // FIXME
    /*
    Poppler::Document *doc = Poppler::Document::loadFromData(buffer);
    if (doc == NULL) {
        qCWarning(LOG_KBIBTEX_IO) << "Could not load PDF document";
        iodevice->close();
        return NULL;
    }

    if (doc->hasEmbeddedFiles()) {
        foreach (Poppler::EmbeddedFile *file, doc->embeddedFiles())
        if (file->name().endsWith(QStringLiteral(".bib"))) {
            QByteArray data = file->data();
            QBuffer buffer(&data);
            FileImporterBibTeX bibTeXimporter;
            connect(&bibTeXimporter, &FileImporter::progress, this, &FileImporter::progress);
            buffer.open(QIODevice::ReadOnly);
            result = bibTeXimporter.load(&buffer);
            buffer.close();

            if (result)
                qCDebug(LOG_KBIBTEX_IO) << "result = " << result->count() << "  " << data.size() << "  " << buffer.size();
            else
                qCDebug(LOG_KBIBTEX_IO) << "result is empty";
            break;
        }
    }

    delete doc;
    */
    iodevice->close();
    return result;
}

bool FileImporterPDF::guessCanDecode(const QString &)
{
    return false;
}

void FileImporterPDF::cancel()
{
    m_cancelFlag = true;
    m_bibTeXimporter->cancel();
}
