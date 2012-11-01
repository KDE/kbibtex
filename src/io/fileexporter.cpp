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
#include "fileexporter.h"

#include <QBuffer>
#include <QTextStream>

const QString FileExporter::keyPaperSize = QLatin1String("paperSize");
const QString FileExporter::defaultPaperSize = QLatin1String("a4");
const QString FileExporter::keyFont = QLatin1String("Font");
const QString FileExporter::defaultFont = QLatin1String("");

FileExporter::FileExporter() : QObject()
{
    // nothing
}

FileExporter::~FileExporter()
{
    // nothing
}

QString FileExporter::toString(const QSharedPointer<const Element> element, QStringList *errorLog)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    if (save(&buffer, element, errorLog)) {
        buffer.close();
        if (buffer.open(QBuffer::ReadOnly)) {
            QTextStream ts(&buffer);
            ts.setCodec("UTF-8");
            return ts.readAll();
        }
    }

    return QString::null;
}

QString FileExporter::toString(const File *bibtexfile, QStringList *errorLog)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    if (save(&buffer, bibtexfile, errorLog)) {
        buffer.close();
        if (buffer.open(QBuffer::ReadOnly)) {
            QTextStream ts(&buffer);
            ts.setCodec("utf-8");
            return ts.readAll();
        }
    }

    return QString::null;
}
