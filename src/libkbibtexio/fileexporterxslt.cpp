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
#include <QRegExp>
#include <QStringList>
#include <QBuffer>

#include <KDebug>
#include <KStandardDirs>

#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include <file.h>
#include <element.h>
#include <entry.h>
#include <encoderxml.h>
#include <macro.h>
#include <comment.h>
#include "fileexporterxml.h"
#include "iocommon.h"
#include "fileexporterxslt.h"

class XSLTransform
{

public:
    XSLTransform(const QString& xsltFilename) {
        m_xlstStylesheet = xsltParseStylesheetFile((const xmlChar*) xsltFilename.toAscii().data());
        if (m_xlstStylesheet == NULL)
            kError() << "Could not load XSLT file " << xsltFilename;
    }

    ~XSLTransform() {
        xsltFreeStylesheet(m_xlstStylesheet);
    }

    QString transform(const QString& xmlText) const {
        QString result = QString::null;
        QByteArray xmlCText = xmlText.toUtf8();
        xmlDocPtr document = xmlParseMemory(xmlCText, xmlCText.length());
        if (document) {
            if (m_xlstStylesheet) {
                xmlDocPtr resultDocument = xsltApplyStylesheet(m_xlstStylesheet, document, NULL);
                if (resultDocument) {
                    // Save the result into the QString
                    xmlChar * mem;
                    int size;
                    xmlDocDumpMemoryEnc(resultDocument, &mem, &size, "UTF-8");
                    result = QString::fromUtf8(QByteArray((char *)(mem), size + 1));
                    xmlFree(mem);

                    xmlFreeDoc(resultDocument);
                } else
                    kError() << "Applying XSLT stylesheet to XML document failed";
            } else
                kError() << "XSLT stylesheet is not available or not valid";

            xmlFreeDoc(document);
        } else
            kError() << "XML document is not available or not valid";

        return result;
    }

private:
    xsltStylesheetPtr m_xlstStylesheet;

};

FileExporterXSLT::FileExporterXSLT(const QString& xsltFilename)
        : FileExporter()
{
    if (xsltFilename.isNull())
        setXSLTFilename(KStandardDirs::locate("appdata", "standard.xsl"));
    else
        setXSLTFilename(xsltFilename);
}


FileExporterXSLT::~FileExporterXSLT()
{
    // nothing
}

bool FileExporterXSLT::save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog)
{
    m_cancelFlag = false;
    XSLTransform xsltransformer(m_xsltFilename);
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
        QString html = xsltransformer.transform(xml);
        QTextStream htmlTS(iodevice);
        htmlTS.setCodec("UTF-8");
        htmlTS << html << endl;
        return !m_cancelFlag;
    }

    return false;
}

bool FileExporterXSLT::save(QIODevice* iodevice, const Element* element, QStringList *errorLog)
{
    m_cancelFlag = false;
    XSLTransform xsltransformer(m_xsltFilename);
    FileExporterXML xmlExporter;

    QBuffer buffer;

    buffer.open(QIODevice::WriteOnly);
    if (xmlExporter.save(&buffer, element, errorLog)) {
        buffer.close();
        buffer.open(QIODevice::ReadOnly);
        QTextStream ts(&buffer);
        ts.setCodec("UTF-8");
        QString xml = ts.readAll();
        buffer.close();

        QString html = xsltransformer.transform(xml);
        QTextStream htmlTS(iodevice);
        htmlTS.setCodec("UTF-8");
        htmlTS << html << endl;
        return !m_cancelFlag;
    }

    return false;
}

void FileExporterXSLT::setXSLTFilename(const QString& xsltFilename)
{
    kDebug() << "xsltFilename=" << xsltFilename;
    m_xsltFilename = xsltFilename;
}

void FileExporterXSLT::cancel()
{
    m_cancelFlag = true;
}
