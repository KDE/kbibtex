/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
    virtual QString decode(const QString &text) const;
    virtual QString encode(const QString &text) const;
    QString convertToPlainAscii(const QString &input) const;
    static bool containsOnlyAscii(const QString &text);

    static EncoderLaTeX *instance();

protected:
    EncoderLaTeX();
    ~EncoderLaTeX();

    /**
     * This data structure keeps individual characters that have
     * a special purpose in LaTeX and therefore needs to be escaped
     * both in text and in math mode by prefixing with a backlash.
     */
    static const char encoderLaTeXProtectedSymbols[];
    static const int encoderLaTeXProtectedSymbolsLen;

    /**
     * This data structure keeps individual characters that have
     * a special purpose in LaTeX in text mode and therefore needs
     * to be escaped by prefixing with a backlash. In math mode,
     * those have a different purpose and may not be escaped there.
     */
    static const char encoderLaTeXProtectedTextOnlySymbols[];
    static const int encoderLaTeXProtectedTextOnlySymbolsLen;

    /**
     * Check if input data contains a verbatim command like \url{...},
     * copy it to output, and update the position to point to the next
     * character after the verbatim command.
     * @return 'true' if a verbatim command has been copied, otherwise 'false'.
     */
    bool testAndCopyVerbatimCommands(const QString &input, int &pos, QString &output) const;

private:
    /**
     * Check if character c represents a modifier like
     * a quotation mark in \"a. Return the modifier's
     * index in the lookup table or -1 if not a known
     * modifier.
     */
    int modifierInLookupTable(const QChar &c) const;

    /**
     * Return a string that represents the part of
     * the base string starting from startFrom containing
     * only alpha characters (a-z,A-Z).
     * Return value may be an empty byte array.
     */
    QString readAlphaCharacters(const QString &base, int startFrom) const;

    static EncoderLaTeX *self;
};

#endif // ENCODERLATEX_H
