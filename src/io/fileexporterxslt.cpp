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

#include "fileexporterxslt.h"

#include <QRegExp>
#include <QStringList>
#include <QBuffer>
#include <QFile>
#include <QStandardPaths>

#include "file.h"
#include "element.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"
#include "encoderxml.h"
#include "fileexporterxml.h"
#include "xsltransform.h"
#include "logging_io.h"

FileExporterXSLT::FileExporterXSLT(const QString &xsltFilename, QObject *parent)
        : FileExporter(parent), m_cancelFlag(false), m_xsltFilename(xsltFilename)
{
    if (xsltFilename.isEmpty() || !QFile(xsltFilename).exists())
        qCWarning(LOG_KBIBTEX_IO) << "Invalid XSLT filename: " << xsltFilename;
}


FileExporterXSLT::~FileExporterXSLT()
{
    /// nothing
}

bool FileExporterXSLT::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    } else if (m_xsltFilename.isEmpty() || !QFile(m_xsltFilename).exists()) {
        qCWarning(LOG_KBIBTEX_IO) << "Invalid XSLT filename: " << m_xsltFilename;
        return false;
    }

    m_cancelFlag = false;
    XSLTransform xsltransformer(m_xsltFilename);
    FileExporterXML xmlExporter(this);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    if (xmlExporter.save(&buffer, bibtexfile, errorLog)) {
        buffer.close();
        buffer.open(QIODevice::ReadOnly);
        const QString xml = QString::fromUtf8(buffer.readAll().constData());
        buffer.close();

        const QString html = xsltransformer.transform(xml);
        if (!html.isEmpty()) {
            iodevice->write(html.toUtf8());
            iodevice->close();
            return !m_cancelFlag;
        }
    }

    iodevice->close();
    return false;
}

bool FileExporterXSLT::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    } else if (m_xsltFilename.isEmpty() || !QFile(m_xsltFilename).exists()) {
        qCWarning(LOG_KBIBTEX_IO) << "Invalid XSLT filename: " << m_xsltFilename;
        return false;
    }

    m_cancelFlag = false;
    XSLTransform xsltransformer(m_xsltFilename);
    FileExporterXML xmlExporter(this);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    if (xmlExporter.save(&buffer, element, bibtexfile, errorLog)) {
        buffer.close();
        buffer.open(QIODevice::ReadOnly);
        const QString xml = QString::fromUtf8(buffer.readAll().constData());
        buffer.close();

        const QString html = xsltransformer.transform(xml);
        if (!html.isEmpty()) {
            iodevice->write(html.toUtf8());
            iodevice->close();
            return !m_cancelFlag;
        }
    }

    iodevice->close();
    return false;
}

void FileExporterXSLT::cancel()
{
    m_cancelFlag = true;
}

FileExporterHTML::FileExporterHTML(QObject *parent)
        : FileExporterXSLT(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kbibtex/standard.xsl")), parent)
{
    /// nothing
}
