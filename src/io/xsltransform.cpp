/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#include <QDebug>

#include "logging_io.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
XSLTransform::XSLTransform(const QString &xsltFilename)
        : xsltData(nullptr)
{
    if (!xsltFilename.isEmpty()) {
        QFile xsltFile(xsltFilename);
        if (xsltFile.open(QFile::ReadOnly)) {
            xsltData = new QByteArray(xsltFile.readAll());
            xsltFile.close();
            if (xsltData->size() == 0) {
                qWarning() << "Read only 0 Bytes from file" << xsltFilename;
                delete xsltData;
                xsltData = nullptr;
            }
        } else
            qWarning() << "Opening XSLT file" << xsltFilename << "failed";
    } else
        qWarning() << "Empty filename for XSLT";
}

XSLTransform::~XSLTransform() {
    if (xsltData != nullptr) delete xsltData;
}

QString XSLTransform::transform(const QString &xmlText) const
{
    if (xsltData == nullptr) {
        qWarning() << "Empty XSL transformation cannot transform";
        return QString();
    }

    QXmlQuery query(QXmlQuery::XSLT20);

    if (!query.setFocus(xmlText)) {
        qWarning() << "Invoking QXmlQuery::setFocus(" << xmlText.left(32) << "...) failed";
        return QString();
    }

    QBuffer xsltBuffer(xsltData);
    xsltBuffer.open(QBuffer::ReadOnly);
    query.setQuery(&xsltBuffer);

    if (!query.isValid()) {
        qWarning() << "QXmlQuery::isValid got negative result";
        return QString();
    }

    QString result;
    if (query.evaluateTo(&result)) {
        /// Return result of XSL transformation and replace
        /// the usual XML suspects with its plain counterparts
        return result;
    } else {
        qWarning() << "Invoking QXmlQuery::evaluateTo(...) failed";
        return QString();
    }
}
