/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "encoderutf8.h"

#include <QString>

#include <KDebug>

EncoderUTF8 *EncoderUTF8::self = NULL;

EncoderUTF8::EncoderUTF8()
{
    // nothing
}

EncoderUTF8::~EncoderUTF8()
{
    // nothing
}

QString EncoderUTF8::encode(const QString &input) const
{
    int len = input.length();
    int s = len * 9 / 8;
    QString output;
    output.reserve(s);
    bool inMathMode = false;

    /// Go through input char by char
    for (int i = 0; i < len; ++i) {
        /**
         * Repeatedly check if input data contains a verbatim command
         * like \url{...}, copy it to output, and update i to point
         * to the next character after the verbatim command.
         */
        while (testAndCopyVerbatimCommands(input, i, output));
        if (i >= len) break;

        const QChar c = input[i];

        /// Some characters have special meaning
        /// in TeX and have to be preceded with a backslash
        bool found = false;
        for (int k = 0; k < encoderLaTeXProtectedSymbolsLen; ++k)
            if (encoderLaTeXProtectedSymbols[k] == c) {
                output.append(QChar('\\'));
                found = true;
                break;
            }

        if (!found && !inMathMode)
            for (int k = 0; k < encoderLaTeXProtectedTextOnlySymbolsLen; ++k)
                if (encoderLaTeXProtectedTextOnlySymbols[k] == c) {
                    output.append(QChar('\\'));
                    found = true;
                    break;
                }

        /// Dump character to output
        output.append(c);

        /// Finally, check if input character is a dollar sign
        /// without a preceding backslash, means toggling between
        /// text mode and math mode
        if (c == '$' && (i == 0 || input[i - 1] != QChar('\\')))
            inMathMode = !inMathMode;
    }

    return output;
}

EncoderUTF8 *EncoderUTF8::instance()
{
    if (self == NULL)
        self = new EncoderUTF8();
    return self;
}
