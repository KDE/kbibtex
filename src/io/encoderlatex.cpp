/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <QString>

#include <KDebug>

#include "encoderlatex.h"

/**
 * General documentation on this topic:
 *   http://www.tex.ac.uk/CTAN/macros/latex/doc/encguide.pdf
 */

EncoderLaTeX *EncoderLaTeX::self = NULL;

/**
 * This structure contains information how escaped characters
 * such as \"a are translated to an Unicode character and back.
 * The structure is a table with three columns: (1) the modified
 * (in the example before the quotation mark) (2) the ASCII
 * character ((in the example before the 'a') (3) the Unicode
 * character described by a hexcode.
 * This data structure is used both directly and indirectly via
 * the LookupTable structure which is initialized when the
 * EncoderLaTeX object is created.
 */
static const struct EncoderLaTeXEscapedCharacter {
    const char modifier;
    const char letter;
    QChar unicode;
}
encoderLaTeXEscapedCharacters[] = {
    {'`', 'A', QChar(0x00C0)},
    {'\'', 'A', QChar(0x00C1)},
    {'^', 'A', QChar(0x00C2)},
    {'~', 'A', QChar(0x00C3)},
    {'"', 'A', QChar(0x00C4)},
    {'r', 'A', QChar(0x00C5)},
    /** 0x00C6 */
    {'c', 'C', QChar(0x00C7)},
    {'`', 'E', QChar(0x00C8)},
    {'\'', 'E', QChar(0x00C9)},
    {'^', 'E', QChar(0x00CA)},
    {'"', 'E', QChar(0x00CB)},
    {'`', 'I', QChar(0x00CC)},
    {'\'', 'I', QChar(0x00CD)},
    {'^', 'I', QChar(0x00CE)},
    {'"', 'I', QChar(0x00CF)},
    /** 0x00D0: see EncoderLaTeXCharacterCommand */
    {'~', 'N', QChar(0x00D1)},
    {'`', 'O', QChar(0x00D2)},
    {'\'', 'O', QChar(0x00D3)},
    {'^', 'O', QChar(0x00D4)},
    {'~', 'O', QChar(0x00D5)},
    {'"', 'O', QChar(0x00D6)},
    /** 0x00D7 */
    /** 0x00D8: see EncoderLaTeXCharacterCommand */
    {'`', 'U', QChar(0x00D9)},
    {'\'', 'U', QChar(0x00DA)},
    {'^', 'U', QChar(0x00DB)},
    {'"', 'U', QChar(0x00DC)},
    {'\'', 'Y', QChar(0x00DD)},
    /** 0x00DE */
    {'"', 's', QChar(0x00DF)},
    {'`', 'a', QChar(0x00E0)},
    {'\'', 'a', QChar(0x00E1)},
    {'^', 'a', QChar(0x00E2)},
    {'~', 'a', QChar(0x00E3)},
    {'"', 'a', QChar(0x00E4)},
    {'r', 'a', QChar(0x00E5)},
    /** 0x00E6 */
    {'c', 'c', QChar(0x00E7)},
    {'`', 'e', QChar(0x00E8)},
    {'\'', 'e', QChar(0x00E9)},
    {'^', 'e', QChar(0x00EA)},
    {'"', 'e', QChar(0x00EB)},
    {'`', 'i', QChar(0x00EC)},
    {'\'', 'i', QChar(0x00ED)},
    {'^', 'i', QChar(0x00EE)},
    {'"', 'i', QChar(0x00EF)},
    /** 0x00F0 */
    {'~', 'n', QChar(0x00F1)},
    {'`', 'o', QChar(0x00F2)},
    {'\'', 'o', QChar(0x00F3)},
    {'^', 'o', QChar(0x00F4)},
    {'~', 'o', QChar(0x00F5)},
    {'"', 'o', QChar(0x00F6)},
    /** 0x00F7 Division Sign */
    /** 0x00F8: see EncoderLaTeXCharacterCommand */
    {'`', 'u', QChar(0x00F9)},
    {'\'', 'u', QChar(0x00FA)},
    {'^', 'u', QChar(0x00FB)},
    {'"', 'u', QChar(0x00FC)},
    {'\'', 'y', QChar(0x00FD)},
    /** 0x00FE Thorn */
    /** 0x00FF Umlaut-y*/
    {'=', 'A', QChar(0x0100)},
    {'=', 'a', QChar(0x0101)},
    {'u', 'A', QChar(0x0102)},
    {'u', 'a', QChar(0x0103)},
    {'c', 'A', QChar(0x0104)},
    {'c', 'a', QChar(0x0105)},
    {'\'', 'C', QChar(0x0106)},
    {'\'', 'c', QChar(0x0107)},
    /** 0x0108 */
    /** 0x0109 */
    /** 0x010A */
    /** 0x010B */
    {'v', 'C', QChar(0x010C)},
    {'v', 'c', QChar(0x010D)},
    {'v', 'D', QChar(0x010E)},
    /** 0x010F */
    /** 0x0110 */
    /** 0x0111 */
    {'=', 'E', QChar(0x0112)},
    {'=', 'e', QChar(0x0113)},
    /** 0x0114 */
    /** 0x0115 */
    /** 0x0116 */
    /** 0x0117 */
    {'c', 'E', QChar(0x0118)},
    {'c', 'e', QChar(0x0119)},
    {'v', 'E', QChar(0x011A)},
    {'v', 'e', QChar(0x011B)},
    /** 0x011C */
    /** 0x011D */
    {'u', 'G', QChar(0x011E)},
    {'u', 'g', QChar(0x011F)},
    /** 0x0120 */
    /** 0x0121 */
    /** 0x0122 */
    /** 0x0123 */
    /** 0x0124 */
    /** 0x0125 */
    /** 0x0126 */
    /** 0x0127 */
    {'~', 'I', QChar(0x0128)},
    {'~', 'i', QChar(0x0129)},
    {'=', 'I', QChar(0x012A)},
    {'=', 'i', QChar(0x012B)},
    {'u', 'I', QChar(0x012C)},
    {'u', 'i', QChar(0x012D)},
    /** 0x012E */
    /** 0x012F */
    /** 0x0130 */
    /** 0x0131 */
    /** 0x0132 */
    /** 0x0133 */
    /** 0x0134 */
    /** 0x0135 */
    /** 0x0136 */
    /** 0x0137 */
    /** 0x0138 */
    {'\'', 'L', QChar(0x0139)},
    {'\'', 'l', QChar(0x013A)},
    /** 0x013B */
    /** 0x013C */
    /** 0x013D */
    /** 0x013E */
    /** 0x013F */
    /** 0x0140 */
    /** 0x0141 */
    /** 0x0142 */
    {'\'', 'N', QChar(0x0143)},
    {'\'', 'n', QChar(0x0144)},
    /** 0x0145 */
    /** 0x0146 */
    {'v', 'N', QChar(0x0147)},
    {'v', 'n', QChar(0x0148)},
    /** 0x0149 */
    /** 0x014A */
    /** 0x014B */
    {'=', 'O', QChar(0x014C)},
    {'=', 'o', QChar(0x014D)},
    {'u', 'O', QChar(0x014E)},
    {'u', 'o', QChar(0x014F)},
    {'H', 'O', QChar(0x0150)},
    {'H', 'o', QChar(0x0151)},
    /** 0x0152 */
    /** 0x0153 */
    {'\'', 'R', QChar(0x0154)},
    {'\'', 'r', QChar(0x0155)},
    /** 0x0156 */
    /** 0x0157 */
    {'v', 'R', QChar(0x0158)},
    {'v', 'r', QChar(0x0159)},
    {'\'', 'S', QChar(0x015A)},
    {'\'', 's', QChar(0x015B)},
    /** 0x015C */
    /** 0x015D */
    {'c', 'S', QChar(0x015E)},
    {'c', 's', QChar(0x015F)},
    {'v', 'S', QChar(0x0160)},
    {'v', 's', QChar(0x0161)},
    /** 0x0162 */
    /** 0x0163 */
    {'v', 'T', QChar(0x0164)},
    /** 0x0165 */
    /** 0x0166 */
    /** 0x0167 */
    {'~', 'U', QChar(0x0168)},
    {'~', 'u', QChar(0x0169)},
    {'=', 'U', QChar(0x016A)},
    {'=', 'u', QChar(0x016B)},
    {'u', 'U', QChar(0x016C)},
    {'u', 'u', QChar(0x016D)},
    {'r', 'U', QChar(0x016E)},
    {'r', 'u', QChar(0x016F)},
    /** 0x0170 */
    /** 0x0171 */
    /** 0x0172 */
    /** 0x0173 */
    /** 0x0174 */
    /** 0x0175 */
    /** 0x0176 */
    /** 0x0177 */
    {'"', 'Y', QChar(0x0178)},
    {'\'', 'Z', QChar(0x0179)},
    {'\'', 'z', QChar(0x017A)},
    /** 0x017B */
    /** 0x017C */
    {'v', 'Z', QChar(0x017D)},
    {'v', 'z', QChar(0x017E)},
    /** 0x017F */
    /** 0x0180 */
    {'=', 'Y', QChar(0x0232)},
    {'=', 'y', QChar(0x0233)},
    {'v', 'A', QChar(0x01CD)},
    {'v', 'a', QChar(0x01CE)},
    {'v', 'G', QChar(0x01E6)},
    {'v', 'g', QChar(0x01E7)},
    {'r', 'q', QChar(0x2019)} ///< tricky: this is \rq
};
static const int encoderLaTeXEscapedCharactersLen = sizeof(encoderLaTeXEscapedCharacters) / sizeof(encoderLaTeXEscapedCharacters[0]);


/**
 * This structure contains information on the usage of dotless i
 * and dotless j in combination with accent-like modifiers.
 * Combinations such as \"{\i} are translated to an Unicode character
 * and back. The structure is a table with three columns: (1) the
 * modified (in the example before the quotation mark) (2) the ASCII
 * character (in the example before the 'i') (3) the Unicode
 * character described by a hexcode.
 */
// TODO other cases of \i and \j?
// TODO capital cases like \I and \J?
static const struct DotlessIJCharacter {
    const char modifier;
    const char letter;
    QChar unicode;
}
dotlessIJCharacters[] = {
    {'`', 'i', QChar(0x00EC)},
    {'\'', 'i', QChar(0x00ED)},
    {'^', 'i', QChar(0x00EE)},
    {'"', 'i', QChar(0x00EF)},
    {'~', 'i', QChar(0x0129)},
    {'=', 'i', QChar(0x012B)},
    {'u', 'i', QChar(0x012D)},
    {'k', 'i', QChar(0x012F)},
    {'^', 'j', QChar(0x0135)},
    {'v', 'i', QChar(0x01D0)},
    {'v', 'j', QChar(0x01F0)},
    {'t', 'i', QChar(0x020B)}
};
static const int dotlessIJCharactersLen = sizeof(dotlessIJCharacters) / sizeof(dotlessIJCharacters[0]);


/**
 * This lookup allows to quickly find hits in the
 * EncoderLaTeXEscapedCharacter table. This data structure here
 * consists of a number of rows. Each row consists of a
 * modifier (like '"' or 'v') and an array of Unicode chars.
 * Letters 'A'..'Z','a'..'z' are used as index to this array
 * by substracting 'A', i.e. 'A' gets index 0, 'B' gets index 1,
 * etc.
 * This data structure is built in the constructor.
 */
static const int lookupTableNumModifiers = 32;
static const int lookupTableNumCharacters = 60;
static struct EncoderLaTeXEscapedCharacterLookupTableRow {
    char modifier;
    QChar unicode[lookupTableNumCharacters];
} *lookupTable[lookupTableNumModifiers];


/**
 * This data structure keeps track of math commands, which
 * have to be treated differently in text and math mode.
 * The math command like "subset of" could be used directly
 * in math mode, but must be enclosed in \ensuremath{...}
 * in text mode.
 */
static const struct MathCommand {
    QString written;
    QChar unicode;
}
mathCommand[] = {
    {QLatin1String("ell"), QChar(0x2113)},
    {QLatin1String("rightarrow"), QChar(0x2192)},
    {QLatin1String("forall"), QChar(0x2200)},
    {QLatin1String("exists"), QChar(0x2203)},
    {QLatin1String("in"), QChar(0x2208)},
    {QLatin1String("ni"), QChar(0x220B)},
    {QLatin1String("asterisk"), QChar(0x2217)},
    {QLatin1String("infty"), QChar(0x221E)},
    {QLatin1String("subset"), QChar(0x2282)},
    {QLatin1String("top"), QChar(0x22A4)},
};
static const int mathCommandLen = sizeof(mathCommand) / sizeof(mathCommand[0]);


/**
 * This data structure holds commands representing a single
 * character. For example, it maps \AA to A with a ring (Nordic
 * letter) and back. The structure is a table with two columns:
 * (1) the command's name without a backslash (in the example
 * before the 'AA') (2) the Unicode character described by a
 * hexcode.
 */
static const struct EncoderLaTeXCharacterCommand {
    QString letters;
    QChar unicode;
    bool meaningfulCommandName;
}
encoderLaTeXCharacterCommands[] = {
    {QLatin1String("pounds"), QChar(0x00A3), false},
    {QLatin1String("S"), QChar(0x00A7), true},
    {QLatin1String("copyright"), QChar(0x00A9), false},
    {QLatin1String("textperiodcentered"), QChar(0x00B7), false},
    {QLatin1String("AA"), QChar(0x00C5), true},
    {QLatin1String("AE"), QChar(0x00C6), true},
    {QLatin1String("DH"), QChar(0x00D0), true},
    {QLatin1String("DJ"), QChar(0x00D0), true},
    {QLatin1String("O"), QChar(0x00D8), true},
    {QLatin1String("TH"), QChar(0x00DE), true},
    {QLatin1String("ss"), QChar(0x00DF), true},
    {QLatin1String("aa"), QChar(0x00E5), true},
    {QLatin1String("ae"), QChar(0x00E6), true},
    {QLatin1String("dh"), QChar(0x00F0), true},
    {QLatin1String("o"), QChar(0x00F8), true},
    {QLatin1String("th"), QChar(0x00FE), true},
    {QLatin1String("i"), QChar(0x0131), true},
    {QLatin1String("NG"), QChar(0x014A), true},
    {QLatin1String("ng"), QChar(0x014B), true},
    {QLatin1String("L"), QChar(0x0141), true},
    {QLatin1String("l"), QChar(0x0142), true},
    {QLatin1String("OE"), QChar(0x0152), true},
    {QLatin1String("oe"), QChar(0x0153), true},
    {QLatin1String("j"), QChar(0x0237), true},
    {QLatin1String("ldots"), QChar(0x2026), false}, /** \ldots must be before \l */
    {QLatin1String("grqq"), QChar(0x201C), false},
    {QLatin1String("rqq"), QChar(0x201D), false},
    {QLatin1String("glqq"), QChar(0x201E), false},
    {QLatin1String("frqq"), QChar(0x00BB), false},
    {QLatin1String("flqq"), QChar(0x00AB), false},
    {QLatin1String("rq"), QChar(0x2019), false}, ///< tricky one: 'r' is a valid modifier
    {QLatin1String("lq"), QChar(0x2018), false},
};
static const int encoderLaTeXCharacterCommandsLen = sizeof(encoderLaTeXCharacterCommands) / sizeof(encoderLaTeXCharacterCommands[0]);

const char EncoderLaTeX::encoderLaTeXProtectedSymbols[] = {'#', '&', '%'};
const int EncoderLaTeX::encoderLaTeXProtectedSymbolsLen = sizeof(EncoderLaTeX::encoderLaTeXProtectedSymbols) / sizeof(EncoderLaTeX::encoderLaTeXProtectedSymbols[0]);

const char EncoderLaTeX::encoderLaTeXProtectedTextOnlySymbols[] = {'_'};
const int EncoderLaTeX::encoderLaTeXProtectedTextOnlySymbolsLen = sizeof(encoderLaTeXProtectedTextOnlySymbols) / sizeof(encoderLaTeXProtectedTextOnlySymbols[0]);


/**
 * This data structure holds LaTeX symbol sequences (without
 * any backslash) that represent a single Unicode character.
 * For example, it maps --- to an 'em dash' and back.
 * The structure is a table with two columns: (1) the symbol
 * sequence (in the example before the '---') (2) the Unicode
 * character described by a hexcode.
 */
static const struct EncoderLaTeXSymbolSequence {
    const char *latex;
    QChar unicode;
    bool useUnicode;
} encoderLaTeXSymbolSequences[] = {
    {"!`", QChar(0x00A1), true},
    {"\"<", QChar(0x00AB), true},
    {"\">", QChar(0x00BB), true},
    {"?`", QChar(0x00BF), true},
    {"---", QChar(0x2014), true},
    {"--", QChar(0x2013), true},
    {"``", QChar(0x201C), true},
    {"''", QChar(0x201D), true},
    {"ff", QChar(0xFB00), false},
    {"fi", QChar(0xFB01), false},
    {"fl", QChar(0xFB02), false},
    {"ffi", QChar(0xFB03), false},
    {"ffl", QChar(0xFB04), false},
    {"ft", QChar(0xFB05), false},
    {"st", QChar(0xFB06), false}
};
static const int encoderLaTeXSymbolSequencesLen = sizeof(encoderLaTeXSymbolSequences) / sizeof(encoderLaTeXSymbolSequences[0]);


EncoderLaTeX::EncoderLaTeX()
{
    /// Initialize lookup table with NULL pointers
    for (int i = 0; i < lookupTableNumModifiers; ++i) lookupTable[i] = NULL;

    int lookupTableCount = 0;
    /// Go through all table rows of encoderLaTeXEscapedCharacters
    for (int i = encoderLaTeXEscapedCharactersLen - 1; i >= 0; --i) {
        /// Check if this row's modifier is already known
        bool knownModifier = false;
        int j;
        for (j = lookupTableCount - 1; j >= 0; --j) {
            knownModifier |= lookupTable[j]->modifier == encoderLaTeXEscapedCharacters[i].modifier;
            if (knownModifier) break;
        }

        if (!knownModifier) {
            /// Ok, this row's modifier appeared for the first time,
            /// therefore initialize memory structure, i.e. row in lookupTable
            lookupTable[lookupTableCount] = new EncoderLaTeXEscapedCharacterLookupTableRow;
            lookupTable[lookupTableCount]->modifier = encoderLaTeXEscapedCharacters[i].modifier;
            for (int k = 0; k < lookupTableNumCharacters; ++k) {
                /// If no special character is known for a letter+modifier
                /// combination, fall back using the ASCII character only
                lookupTable[lookupTableCount]->unicode[k] = QChar('A' + k);
            }
            j = lookupTableCount;
            ++lookupTableCount;
        }

        /// Add the letter as of the current row in encoderLaTeXEscapedCharacters
        /// into Unicode char array in the current modifier's row in the lookup table.
        if (encoderLaTeXEscapedCharacters[i].letter >= 'A' && encoderLaTeXEscapedCharacters[i].letter <= 'z') {
            int pos = encoderLaTeXEscapedCharacters[i].letter - 'A';
            lookupTable[j]->unicode[pos] = encoderLaTeXEscapedCharacters[i].unicode;
        } else
            kWarning() << "Cannot handle letter " << encoderLaTeXEscapedCharacters[i].letter;
    }
}

EncoderLaTeX::~EncoderLaTeX()
{
    /// Clean-up memory
    for (int i = lookupTableNumModifiers - 1; i >= 0; --i)
        if (lookupTable[i] != NULL)
            delete lookupTable[i];
}

QString EncoderLaTeX::decode(const QString &input) const
{
    int len = input.length();
    QString output;
    output.reserve(len);
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

        /// Fetch current input char
        const QChar &c = input[i];

        if (c == '{') {
            /// First case: An opening curly bracket,
            /// which is harmless (see else case), unless ...
            if (i < len - 3 && input[i + 1] == '\\') {
                /// ... it continues with a backslash

                /// Next, check if there follows a modifier after the backslash
                /// For example an quotation mark as used in {\"a}
                int lookupTablePos = modifierInLookupTable(input[i + 2]);

                if (lookupTablePos >= 0 && input[i + 3] >= 'A' && input[i + 3] <= 'z' && input[i + 4] == '}') {
                    /// If we found a modifier which is followed by
                    /// a letter followed by a closing curly bracket,
                    /// we are looking at something like {\"A}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    output.append(lookupTable[lookupTablePos]->unicode[input[i + 3].toAscii() - 'A']);
                    /// Step over those additional characters
                    i += 4;
                } else if (lookupTablePos >= 0 && input[i + 3] == '\\' && input[i + 4] >= 'A' && input[i + 4] <= 'z' && input[i + 5] == '}') {
                    /// This is the case for {\'\i} or alike.
                    bool found = false;
                    for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                        if (dotlessIJCharacters[k].letter == input[i + 4] && dotlessIJCharacters[k].modifier == input[i + 2]) {
                            output.append(dotlessIJCharacters[k].unicode);
                            i += 5;
                            found = true;
                        }
                    if (!found)
                        kWarning() << "Cannot interprete BACKSLASH" << input[i + 2] << "BACKSLASH" << input[i + 4];
                } else if (lookupTablePos >= 0 && input[i + 3] == '{' && input[i + 4] >= 'A' && input[i + 4] <= 'z' && input[i + 5] == '}' && input[i + 6] == '}') {
                    /// If we found a modifier which is followed by
                    /// an opening curly bracket followed by a letter
                    /// followed by two closing curly brackets,
                    /// we are looking at something like {\"{A}}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    output.append(lookupTable[lookupTablePos]->unicode[input[i + 4].toAscii() - 'A']);
                    /// Step over those additional characters
                    i += 6;
                } else if (lookupTablePos >= 0 && input[i + 3] == '{' && input[i + 4] == '\\' && input[i + 5] >= 'A' && input[i + 5] <= 'z' && input[i + 6] == '}' && input[i + 7] == '}') {
                    /// This is the case for {\'{\i}} or alike.
                    bool found = false;
                    for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                        if (dotlessIJCharacters[k].letter == input[i + 5] && dotlessIJCharacters[k].modifier == input[i + 2]) {
                            output.append(dotlessIJCharacters[k].unicode);
                            i += 7;
                            found = true;
                        }
                    if (!found)
                        kWarning() << "Cannot interprete BACKSLASH" << input[i + 2] << "BACKSLASH {" << input[i + 5] << "}";
                } else {
                    /// Now, the case of something like {\AA} is left
                    /// to check for
                    QString alpha = readAlphaCharacters(input, i + 2);
                    int nextPosAfterAlpha = i + 2 + alpha.size();
                    if (input[nextPosAfterAlpha] == '}') {
                        /// We are dealing actually with a string like {\AA}
                        /// Check which command it is,
                        /// insert corresponding Unicode character
                        bool foundCommand = false;
                        for (int ci = 0; !foundCommand && ci < encoderLaTeXCharacterCommandsLen; ++ci) {
                            if (encoderLaTeXCharacterCommands[ci].letters == alpha) {
                                output.append(encoderLaTeXCharacterCommands[ci].unicode);
                                foundCommand = true;
                            }
                        }

                        /// Check if a math command has been read,
                        /// like \subset
                        /// (automatically skipped if command was found above)
                        // FIXME a math command could be inside \ensuremath, which is not tested here
                        // may lead to increasing numbers of nested \ensuremath commands
                        for (int k = 0; !foundCommand && k < mathCommandLen; ++k) {
                            if (mathCommand[k].written == alpha) {
                                output.append(mathCommand[k].unicode);
                                foundCommand = true;
                            }
                        }

                        if (foundCommand)
                            i = nextPosAfterAlpha;
                        else {
                            /// Dealing with a string line {\noopsort}
                            /// (see BibTeX documentation where this gets explained)
                            output.append(c);
                        }
                    } else {
                        /// Nothing special, copy input char to output
                        output.append(c);
                    }
                }
            } else {
                /// Nothing special, copy input char to output
                output.append(c);
            }
        } else if (c == '\\' && i < len - 1) {
            /// Second case: A backslash as in \"o

            /// Sometimes such command are closed with just {},
            /// so remember if to check for that
            bool checkForExtraCurlyAtEnd = false;

            /// Check if there follows a modifier after the backslash
            /// For example an quotation mark as used in \"a
            int lookupTablePos = modifierInLookupTable(input[i + 1]);

            if (lookupTablePos >= 0 && i <= len - 3 && input[i + 2] >= 'A' && input[i + 2] <= 'z' && (i == len - 3 || input[i + 1] == '"' || input[i + 1] == '\'' || input[i + 1] == '`' || input[i + 1] == '=')) { // TODO more special cases?
                /// We found a special modifier which is followed by
                /// a letter followed by a command delimiter such
                /// as a whitespace, so we are looking at something
                /// like \"u inside Kr\"uger
                /// Use lookup table to see what Unicode char this
                /// represents
                output.append(lookupTable[lookupTablePos]->unicode[input[i + 2].toAscii() - 'A']);
                /// Step over those additional characters
                i += 2;
            } else if (lookupTablePos >= 0 && i <= len - 3 && input[i + 2] >= 'A' && input[i + 2] <= 'z' && (i == len - 3 || input[i + 3] == '}' ||  input[i + 3] == '{' || input[i + 3] == ' ' || input[i + 3] == '\t' || input[i + 3] == '\\' || input[i + 3] == '\r' || input[i + 3] == '\n')) {
                /// We found a modifier which is followed by
                /// a letter followed by a command delimiter such
                /// as a whitespace, so we are looking at something
                /// like \"u
                /// Use lookup table to see what Unicode char this
                /// represents
                output.append(lookupTable[lookupTablePos]->unicode[input[i + 2].toAscii() - 'A']);
                /// Step over those additional characters
                i += 2;

                /// Now, after this command, a whitespace may follow
                /// which has to get "eaten" as it acts as a command
                /// delimiter
                if (input[i + 1] == ' ' || input[i + 1] == '\r' || input[i + 1] == '\n')
                    ++i;
                else {
                    /// If no whitespace follows, still
                    /// check for extra curly brackets
                    checkForExtraCurlyAtEnd = true;
                }
            } else if (lookupTablePos >= 0 && i < len - 4 && input[i + 2] == '{' && input[i + 3] >= 'A' && input[i + 3] <= 'z' && input[i + 4] == '}') {
                /// We found a modifier which is followed by an opening
                /// curly bracket followed a letter followed by a closing
                /// curly bracket, so we are looking at something
                /// like \"{u}
                /// Use lookup table to see what Unicode char this
                /// represents
                output.append(lookupTable[lookupTablePos]->unicode[input[i + 3].toAscii() - 'A']);
                /// Step over those additional characters
                i += 4;
            } else if (lookupTablePos >= 0 && input[i + 2] == '\\' && input[i + 3] >= 'A' && input[i + 3] <= 'z') {
                /// This is the case for \'\i or alike.
                bool found = false;
                for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                    if (dotlessIJCharacters[k].letter == input[i + 3] && dotlessIJCharacters[k].modifier == input[i + 1]) {
                        output.append(dotlessIJCharacters[k].unicode);
                        i += 3;
                        found = true;
                    }
                if (!found)
                    kWarning() << "Cannot interprete BACKSLASH" << input[i + 1] << "BACKSLASH" << input[i + 3];
            } else if (lookupTablePos >= 0 && input[i + 2] == '{' && input[i + 3] == '\\' && input[i + 4] >= 'A' && input[i + 4] <= 'z' && input[i + 5] == '}') {
                /// This is the case for \'{\i} or alike.
                bool found = false;
                for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                    if (dotlessIJCharacters[k].letter == input[i + 4] && dotlessIJCharacters[k].modifier == input[i + 1]) {
                        output.append(dotlessIJCharacters[k].unicode);
                        i += 5;
                        found = true;
                    }
                if (!found)
                    kWarning() << "Cannot interprete BACKSLASH" << input[i + 1] << "BACKSLASH {" << input[i + 4] << "}";
            } else if (i < len - 1) {
                /// Now, the case of something like \AA is left
                /// to check for
                QString alpha = readAlphaCharacters(input, i + 1);
                int nextPosAfterAlpha = i + 1 + alpha.size();
                if (alpha.size() > 1) {
                    /// We are dealing actually with a string like \AA
                    /// Check which command it is,
                    /// insert corresponding Unicode character
                    bool foundCommand = false;
                    for (int ci = 0; !foundCommand && ci < encoderLaTeXCharacterCommandsLen; ++ci) {
                        if (encoderLaTeXCharacterCommands[ci].letters == alpha) {
                            output.append(encoderLaTeXCharacterCommands[ci].unicode);
                            foundCommand = true;
                        }
                    }

                    if (foundCommand) {
                        /// Now, after a command, a whitespace may follow
                        /// which has to get "eaten" as it acts as a command
                        /// delimiter
                        if (nextPosAfterAlpha < input.length() && (input[nextPosAfterAlpha] == ' ' || input[nextPosAfterAlpha] == '\r' || input[nextPosAfterAlpha] == '\n'))
                            ++nextPosAfterAlpha;
                        else {
                            /// If no whitespace follows, still
                            /// check for extra curly brackets
                            checkForExtraCurlyAtEnd = true;
                        }
                        i = nextPosAfterAlpha - 1;
                    } else {
                        /// No command found? Just copy input char to output
                        output.append(c);
                    }
                } else {
                    /// Maybe we are dealing with a string like \& or \_
                    /// Check which command it is
                    bool foundCommand = false;
                    for (int k = 0; k < encoderLaTeXProtectedSymbolsLen; ++k)
                        if (encoderLaTeXProtectedSymbols[k] == input[i + 1]) {
                            output.append(encoderLaTeXProtectedSymbols[k]);
                            foundCommand = true;
                        }

                    if (!foundCommand && !inMathMode)
                        for (int k = 0; k < encoderLaTeXProtectedTextOnlySymbolsLen; ++k)
                            if (encoderLaTeXProtectedTextOnlySymbols[k] == input[i + 1]) {
                                output.append(encoderLaTeXProtectedTextOnlySymbols[k]);
                                foundCommand = true;
                            }

                    /// If command has been found, nothing has to be done
                    /// except for hopping over this backslash
                    if (foundCommand)
                        ++i;
                    else {
                        /// Nothing special, copy input char to output
                        output.append(c);
                    }
                }
            } else {
                /// Nothing special, copy input char to output
                output.append(c);
            }

            /// Finally, check if there may be extra curly brackets
            /// like {} and hop over them
            if (checkForExtraCurlyAtEnd && i < len - 2 && input[i + 1] == '{' && input[i + 2] == '}') i += 2;
        } else {
            /// So far, no opening curly bracket and no backslash
            /// May still be a symbol sequence like ---
            bool isSymbolSequence = false;
            /// Go through all known symbol sequnces
            for (int l = 0; l < encoderLaTeXSymbolSequencesLen; ++l)
                /// First, check if read input character matches beginning of symbol sequence
                /// and input buffer as enough characters left to potentially contain
                /// symbol sequence
                if (encoderLaTeXSymbolSequences[l].useUnicode && encoderLaTeXSymbolSequences[l].latex[0] == c && i <= len - (int)qstrlen(encoderLaTeXSymbolSequences[l].latex)) {
                    /// Now actually check if symbol sequence is in input buffer
                    isSymbolSequence = true;
                    for (int p = 1; isSymbolSequence && p < (int)qstrlen(encoderLaTeXSymbolSequences[l].latex); ++p)
                        isSymbolSequence &= encoderLaTeXSymbolSequences[l].latex[p] == input[i + p];
                    if (isSymbolSequence) {
                        /// Ok, found sequence: insert Unicode character in output
                        /// and hop over sequence in input buffer
                        output.append(encoderLaTeXSymbolSequences[l].unicode);
                        i += qstrlen(encoderLaTeXSymbolSequences[l].latex) - 1;
                        break;
                    }
                }

            if (!isSymbolSequence) {
                /// No symbol sequence found, so just copy input to output
                output.append(c);

                /// Still, check if input character is a dollar sign
                /// without a preceeding backslash, means toggling between
                /// text mode and math mode
                if (c == '$' && (i == 0 || input[i - 1] != '\\'))
                    inMathMode = !inMathMode;
            }
        }
    }

    output.squeeze();
    return output;
}

bool EncoderLaTeX::testAndCopyVerbatimCommands(const QString &input, int &pos, QString &output) const
{
    int copyBytesCount = 0;
    int openedClosedCurlyBrackets = 0;

    /// check for \url
    if (pos < input.length() - 6 && input[pos] == '\\' && input[pos + 1] == 'u' && input[pos + 2] == 'r' && input[pos + 3] == 'l' && input[pos + 4] == '{') {
        copyBytesCount = 5;
        openedClosedCurlyBrackets = 1;
    }

    if (copyBytesCount > 0) {
        while (openedClosedCurlyBrackets > 0 && pos + copyBytesCount < input.length()) {
            ++copyBytesCount;
            if (input[pos + copyBytesCount] == '{' && input[pos + copyBytesCount - 1] != '\\') ++openedClosedCurlyBrackets;
            else if (input[pos + copyBytesCount] == '}' && input[pos + copyBytesCount - 1] != '\\') --openedClosedCurlyBrackets;
        }

        output.append(input.mid(pos, copyBytesCount));
        pos += copyBytesCount;
    }

    return copyBytesCount > 0;
}

QString EncoderLaTeX::encode(const QString &input) const
{
    int len = input.length();
    QString output;
    output.reserve(len);
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

        if (c.unicode() > 127) {
            /// If current char is outside ASCII boundaries ...
            bool found = false;

            /// Handle special cases of i without a dot (\i)
            for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                if (c.unicode() == dotlessIJCharacters[k].unicode) {
                    output.append(QString("\\%1{\\%2}").arg(dotlessIJCharacters[k].modifier).arg(dotlessIJCharacters[k].letter));
                    found = true;
                }

            if (!found) {
                /// ... test if there is a symbol sequence like ---
                /// to encode it
                for (int k = 0; k < encoderLaTeXSymbolSequencesLen; ++k)
                    if (encoderLaTeXSymbolSequences[k].unicode == c) {
                        for (int l = 0; l < (int)qstrlen(encoderLaTeXSymbolSequences[k].latex); ++l)
                            output.append(encoderLaTeXSymbolSequences[k].latex[l]);
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, no symbol sequence. Let's test character
                /// commands like \ss
                for (int k = 0; k < encoderLaTeXCharacterCommandsLen; ++k)
                    if (encoderLaTeXCharacterCommands[k].unicode == c) {
                        output.append(QString("{\\%1}").arg(encoderLaTeXCharacterCommands[k].letters));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, neither a character command. Let's test
                /// escaped characters with modifiers like \"a
                for (int k = 0; k < encoderLaTeXEscapedCharactersLen; ++k)
                    if (encoderLaTeXEscapedCharacters[k].unicode == c) {
                        output.append(QString("\\%1{%2}").arg(encoderLaTeXEscapedCharacters[k].modifier).arg(encoderLaTeXEscapedCharacters[k].letter));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, test for math commands
                for (int k = 0; k < mathCommandLen; ++k)
                    if (mathCommand[k].unicode == c) {
                        if (inMathMode)
                            output.append(QString("\\%1{}").arg(mathCommand[k].written));
                        else
                            output.append(QString("\\ensuremath{\\%1}").arg(mathCommand[k].written));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                kWarning() << "Don't know how to encode Unicode char" << QString("0x%1").arg(c.unicode(), 4, 16, QLatin1Char('0'));
                output.append(c);
            }
        } else {
            /// Current character is normal ASCII

            /// Still, some characters have special meaning
            /// in TeX and have to be preceeded with a backslash
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
            /// without a preceeding backslash, means toggling between
            /// text mode and math mode
            if (c == '$' && (i == 0 || input[i - 1] != QChar('\\')))
                inMathMode = !inMathMode;
        }
    }

    output.squeeze();
    return output;
}

QString EncoderLaTeX::convertToPlainAscii(const QString &input) const
{
    int len = input.length();
    QString output;
    output.reserve(len);

    /// Go through input char by char
    for (int i = 0; i < len; ++i) {
        const QChar c = input[i];

        if (c.unicode() > 127) {
            /// If current char is outside ASCII boundaries ...

            /// ... test if there is a symbol sequence like ---
            /// to encode it
            bool found = false;

            /// Let's test character commands like \'\i
            for (int k = 0; k < dotlessIJCharactersLen; ++k)
                if (dotlessIJCharacters[k].unicode == c) {
                    output.append(dotlessIJCharacters[k].letter);
                    found = true;
                    break;
                }

            /// Let's test character commands like \ss
            for (int k = 0; k < encoderLaTeXCharacterCommandsLen; ++k)
                if (encoderLaTeXCharacterCommands[k].meaningfulCommandName && encoderLaTeXCharacterCommands[k].unicode == c) {
                    output.append(encoderLaTeXCharacterCommands[k].letters);
                    found = true;
                    break;
                }

            if (!found) {
                /// Ok, not a character command. Let's test
                /// escaped characters with modifiers like \"a
                for (int k = 0; k < encoderLaTeXEscapedCharactersLen; ++k)
                    if (encoderLaTeXEscapedCharacters[k].unicode == c) {
                        output.append(encoderLaTeXEscapedCharacters[k].letter);
                        found = true;
                        break;
                    }
            }

            if (!found) {
                kDebug() << "Don't know how to convert to plain ASCII this Unicode char: " << QString("0x%1").arg(c.unicode(), 4, 16, QLatin1Char('0'));
                output.append(QLatin1Char('X'));
            }
        } else {
            /// Current character is normal ASCII

            /// Dump character to output
            output.append(c);
        }
    }

    output.squeeze();
    return output;
}

bool EncoderLaTeX::containsOnlyAscii(const QString &text)
{
    for (QString::ConstIterator it = text.constBegin(); it != text.constEnd(); ++it)
        if (it->unicode() > 127) return false;
    return true;
}

int EncoderLaTeX::modifierInLookupTable(const QChar &c) const
{
    for (int m = 0; m < lookupTableNumModifiers && lookupTable[m] != NULL; ++m)
        if (lookupTable[m]->modifier == c) return m;
    return -1;
}

QString EncoderLaTeX::readAlphaCharacters(const QString &base, int startFrom) const
{
    int len = base.size();
    for (int j = startFrom; j < len; ++j) {
        if ((base[j] < 'A' || base[j] > 'Z') && (base[j] < 'a' || base[j] > 'z'))
            return base.mid(startFrom, j - startFrom);
    }
    return base.mid(startFrom);
}

EncoderLaTeX *EncoderLaTeX::instance()
{
    if (self == NULL)
        self = new EncoderLaTeX();
    return self;
}
