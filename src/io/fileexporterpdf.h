/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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
#ifndef BIBTEXFILEEXPORTERPDF_H
#define BIBTEXFILEEXPORTERPDF_H

#include <QStringList>

#include "fileexportertoolchain.h"

/**
@author Thomas Fischer
*/
class KBIBTEXIO_EXPORT FileExporterPDF : public FileExporterToolchain
{
public:
    enum FileEmbedding { NoFileEmbedding = 0, EmbedBibTeXFile = 1, EmbedReferences = 2, EmbedBibTeXFileAndReferences = EmbedBibTeXFile | EmbedReferences};
    explicit FileExporterPDF(FileEmbedding fileEmbedding = FileExporterPDF::EmbedBibTeXFileAndReferences);
    ~FileExporterPDF();

    void reloadConfig();

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = NULL);

    void setDocumentSearchPaths(const QStringList &searchPaths);

private:
    QString m_fileBasename;
    QString m_fileStem;
    QString m_babelLanguage;
    QString m_paperSize;
    QString m_font;
    QString m_bibliographyStyle;
    FileEmbedding m_fileEmbedding;
    QStringList m_embeddedFileList;
    QStringList m_searchPaths;

    bool generatePDF(QIODevice *iodevice, QStringList *errorLog);
    bool writeLatexFile(const QString &filename);
    void fillEmbeddedFileList(const File *bibtexfile);
    void fillEmbeddedFileList(const QSharedPointer<const Element> element, const File *bibtexfile);
};

#endif
