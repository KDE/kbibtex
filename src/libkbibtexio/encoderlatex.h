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
#ifndef ENCODERLATEX_H
#define ENCODERLATEX_H

#include "kbibtexio_export.h"

#include "encoder.h"

/**
 * Base class for that convert between different textual representations
 * for non-ASCII characters, specialized for LaTeX.
 * Example for a character to convert is \"a.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT EncoderLaTeX: public Encoder
{
public:
    EncoderLaTeX();
    ~EncoderLaTeX();

    QString decode(const QString &text) const;
    QString encode(const QString &text) const;
    QString convertToPlainAscii(const QString &text) const;

    static EncoderLaTeX* instance();

private:
    class EncoderLaTeXPrivate;
    EncoderLaTeXPrivate * const d;

    QString& decomposedUTF8toLaTeX(QString &text) const;
};

#endif // ENCODERLATEX_H
