/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#ifndef KBIBTEX_XSLTRANSFORM_H
#define KBIBTEX_XSLTRANSFORM_H

#include "kbibtexio_export.h"

#include <QString>

/**
 * This class is a wrapper around libxslt, which allows to
 * apply XSL transformation on XML files.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT XSLTransform
{
public:
    /**
     * Create a new instance of a transformer.
     * @param xsltFilename file name of the XSL file
     */
    XSLTransform(const QString& xsltFilename);
    ~XSLTransform();

    /**
     * Transform a given XML document using the tranformer's
     * XSL file.
     * @param xmlText XML document to transform
     * @return transformed document
     */
    QString transform(const QString& xmlText) const;

private:
    class XSLTransformPrivate;
    XSLTransformPrivate *d;
};

#endif // KBIBTEX_XSLTRANSFORM_H
