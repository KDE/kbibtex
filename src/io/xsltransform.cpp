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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "xsltransform.h"

#include <QFileInfo>
#include <QXmlQuery>
#include <QBuffer>

#include "logging_io.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
XSLTransform::XSLTransform(const QString &_xsltFilename)
        : xsltFilename(_xsltFilename), query(new QXmlQuery(QXmlQuery::XSLT20))
{
    /// nothing
}

XSLTransform::~XSLTransform() {
    delete query;
}

QString XSLTransform::transform(const QString &xmlText) const
{
    /// Create QBuffer from QString, set as XML data via setFocus(..)
    QByteArray xmlData(xmlText.toUtf8());
    QBuffer xmlBuffer(&xmlData);
    xmlBuffer.open(QIODevice::ReadOnly);
    query->setFocus(&xmlBuffer);
    query->setQuery(QUrl::fromLocalFile(xsltFilename));

    QString result;
    if (query->evaluateTo(&result))
        return result;
    else
        return QString();
}
