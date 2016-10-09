/***************************************************************************
 *   Copyright (C) 2004-2016 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <QFileInfo>
#include <QXmlQuery>
#include <QBuffer>

#include "logging_io.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
XSLTransform *XSLTransform::createXSLTransform(const QString &xsltFilename)
{
    if (xsltFilename.isEmpty()) {
        qCWarning(LOG_KBIBTEX_IO) << "Filename xsltFilename=" << xsltFilename << "is empty";
        return NULL;
    }

    if (!QFileInfo::exists(xsltFilename)) {
        qCWarning(LOG_KBIBTEX_IO) << "File xsltFilename=" << xsltFilename << " does not exist";
        return NULL;
    }

    QFile xsltFile(xsltFilename);
    if (xsltFile.open(QFile::ReadOnly)) {
        const QString xsltText = QString::fromUtf8(xsltFile.readAll().constData());
        if (xsltText.isEmpty()) {
            qCWarning(LOG_KBIBTEX_IO) << "File xsltFilename=" << xsltFilename << " is empty";
            return NULL;
        } else
            return new XSLTransform(xsltText);
    } else {
        qCWarning(LOG_KBIBTEX_IO) << "File xsltFilename=" << xsltFilename << " cannot be opened for reading";
        return NULL;
    }
}

XSLTransform::XSLTransform(const QString &_xsltText)
    : xsltText(_xsltText)
{
    /// nothing
}

QString XSLTransform::transform(const QString &xmlText) const
{
    QXmlQuery query(QXmlQuery::XSLT20);

    /// Create QBuffer from QString, set as XML data via setFocus(..)
    QByteArray xmlData(xmlText.toUtf8());
    QBuffer xmlBuffer(&xmlData);
    xmlBuffer.open(QIODevice::ReadOnly);
    query.setFocus(&xmlBuffer);

    /// Create QBuffer from QString, set as XSLT data via setQuery(..)
    QByteArray xsltData(xsltText.toUtf8());
    QBuffer xsltBuffer(&xsltData);
    xsltBuffer.open(QIODevice::ReadOnly);
    query.setQuery(&xsltBuffer);

    QString result;
    if (query.evaluateTo(&result))
        return result;
    else
        return QString();
}
