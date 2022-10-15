/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include "fileexporterbibtex.h"
#include "fileexporterbibtexoutput.h"
#include "fileexporterxml.h"
#include "fileexporterxslt.h"
#include "fileexporterris.h"
#include "fileexporterpdf.h"
#include "fileexporterps.h"
#include "fileexporterrtf.h"
#include "fileexporterbibtex2html.h"
#include "fileexporterbibutils.h"
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


FileExporter *FileExporter::factory(const QFileInfo &fileInfo, QObject *parent)
{
    const QString ending = fileInfo.completeSuffix().toLower();

    if (ending.endsWith(QStringLiteral("html")) || ending.endsWith(QStringLiteral("htm"))) {
        return new FileExporterHTML(parent);
    } else if (ending.endsWith(QStringLiteral("xml"))) {
        return new FileExporterXML(parent);
    } else if (ending.endsWith(QStringLiteral("ris"))) {
        return new FileExporterRIS(parent);
    } else if (ending.endsWith(QStringLiteral("pdf"))) {
        return new FileExporterPDF(parent);
    } else if (ending.endsWith(QStringLiteral("ps"))) {
        return new FileExporterPS(parent);
    } else if (BibUtils::available() && ending.endsWith(QStringLiteral("isi"))) {
        FileExporterBibUtils *fileExporterBibUtils = new FileExporterBibUtils(parent);
        fileExporterBibUtils->setFormat(BibUtils::Format::ISI);
        return fileExporterBibUtils;
    } else if (ending.endsWith(QStringLiteral("rtf"))) {
        return new FileExporterRTF(parent);
    }
    /*
     * FIXME: FileExporterHTML and FileExporterBibTeX2HTML compete for which exporter is used
     *        for HTML export. This should be determined in some global setting.
     *        If FileExporterBibTeX2HTML is to be used, the bibliography style has to be
     *        determined as well.
    else if (ending.endsWith(QStringLiteral("html")) || ending.endsWith(QStringLiteral("htm"))) {
        return new FileExporterBibTeX2HTML(parent);
    }
    */
    else if (ending.endsWith(QStringLiteral("bbl"))) {
        return new FileExporterBibTeXOutput(FileExporterBibTeXOutput::OutputType::BibTeXBlockList, parent);
    } else {
        return new FileExporterBibTeX(parent);
    }
}

FileExporter *FileExporter::factory(const QUrl &url, QObject *parent)
{
    const QFileInfo inputFileInfo(url.fileName());
    return factory(inputFileInfo, parent);
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
