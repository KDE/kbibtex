/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileimporterpdf.h"

#include <QBuffer>
#include <QFile>

#include <KDebug>
#include <kio/netaccess.h>

#include <poppler-qt4.h>

#include "file.h"
#include "fileimporterbibtex.h"

FileImporterPDF::FileImporterPDF()
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
        kDebug() << "Input device not readable";
        return NULL;
    }

    m_cancelFlag = false;
    File *result = NULL;
    QByteArray buffer = iodevice->readAll();

    Poppler::Document *doc = Poppler::Document::loadFromData(buffer);
    if (doc == NULL) {
        kWarning() << "Could not load PDF document";
        iodevice->close();
        return NULL;
    }

    if (doc->hasEmbeddedFiles()) {
        foreach(Poppler::EmbeddedFile *file, doc->embeddedFiles())
        if (file->name().endsWith(QLatin1String(".bib"))) {
            kDebug() << "filename is " << file->name();
            QByteArray data = file->data();
            QBuffer buffer(&data);
            FileImporterBibTeX bibTeXimporter;
            connect(&bibTeXimporter, SIGNAL(progress(int,int)), this, SIGNAL(progress(int,int)));
            buffer.open(QIODevice::ReadOnly);
            result = bibTeXimporter.load(&buffer);
            buffer.close();

            if (result)
                kDebug() << "result = " << result->count() << "  " << data.size() << "  " << buffer.size();
            else
                kDebug() << "result is empty";
            break;
        }
    }

    delete doc;
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

bool FileImporterPDF::containsBibTeXData(const KUrl &url)
{
    bool result = false;

    QString tmpFile;
    if (KIO::NetAccess::download(url, tmpFile, NULL)) {
        Poppler::Document *doc = Poppler::Document::load(tmpFile);
        if (doc != NULL) {
            if (doc->hasEmbeddedFiles())
                foreach(Poppler::EmbeddedFile *file, doc->embeddedFiles())
                if (file->name().endsWith(QLatin1String(".bib"))) {
                    result = true;
                    break;
                }
            delete doc;
        }
        KIO::NetAccess::removeTempFile(tmpFile);
    }

    return result;
}
