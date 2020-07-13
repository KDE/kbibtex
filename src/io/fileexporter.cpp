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

#include "fileexporter.h"

#include <QBuffer>
#include <QTextStream>

#include <Element>
#include "logging_io.h"

FileExporter::FileExporter(QObject *parent)
        : QObject(parent)
{
    /// nothing
}

FileExporter::~FileExporter()
{
    /// nothing
}

QString FileExporter::toString(const QSharedPointer<const Element> element, const File *bibtexfile)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    if (save(&buffer, element, bibtexfile)) {
        buffer.close();
        if (buffer.open(QBuffer::ReadOnly)) {
            QTextStream ts(&buffer);
            ts.setCodec("UTF-8");
            return ts.readAll();
        }
    }

    return QString();
}

QString FileExporter::toString(const File *bibtexfile)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    if (save(&buffer, bibtexfile)) {
        buffer.close();
        if (buffer.open(QBuffer::ReadOnly)) {
            QTextStream ts(&buffer);
            ts.setCodec("utf-8");
            return ts.readAll();
        }
    }

    return QString();
}

QString FileExporter::numberToOrdinal(const int number)
{
    if (number == 1)
        return QStringLiteral("First");
    else if (number == 2)
        return QStringLiteral("Second");
    else if (number == 3)
        return QStringLiteral("Third");
    else if (number == 4)
        return QStringLiteral("Fourth");
    else if (number == 5)
        return QStringLiteral("Fifth");
    else if (number >= 20 && number % 10 == 1) {
        // 21, 31, 41, ...
        return QString(QStringLiteral("%1st")).arg(number);
    } else if (number >= 20 && number % 10 == 2) {
        // 22, 32, 42, ...
        return QString(QStringLiteral("%1nd")).arg(number);
    } else if (number >= 20 && number % 10 == 3) {
        // 23, 33, 43, ...
        return QString(QStringLiteral("%1rd")).arg(number);
    } else if (number >= 6) {
        // Remaining editions: 6, 7, ..., 19, 20, 24, 25, ...
        return QString(QStringLiteral("%1th")).arg(number);
    } else {
        // Unsupported editions, like -23
        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to convert number" << number << "into an ordinal string for edition";
        return QString();
    }

}
