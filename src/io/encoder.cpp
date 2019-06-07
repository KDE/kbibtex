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

#ifdef HAVE_ICU
#include <unicode/translit.h>
#else // HAVE_ICU
#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV
#endif // HAVE_ICU

#include "encoderlatex.h"
#include "logging_io.h"

#if defined(HAVE_ICU) || defined(HAVE_ICONV)
class Encoder::Private
{
public:
#ifdef HAVE_ICU
    icu::Transliterator *translit;
#else // HAVE_ICU
#ifdef HAVE_ICONV
    iconv_t iconvHandle;
#endif // HAVE_ICONV
#endif // HAVE_ICU

    Private()
#ifdef HAVE_ICU
            : translit(nullptr)
#else // HAVE_ICU
#ifdef HAVE_ICONV
            : iconvHandle((iconv_t) -1)
#endif // HAVE_ICONV
#endif // HAVE_ICU
    {
#ifdef HAVE_ICU
        /// Create an ICU Transliterator, configured to
        /// transliterate virtually anything into plain ASCII
        UErrorCode uec = U_ZERO_ERROR;
        translit = icu::Transliterator::createInstance("Any-Latin;Latin-ASCII", UTRANS_FORWARD, uec);
        if (U_FAILURE(uec)) {
            qCWarning(LOG_KBIBTEX_IO) << "Error creating an ICU Transliterator instance: " << u_errorName(uec);
            if (translit != nullptr) delete translit;
            translit = nullptr;
        }
#else // HAVE_ICU
#ifdef HAVE_ICONV
        iconvHandle = iconv_open("ASCII//TRANSLIT//IGNORE", "UTF-8");
#endif // HAVE_ICONV
#endif // HAVE_ICU
    }

    ~Private()
    {
#ifdef HAVE_ICU
        if (translit != nullptr)
            delete translit;
#else // HAVE_ICU
#ifdef HAVE_ICONV
        if (iconvHandle != (iconv_t) -1)
            iconv_close(iconvHandle);
#endif // HAVE_ICONV
#endif // HAVE_ICU
    }
};
#endif // defined(HAVE_ICU) || defined(HAVE_ICONV)


Encoder::Encoder()
#if defined(HAVE_ICU) || defined(HAVE_ICONV)
        : d(new Encoder::Private())
#endif // defined(HAVE_ICU) || defined(HAVE_ICONV)
{
    /// nothing
}

Encoder::~Encoder()
{
#if defined(HAVE_ICU) || defined(HAVE_ICONV)
    delete d;
#endif // defined(HAVE_ICU) || defined(HAVE_ICONV)
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

#if defined(HAVE_ICU) || defined(HAVE_ICONV)
QString Encoder::convertToPlainAscii(const QString &ninput) const
{
    /// Preprocessing where ICU or Iconv may give unexpected results otherwise
    QString input = ninput;
    input = input.replace(QChar(0x2013), QStringLiteral("--")).replace(QChar(0x2014), QStringLiteral("---"));

#ifdef HAVE_ICU
    const int inputLen = input.length();
    /// Make a copy of the input string into an array of UChar
    UChar *uChars = new UChar[inputLen];
    for (int i = 0; i < inputLen; ++i)
        uChars[i] = input.at(i).unicode();
    /// Create an ICU-specific unicode string
    icu::UnicodeString uString = icu::UnicodeString(uChars, inputLen);
    /// Perform the actual transliteration, modifying Unicode string
    if (d->translit != nullptr) d->translit->transliterate(uString);
    /// Create regular C++ string from Unicode string
    std::string cppString;
    uString.toUTF8String(cppString);
    /// Clean up any mess
    delete[] uChars;
    /// Convert regular C++ to Qt-specific QString,
    /// should work as cppString contains only ASCII text
    return QString::fromStdString(cppString);
#else // HAVE_ICU
#ifdef HAVE_ICONV
    /// Get an UTF-8 representation of the input string
    QByteArray inputByteArray = input.toUtf8();
    /// Limit the size of the output buffer
    /// by making an educated guess of its maximum size
    const size_t maxOutputByteArraySize = inputByteArray.size() * 8 + 1024;
    /// iconv on Linux likes to have it as char *
    char *inputBuffer = inputByteArray.data();
    QByteArray outputByteArray(maxOutputByteArraySize, '\0');
    char *outputBuffer = outputByteArray.data();
    size_t inputBufferBytesLeft = inputByteArray.size();
    size_t ouputBufferBytesLeft = maxOutputByteArraySize;

    while (inputBufferBytesLeft > 0 && ouputBufferBytesLeft > inputBufferBytesLeft * 2 + 1024 && iconv(d->iconvHandle, &inputBuffer, &inputBufferBytesLeft, &outputBuffer, &ouputBufferBytesLeft) == (size_t)(-1)) {
        /// Split text into character where iconv stopped and remaining text
        QString remainingString = QString::fromUtf8(inputBuffer);
        const QString problematicChar = remainingString.left(1);
        remainingString = remainingString.mid(1);

        /// Setup input buffer to continue with remaining text
        inputByteArray = remainingString.toUtf8();
        inputBuffer = inputByteArray.data();
        inputBufferBytesLeft = inputByteArray.size();

        /// Encode problematic character in LaTeX encoding and append to output buffer
        const QString encodedProblem = EncoderLaTeX::instance().encode(problematicChar, Encoder::TargetEncodingASCII);
        QByteArray encodedProblemByteArray = encodedProblem.toUtf8();
        qstrncpy(outputBuffer, encodedProblemByteArray.data(), ouputBufferBytesLeft);
        ouputBufferBytesLeft -= encodedProblemByteArray.size();
        outputBuffer += encodedProblemByteArray.size();
    }

    outputByteArray.resize(maxOutputByteArraySize - ouputBufferBytesLeft);

    return QString::fromUtf8(outputByteArray);
#endif // HAVE_ICONV
#endif // HAVE_ICU
}
#endif // defined(HAVE_ICU) || defined(HAVE_ICONV)

bool Encoder::containsOnlyAscii(const QString &ntext)
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString text = ntext.normalized(QString::NormalizationForm_C);

    for (const QChar &c : text)
        if (c.unicode() > 127) return false;
    return true;
}
