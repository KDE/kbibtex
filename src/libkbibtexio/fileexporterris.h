/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#ifndef BIBTEXFILEEXPORTERRIS_H
#define BIBTEXFILEEXPORTERRIS_H

#include <QTextStream>

#include <fileexporter.h>

class KBIBTEXIO_EXPORT FileExporterRIS : public FileExporter
{
public:
    FileExporterRIS();

    ~FileExporterRIS();

    bool save(QIODevice* iodevice, const Element* element, QStringList* errorLog = NULL);
    bool save(QIODevice* iodevice, const File* bibtexfile, QStringList* errorLog = NULL);

public slots:
    void cancel();

private:
    bool m_cancelFlag;

    bool writeEntry(QTextStream &stream, const Entry* entry, const File* bibtexfile = NULL);
    bool writeKeyValue(QTextStream &stream, const QString& key, const QString&value);
};

#endif
