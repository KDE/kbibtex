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
#ifndef BIBTEXFILEEXPORTERXML_H
#define BIBTEXFILEEXPORTERXML_H

#include <QTextStream>

#include "element.h"
#include "fileexporter.h"
#include "value.h"

class Entry;
class Macro;
class Comment;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileExporterXML : public FileExporter
{
public:
    FileExporterXML();
    ~FileExporterXML();

    bool save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice* iodevice, const QSharedPointer<const Element> element, QStringList *errorLog = NULL);

    static QString valueToXML(const Value& value, const QString& fieldType = QString::null);

public slots:
    void cancel();

private:
    bool m_cancelFlag;

    bool write(QTextStream&stream, const Element* element, const File* bibtexfile = NULL);
    bool writeEntry(QTextStream &stream, const Entry* entry);
    bool writeMacro(QTextStream &stream, const Macro* macro);
    bool writeComment(QTextStream &stream, const Comment* comment);

    static QString cleanXML(const QString &text);
};

#endif
