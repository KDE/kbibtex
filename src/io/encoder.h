/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef BIBTEXENCODER_H
#define BIBTEXENCODER_H

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

#ifdef HAVE_ICU
#include <unicode/translit.h>
#endif // HAVE_ICU

#include <QString>

/**
 * Base class for that convert between different textual representations
 * for non-ASCII characters. Examples for external textual representations
 * are \"a in LaTeX and &auml; in XML.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT Encoder
{
public:
    enum TargetEncoding {TargetEncodingASCII = 0, TargetEncodingUTF8 = 1};

    static const Encoder &instance();
    virtual ~Encoder() {
#ifdef HAVE_ICU
        if (m_trans != nullptr)
            delete m_trans;
#endif // HAVE_ICU
    }

    /**
     * Decode from external textual representation to internal (UTF-8) representation.
     * @param text text in external textual representation
     * @return text in internal (UTF-8) representation
     */
    virtual QString decode(const QString &text) const;

    /**
     * Encode from internal (UTF-8) representation to external textual representation.
     * Output may be restricted to ASCII (non-ASCII characters will be rewritten depending
     * on concrete Encoder class, for example as '&#228;' as XML or '\"a' for LaTeX)
     * or UTF-8 (all characters allowed, only 'special ones' rewritten, for example
     * '&amp;' for XML and '\&' for LaTeX).
     * @param text in internal (UTF-8) representation
     * @param targetEncoding allow either only ASCII output or UTF-8 output.
     * @return text text in external textual representation
     */
    virtual QString encode(const QString &text, const TargetEncoding targetEncoding) const;

#ifdef HAVE_ICU
    QString convertToPlainAscii(const QString &input) const;
#else // HAVE_ICU
    /// Dummy implementation without ICU
    inline QString convertToPlainAscii(const QString &input) const {
        return input;
    }
#endif // HAVE_ICU
    static bool containsOnlyAscii(const QString &text);

protected:
    Encoder();

private:
#ifdef HAVE_ICU
    icu::Transliterator *m_trans;
#endif // HAVE_ICU
};

#endif
