/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileexporter.h"

#include <QBuffer>
#include <QTextStream>

const QString FileExporter::keyPaperSize = QStringLiteral("paperSize");
const QString FileExporter::defaultPaperSize = QStringLiteral("a4");
const QString FileExporter::keyFont = QStringLiteral("Font");
const QString FileExporter::defaultFont = QStringLiteral("");

FileExporter::FileExporter(QObject *parent)
        : QObject(parent)
{
    /// nothing
}

FileExporter::~FileExporter()
{
    /// nothing
}

QString FileExporter::toString(const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    if (save(&buffer, element, bibtexfile, errorLog)) {
        buffer.close();
        if (buffer.open(QBuffer::ReadOnly)) {
            QTextStream ts(&buffer);
            ts.setCodec("UTF-8");
            return ts.readAll();
        }
    }

    return QString();
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

    return QString();
}
