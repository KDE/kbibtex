/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "textencoder.h"

#include <QStringList>
#include <QTextCodec>
#include <QDebug>
#include "logging_io.h"

#include "encoderlatex.h"

TextEncoder::TextEncoder()
{
    /// nothing
}

QByteArray TextEncoder::encode(const QString &input, const QString &destinationEncoding)
{
    QTextCodec *destinationCodec(QTextCodec::codecForName(destinationEncoding.toLatin1()));
    return encode(input, destinationCodec);
}

QByteArray TextEncoder::encode(const QString &input, const QTextCodec *destinationCodec)
{
    /// Invalid codec? Cannot do anything
    if (destinationCodec == nullptr)
        return QByteArray();

    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString ninput = input.normalized(QString::NormalizationForm_C);

    QByteArray result;
    const Encoder &laTeXEncoder = EncoderLaTeX::instance();
    /// Build result, character by character
    for (const QChar &c : ninput) {
        /// Get byte sequence representing current character in chosen codec
        const QByteArray cba = destinationCodec->fromUnicode(c);
        if (destinationCodec->canEncode(c) && (c == QChar(0x003f /** question mark */) || cba.size() != 1 || cba[0] != 0x3f /** question mark */)) {
            /// Codec claims that it can encode current character, but some codecs
            /// still cannot encode character and simply return a question mark, so
            /// only accept question marks as encoding result if original character
            /// was question mark (assuming all codecs can encode question marks).
            result.append(cba);
        } else {
            /// Chosen codec can NOT encode current Unicode character, so try to use
            /// 'LaTeX encoder', which may translate 0x00c5 (A with ring above) into
            /// '\AA'. LaTeX encoder returns UTF-8 representation if given character
            /// cannot be encoded
            result.append(laTeXEncoder.encode(QString(c), Encoder::TargetEncodingASCII).toUtf8());
        }
    }

    return result;
}

const QStringList TextEncoder::encodings {
        /// the classics
        QStringLiteral("US-ASCII"), /** effectively like 'LaTeX' encoding */
        /// ISO 8859 a.k.a. Latin codecs
        QStringLiteral("ISO-8859-1"), QStringLiteral("ISO-8859-2"), QStringLiteral("ISO-8859-3"), QStringLiteral("ISO-8859-4"), QStringLiteral("ISO-8859-5"), QStringLiteral("ISO-8859-6"), QStringLiteral("ISO-8859-7"), QStringLiteral("ISO-8859-8"), QStringLiteral("ISO-8859-9"), QStringLiteral("ISO-8859-10"), QStringLiteral("ISO-8859-13"), QStringLiteral("ISO-8859-14"), QStringLiteral("ISO-8859-15"), QStringLiteral("ISO-8859-16"),
        /// various Unicode codecs
        QStringLiteral("UTF-8"), QStringLiteral("UTF-16"), QStringLiteral("UTF-16BE"), QStringLiteral("UTF-16LE"), QStringLiteral("UTF-32"), QStringLiteral("UTF-32BE"), QStringLiteral("UTF-32LE"),
        /// various Cyrillic codecs
        QStringLiteral("KOI8-R"), QStringLiteral("KOI8-U"),
        /// various CJK codecs
        QStringLiteral("Big5"), QStringLiteral("Big5-HKSCS"), QStringLiteral("GB18030"), QStringLiteral("EUC-JP"), QStringLiteral("EUC-KR"), QStringLiteral("ISO 2022-JP"), QStringLiteral("Shift-JIS"),
        /// Windows codecs
        QStringLiteral("Windows-1250"), QStringLiteral("Windows-1251"), QStringLiteral("Windows-1252"), QStringLiteral("Windows-1253"), QStringLiteral("Windows-1254"), QStringLiteral("Windows-1255"), QStringLiteral("Windows-1256"), QStringLiteral("Windows-1257"), QStringLiteral("Windows-1258")
};
