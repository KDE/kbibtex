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

#ifndef ENCODERLATEX_H
#define ENCODERLATEX_H

#include "kbibtexio_export.h"

#ifdef HAVE_ICU
#include <unicode/translit.h>
#endif // HAVE_ICU

#include <QIODevice>

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
    QString decode(const QString &text) const override;
    QString encode(const QString &text, const TargetEncoding targetEncoding) const override;
#ifdef HAVE_ICU
    QString convertToPlainAscii(const QString &input) const;
#else // HAVE_ICU
    /// Dummy implementation without ICU
    inline QString convertToPlainAscii(const QString &input) const { return input; }
#endif // HAVE_ICU
    static bool containsOnlyAscii(const QString &text);

    static const EncoderLaTeX &instance();
    ~EncoderLaTeX() override;

#ifdef BUILD_TESTING
    static bool writeLaTeXTables(QIODevice &output);
#endif // BUILD_TESTING

protected:
    EncoderLaTeX();

    /**
     * This data structure keeps individual characters that have
     * a special purpose in LaTeX and therefore needs to be escaped
     * both in text and in math mode by prefixing with a backlash.
     */
    static const QChar encoderLaTeXProtectedSymbols[];

    /**
     * This data structure keeps individual characters that have
     * a special purpose in LaTeX in text mode and therefore needs
     * to be escaped by prefixing with a backlash. In math mode,
     * those have a different purpose and may not be escaped there.
     */
    static const QChar encoderLaTeXProtectedTextOnlySymbols[];

    /**
     * Check if input data contains a verbatim command like \url{...},
     * append it to output, and update the position to point to the next
     * character after the verbatim command.
     * @return 'true' if a verbatim command has been copied, otherwise 'false'.
     */
    bool testAndCopyVerbatimCommands(const QString &input, int &pos, QString &output) const;

private:
    Q_DISABLE_COPY(EncoderLaTeX)

    /**
     * Check if character c represents a modifier like
     * a quotation mark in \"a. Return the modifier's
     * index in the lookup table or -1 if not a known
     * modifier.
     */
    int modifierInLookupTable(const QChar modifier) const;

    /**
     * Return a string that represents the part of
     * the base string starting from startFrom contains
     * only alpha characters (a-z,A-Z).
     * Return value may be an empty string.
     */
    QString readAlphaCharacters(const QString &base, int startFrom) const;

#ifdef HAVE_ICU
    icu::Transliterator *m_trans;
#endif // HAVE_ICU
};

#endif // ENCODERLATEX_H
