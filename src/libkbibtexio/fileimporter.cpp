/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QTextStream>

#include "fileimporter.h"

using namespace KBibTeX::IO;

FileImporter::FileImporter()
        : QObject()
{
    // nothing
}

FileImporter::~FileImporter()
{
    // nothing
}

File* FileImporter::load(const QString& text)
{
    if (text.isNull() || text.isEmpty())
        return NULL;

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QTextStream stream(&buffer);
    stream.setCodec("UTF-8");
    stream << text;
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    File *result = load(&buffer);
    buffer.close();

    return result;
}

// #include "fileimporter.moc"
