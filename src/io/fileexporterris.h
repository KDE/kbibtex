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
#ifndef BIBTEXFILEEXPORTERRIS_H
#define BIBTEXFILEEXPORTERRIS_H

#include <QTextStream>

#include "fileexporter.h"

class Element;
class File;
class Entry;

class KBIBTEXIO_EXPORT FileExporterRIS : public FileExporter
{
    Q_OBJECT

public:
    FileExporterRIS();
    ~FileExporterRIS();

    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = NULL);

public slots:
    void cancel();

private:
    bool m_cancelFlag;

    bool writeEntry(QTextStream &stream, const Entry *entry);
    bool writeKeyValue(QTextStream &stream, const QString &key, const QString &value);
};

#endif
