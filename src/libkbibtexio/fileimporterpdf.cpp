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
#include <QBuffer>
#include <QFile>

#include <KDebug>
#include <kio/netaccess.h>

#include <poppler-qt4.h>

#include "fileimporterpdf.h"
#include <file.h>
#include <fileimporterbibtex.h>

FileImporterPDF::FileImporterPDF()
{
    m_bibTeXimporter = new FileImporterBibTeX();
}

FileImporterPDF::~FileImporterPDF()
{
    delete m_bibTeXimporter;
}

File* FileImporterPDF::load(QIODevice *iodevice)
{
    m_cancelFlag = false;
    File* result = NULL;
    QByteArray buffer = iodevice->readAll();

    Poppler::Document *doc = Poppler::Document::loadFromData(buffer);
    if (doc == NULL) {
        kWarning() << "Could not load PDF document";
        return NULL;
    }

    if (doc->hasEmbeddedFiles()) {
        foreach(Poppler::EmbeddedFile *file, doc->embeddedFiles())
        if (file->name().endsWith(".bib")) {
            kDebug() << "filename is " << file->name();
            QByteArray data = file->data();
            QBuffer buffer(&data);
            FileImporterBibTeX bibTeXimporter;
            connect(&bibTeXimporter, SIGNAL(progress(int, int)), this, SIGNAL(progress(int, int)));
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
                if (file->name().endsWith(".bib")) {
                    result = true;
                    break;
                }
            delete doc;
        }
        KIO::NetAccess::removeTempFile(tmpFile);
    }

    return result;
}
