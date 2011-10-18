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

#ifndef ENCODERLATEX_H
#define ENCODERLATEX_H

#include "kbibtexio_export.h"

#include "encoder.h"

/**
 * Base class for that convert between different textual representations
 * for non-ASCII characters, specialized for LaTeX. For example, this
 * class can "translate" between \"a and its UTF-8 representation.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT EncoderLaTeX: public Encoder
{
public:
    QString decode(const QString &text) const;
    QString encode(const QString &text) const;
//   QString encode(const QString &text, const QChar &replace);
    // QString& decomposedUTF8toLaTeX(QString &text);
    QString convertToPlainAscii(const QString &input) const;

    static EncoderLaTeX* instance();

private:
    /**
     * Check if character c represents a modifier like
     * a quotation mark in \"a. Return the modifier's
     * index in the lookup table or -1 if not a known
     * modifier.
     */
    int modifierInLookupTable(const QChar &c) const;

    /**
     * Return a byte array that represents the part of
     * the base byte array starting from startFrom containing
     * only alpha characters (a-z,A-Z).
     * Return value may be an empty byte array.
     */
    QString readAlphaCharacters(const QString &base, int startFrom) const;

    EncoderLaTeX();
    ~EncoderLaTeX();
    static EncoderLaTeX *self;
};

#endif // ENCODERLATEX_H
