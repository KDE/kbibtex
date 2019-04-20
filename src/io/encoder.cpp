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

#include "encoder.h"

#include "logging_io.h"

Encoder::Encoder()
{
#ifdef HAVE_ICU
    /// Create an ICU Transliterator, configured to
    /// transliterate virtually anything into plain ASCII
    UErrorCode uec = U_ZERO_ERROR;
    m_trans = icu::Transliterator::createInstance("Any-Latin;Latin-ASCII", UTRANS_FORWARD, uec);
    if (U_FAILURE(uec)) {
        qCWarning(LOG_KBIBTEX_IO) << "Error creating an ICU Transliterator instance: " << u_errorName(uec);
        if (m_trans != nullptr) delete m_trans;
        m_trans = nullptr;
    }
#endif // HAVE_ICU
}

const Encoder &Encoder::instance()
{
    static const Encoder self;
    return self;
}

QString Encoder::decode(const QString &text) const
{
    return text;
}

QString Encoder::encode(const QString &text, const TargetEncoding) const
{
    return text;
}

#ifdef HAVE_ICU
QString Encoder::convertToPlainAscii(const QString &ninput) const
{
    /// Previously, iconv's //TRANSLIT feature had been used here.
    /// However, the transliteration is locale-specific as discussed
    /// here:
    /// http://taschenorakel.de/mathias/2007/11/06/iconv-transliterations/
    /// Therefore, iconv is not an acceptable solution.
    ///
    /// Instead, "International Components for Unicode" (ICU) is used.
    /// It is already a dependency for Qt, so there is no "cost" involved
    /// in using it.

    /// Preprocessing where ICU may give unexpected results otherwise
    QString input = ninput;
    input = input.replace(QChar(0x2013), QStringLiteral("--")).replace(QChar(0x2014), QStringLiteral("---"));

    const int inputLen = input.length();
    /// Make a copy of the input string into an array of UChar
    UChar *uChars = new UChar[inputLen];
    for (int i = 0; i < inputLen; ++i)
        uChars[i] = input.at(i).unicode();
    /// Create an ICU-specific unicode string
    icu::UnicodeString uString = icu::UnicodeString(uChars, inputLen);
    /// Perform the actual transliteration, modifying Unicode string
    if (m_trans != nullptr) m_trans->transliterate(uString);
    /// Create regular C++ string from Unicode string
    std::string cppString;
    uString.toUTF8String(cppString);
    /// Clean up any mess
    delete[] uChars;
    /// Convert regular C++ to Qt-specific QString,
    /// should work as cppString contains only ASCII text
    return QString::fromStdString(cppString);
}

bool Encoder::containsOnlyAscii(const QString &ntext)
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString text = ntext.normalized(QString::NormalizationForm_C);

    for (const QChar &c : text)
        if (c.unicode() > 127) return false;
    return true;
}

#endif // HAVE_ICU
