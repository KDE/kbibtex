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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "fileexporterxslt.h"

#include <QRegExp>
#include <QStringList>
#include <QBuffer>
#include <QFile>

#include <KDebug>
#include <KStandardDirs>

#include "file.h"
#include "element.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"
#include "encoderxml.h"
#include "fileexporterxml.h"
#include "iocommon.h"
#include "xsltransform.h"

FileExporterXSLT::FileExporterXSLT(const QString &xsltFilename)
        : FileExporter()
{
    if (xsltFilename.isEmpty() || !QFile(xsltFilename).exists())
        setXSLTFilename(KStandardDirs::locate("data", "kbibtex/standard.xsl"));
    else
        setXSLTFilename(xsltFilename);
}


FileExporterXSLT::~FileExporterXSLT()
{
    // nothing
}

bool FileExporterXSLT::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        kDebug() << "Output device not writable";
        return false;
    }

    m_cancelFlag = false;
    XSLTransform *xsltransformer = XSLTransform::createXSLTransform(m_xsltFilename);
    if (xsltransformer != NULL) {
        FileExporterXML xmlExporter;

        QBuffer buffer;

        buffer.open(QIODevice::WriteOnly);
        if (xmlExporter.save(&buffer, bibtexfile, errorLog)) {
            buffer.close();
            buffer.open(QIODevice::ReadOnly);
            QTextStream ts(&buffer);
            ts.setCodec("UTF-8");
            QString xml = ts.readAll();
            buffer.close();
            QString html = xsltransformer->transform(xml);
            QTextStream htmlTS(iodevice);
            htmlTS.setCodec("UTF-8");
            htmlTS << html << endl;

            delete xsltransformer;
            iodevice->close();
            return !m_cancelFlag;
        }

        delete xsltransformer;
    }

    iodevice->close();
    return false;
}

bool FileExporterXSLT::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        kDebug() << "Output device not writable";
        return false;
    }

    m_cancelFlag = false;
    XSLTransform *xsltransformer = XSLTransform::createXSLTransform(m_xsltFilename);
    if (xsltransformer != NULL) {
        FileExporterXML xmlExporter;

        QBuffer buffer;

        buffer.open(QIODevice::WriteOnly);
        if (xmlExporter.save(&buffer, element, bibtexfile, errorLog)) {
            buffer.close();
            buffer.open(QIODevice::ReadOnly);
            QTextStream ts(&buffer);
            ts.setCodec("UTF-8");
            QString xml = ts.readAll();
            buffer.close();

            QString html = xsltransformer->transform(xml);
            QTextStream htmlTS(iodevice);
            htmlTS.setCodec("UTF-8");
            htmlTS << html << endl;

            delete xsltransformer;
            iodevice->close();
            return !m_cancelFlag;
        }

        delete xsltransformer;
    }

    iodevice->close();
    return false;
}

void FileExporterXSLT::setXSLTFilename(const QString &xsltFilename)
{
    m_xsltFilename = xsltFilename;
}

void FileExporterXSLT::cancel()
{
    m_cancelFlag = true;
}
