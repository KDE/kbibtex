/***************************************************************************
*   Copyright (C) 2004-2006 by Thomas Fischer                             *
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
#ifndef BIBTEXFILEEXPORTERPS_H
#define BIBTEXFILEEXPORTERPS_H

#include "fileexportertoolchain.h"

class QStringList;

/**
@author Thomas Fischer
*/
class KBIBTEXIO_EXPORT FileExporterPS : public FileExporterToolchain
{
public:
    FileExporterPS();
    ~FileExporterPS();

    void reloadConfig();

    bool save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice* iodevice, const QSharedPointer<const Element> element, QStringList *errorLog = NULL);

private:
    QString m_laTeXFilename;
    QString m_bibTeXFilename;
    QString m_outputFilename;
    QString m_babelLanguage;
    QString m_paperSize;
    QString m_font;
    QString m_bibliographyStyle;

    bool generatePS(QIODevice* iodevice, QStringList *errorLog);
    bool writeLatexFile(const QString &filename);
    bool beautifyPostscriptFile(const QString &filename, const QString &title);
};


#endif
