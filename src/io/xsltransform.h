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

#ifndef KBIBTEX_XSLTRANSFORM_H
#define KBIBTEX_XSLTRANSFORM_H

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

#include <QString>

class QXmlQuery;
class QByteArray;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT XSLTransform
{
public:
    /**
     * Create a new instance of a transformer.
     * @param xsltFilename file name of the XSL file
     */
    explicit XSLTransform(const QString &xsltFilename);
    ~XSLTransform();

    XSLTransform(const XSLTransform &other) = delete;
    XSLTransform &operator= (const XSLTransform &other) = delete;

    bool isValid() const;

    /**
     * Transform a given XML document using the tranformer's
     * XSL file.
     * @param xmlText XML document to transform
     * @return transformed document
     */
    QString transform(const QString &xmlText) const;

    static QString locateXSLTfile(const QString &stem);

private:
    QByteArray *xsltData;
};

#endif // KBIBTEX_XSLTRANSFORM_H
