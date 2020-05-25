/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_IO_FILEEXPORTERPDF_H
#define KBIBTEX_IO_FILEEXPORTERPDF_H

#include <QStringList>

#include <FileExporterToolchain>

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterPDF : public FileExporterToolchain
{
    Q_OBJECT

public:
    enum class FileEmbedding {
        None = 0,
        BibTeXFile = 1,
        References = 2,
        BibTeXFileAndReferences = BibTeXFile | References
    };
    Q_DECLARE_FLAGS(FileEmbeddings, FileEmbedding)

    explicit FileExporterPDF(QObject *parent);
    ~FileExporterPDF() override;

    bool save(QIODevice *iodevice, const File *bibtexfile) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile) override;

    void setDocumentSearchPaths(const QStringList &searchPaths);
    void setFileEmbedding(const FileEmbeddings fileEmbedding);

private:
    QString m_fileBasename;
    QString m_fileStem;
    FileEmbeddings m_fileEmbeddings;
    QStringList m_embeddedFileList;
    QStringList m_searchPaths;

    bool generatePDF(QIODevice *iodevice);
    bool writeLatexFile(const QString &filename);
    void fillEmbeddedFileList(const File *bibtexfile);
    void fillEmbeddedFileList(const QSharedPointer<const Element> element, const File *bibtexfile);
};

#endif // KBIBTEX_IO_FILEEXPORTERPDF_H
