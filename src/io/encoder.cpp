/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <unicode/translit.h>

#include "encoderlatex.h"
#include "logging_io.h"

class Encoder::Private
{
public:
    icu::Transliterator *translit;
    UErrorCode translitErrorCode;

    Private()
            : translit(nullptr)
    {
        /// Create an ICU Transliterator, configured to
        /// transliterate virtually anything into plain ASCII
        translitErrorCode = U_ZERO_ERROR;
        translit = icu::Transliterator::createInstance("Any-Latin;Latin-ASCII", UTRANS_FORWARD, translitErrorCode);
        if (U_FAILURE(translitErrorCode)) {
            qCWarning(LOG_KBIBTEX_IO) << "Error creating an ICU Transliterator instance: " << u_errorName(translitErrorCode);
            if (translit != nullptr) delete translit;
            translit = nullptr;
        }
    }

    ~Private()
    {
        if (translit != nullptr)
            delete translit;
    }

    static inline QString unicodeStringToQString(const icu::UnicodeString &input)
    {
        static const int outputSize{(1 << 20) - 4};
        static char output[outputSize];
        icu::CheckedArrayByteSink byteSink(output, outputSize);
        input.toUTF8(byteSink);
        return QString::fromUtf8(output, input.length());
    }
};


Encoder::Encoder()
        : d(new Encoder::Private())
{
    // nothing
}

Encoder::~Encoder()
{
    delete d;
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

QString Encoder::convertToPlainAscii(const QString &_input) const
{
    const QString input{[_input]() {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QString r;
        r.reserve((((_input.length() * 12 / 10) >> 8) + 1) << 8);
        for (const QChar &c : _input) {
            const auto u{c.unicode()};
            if (c == QChar(0x2013))
                r.append(QStringLiteral("--"));
            else if (c == QChar(0x2014))
                r.append(QStringLiteral("---"));
            else if (u < 32 || u == 127) {
                // skip
            } else
                r.append(c);
        }
#else
        QString r(_input);
        r.replace(QChar(0x2013), QStringLiteral("--")).replace(QChar(0x2014), QStringLiteral("---"));
        r.removeIf([](const QChar c) {
            // Remove all ASCII control characters (0x00 .. 0x1F) and 0x7F
            const auto u{c.unicode()};
            return u < 32 || u == 127;
        });
#endif
        return r;
    }()};

    if (d->translit == nullptr) {
        qCWarning(LOG_KBIBTEX_IO) << "Cannot convert to ASCII as no icu::Transliterator instance got instantiated";
        return input;
    }

    /// Create an ICU-specific Unicode string
    icu::UnicodeString uString = icu::UnicodeString(input.toStdU16String());
    /// Perform the actual transliteration, modifying Unicode string
    d->translit->transliterate(uString);
    if (U_FAILURE(d->translitErrorCode)) {
        qCWarning(LOG_KBIBTEX_IO) << "Failed to transliterate input string: " << u_errorName(d->translitErrorCode);
        return input;
    }

    return Private::unicodeStringToQString(uString);
}

bool Encoder::containsOnlyAscii(const QString &ntext)
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString text = ntext.normalized(QString::NormalizationForm_C);

    for (const QChar &c : text)
        if (c.unicode() > 127) return false;
    return true;
}
