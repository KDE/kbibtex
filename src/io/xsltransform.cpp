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

#include "xsltransform.h"

#include <libxslt/xsltutils.h>

#include <QFileInfo>
#include <QDebug>

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
XSLTransform *XSLTransform::createXSLTransform(const QString &xsltFilename)
{
    if (xsltFilename.isEmpty()) {
        qWarning() << "Filename xsltFilename=" << xsltFilename << "is empty";
        return NULL;
    }

    if (!QFileInfo(xsltFilename).exists()) {
        qWarning() << "File xsltFilename=" << xsltFilename << " does not exist";
        return NULL;
    }

    /// create an internal representation of the XSL file using libxslt
    const xsltStylesheetPtr xsltStylesheet = xsltParseStylesheetFile((const xmlChar *) xsltFilename.toLatin1().data());
    if (xsltStylesheet == NULL) {
        qWarning() << "File xsltFilename=" << xsltFilename << " resulted in empty/invalid XSLT style sheet";
        return NULL;
    }

    return new XSLTransform(xsltStylesheet);
}

XSLTransform::XSLTransform(const xsltStylesheetPtr &xsltSS)
        : xsltStylesheet(xsltSS)
{
    // nothing
}

XSLTransform::~XSLTransform()
{
    /// Clean up memory
    xsltFreeStylesheet(xsltStylesheet);
}

QString XSLTransform::transform(const QString &xmlText) const
{
    QString result;
    QByteArray xmlCText = xmlText.toUtf8();
    xmlDocPtr document = xmlParseMemory(xmlCText, xmlCText.length());
    if (document) {
        if (xsltStylesheet != NULL) {
            xmlDocPtr resultDocument = xsltApplyStylesheet(xsltStylesheet, document, NULL);
            if (resultDocument) {
                /// Save the result into the QString
                xmlChar *mem;
                int size;
                xmlDocDumpMemoryEnc(resultDocument, &mem, &size, "UTF-8");
                result = QString::fromUtf8(QByteArray((char *)(mem), size + 1));
                xmlFree(mem);

                xmlFreeDoc(resultDocument);
            } else
                qCritical() << "Applying XSLT stylesheet to XML document failed";
        } else
            qCritical() << "XSLT stylesheet is not available or not valid";

        xmlFreeDoc(document);
    } else
        qCritical() << "XML document is not available or not valid";

    return result;
}

void XSLTransform::cleanupGlobals()
{
    xsltCleanupGlobals();
}
