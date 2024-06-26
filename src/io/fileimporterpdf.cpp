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

#ifdef HAVE_POPPLERQT

#include "fileimporterpdf.h"

#include <QBuffer>
#include <QFile>

#ifdef HAVE_POPPLERQT5
#include <poppler-qt5.h>
#else // not HAVE_POPPLERQT5
#ifdef HAVE_POPPLERQT6
#include <poppler-qt6.h>
#endif // HAVE_POPPLERQT6
#endif // HAVE_POPPLERQT5

#include <File>
#include "fileimporterbibtex.h"
#include "fileimporter_p.h"
#include "logging_io.h"

FileImporterPDF::FileImporterPDF(QObject *parent)
        : FileImporter(parent), m_cancelFlag(false)
{
    m_bibTeXimporter = new FileImporterBibTeX(this);
    connect(m_bibTeXimporter, &FileImporterBibTeX::message, this, &FileImporterPDF::message);
}

FileImporterPDF::~FileImporterPDF()
{
    delete m_bibTeXimporter;
}

File *FileImporterPDF::load(QIODevice *iodevice)
{
    check_if_iodevice_invalid(iodevice);

    m_cancelFlag = false;
    File *result = nullptr;
    QByteArray buffer = iodevice->readAll();

#ifdef HAVE_POPPLERQT5
    QScopedPointer<Poppler::Document> doc(Poppler::Document::loadFromData(buffer));
#else // not HAVE_POPPLERQT5
#ifdef HAVE_POPPLERQT6
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::loadFromData(buffer);
#endif // HAVE_POPPLERQT6
#endif // HAVE_POPPLERQT5
    if (!doc) {
        qCWarning(LOG_KBIBTEX_IO) << "Could not load PDF document";
        iodevice->close();
        return nullptr;
    }

    /// Iterate through all files embedded in this PDF file (if any),
    /// check for file extension '.bib', and try to load bibliography
    /// data.
    if (doc->hasEmbeddedFiles()) {
        const QList<Poppler::EmbeddedFile *> embeddedFiles = doc->embeddedFiles();
        for (Poppler::EmbeddedFile *file : embeddedFiles) {
            if (file->name().endsWith(QStringLiteral(".bib"))) {
                // TODO maybe request implementation of a constData() for
                // Poppler::EmbeddedFile to operate on const objects?
                QByteArray data(file->data());
                QBuffer buffer(&data);
                FileImporterBibTeX bibTeXimporter(this);
                connect(&bibTeXimporter, &FileImporter::progress, this, &FileImporter::progress);
                buffer.open(QIODevice::ReadOnly);
                result = bibTeXimporter.load(&buffer);
                buffer.close();

                if (result) {
                    qCDebug(LOG_KBIBTEX_IO) << "Bibliography extracted from embedded file" << file->name() << "has" << result->count() << "entries";
                    if (result->count() > 0)
                        break; ///< stop processing after first valid, non-empty BibTeX file
                    else {
                        /// ... otherwise delete empty bibliography object
                        delete result;
                        result = nullptr;
                    }
                } else
                    qCDebug(LOG_KBIBTEX_IO) << "Create bibliography file from embedded file" << file->name() << "failed";
            } else
                qCDebug(LOG_KBIBTEX_IO) << "Embedded file" << file->name() << "doesn't have right extension ('.bib')";
        }
    } else
        qCDebug(LOG_KBIBTEX_IO) << "PDF document has no files embedded";

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

#endif // HAVE_POPPLERQT
