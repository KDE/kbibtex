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

#include "encoderlatex.h"

#include <QString>

#include "logging_io.h"

inline bool isAsciiLetter(const QChar c) {
    return ((c >= QLatin1Char('A') && c <= QLatin1Char('Z')) || (c >= QLatin1Char('a') && c <= QLatin1Char('z')));
}

inline bool isAsciiLetter(const char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

/**
 * General documentation on this topic:
 *   http://www.tex.ac.uk/CTAN/macros/latex/doc/encguide.pdf
 */

/**
 * This structure contains information how escaped characters
 * such as \"a are translated to an Unicode character and back.
 * The structure is a table with three columns: (1) the modifier
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
    ushort unicode;
}
encoderLaTeXEscapedCharacters[] = {
    {'`', 'A', 0x00C0},
    {'\'', 'A', 0x00C1},
    {'^', 'A', 0x00C2},
    {'~', 'A', 0x00C3},
    {'"', 'A', 0x00C4},
    {'r', 'A', 0x00C5},
    /** 0x00C6 */
    {'c', 'C', 0x00C7},
    {'`', 'E', 0x00C8},
    {'\'', 'E', 0x00C9},
    {'^', 'E', 0x00CA},
    {'"', 'E', 0x00CB},
    {'`', 'I', 0x00CC},
    {'\'', 'I', 0x00CD},
    {'^', 'I', 0x00CE},
    {'"', 'I', 0x00CF},
    /** 0x00D0: see EncoderLaTeXCharacterCommand */
    {'~', 'N', 0x00D1},
    {'`', 'O', 0x00D2},
    {'\'', 'O', 0x00D3},
    {'^', 'O', 0x00D4},
    {'~', 'O', 0x00D5},
    {'"', 'O', 0x00D6},
    /** 0x00D7 */
    /** 0x00D8: see EncoderLaTeXCharacterCommand */
    {'`', 'U', 0x00D9},
    {'\'', 'U', 0x00DA},
    {'^', 'U', 0x00DB},
    {'"', 'U', 0x00DC},
    {'\'', 'Y', 0x00DD},
    /** 0x00DE */
    {'"', 's', 0x00DF},
    {'`', 'a', 0x00E0},
    {'\'', 'a', 0x00E1},
    {'^', 'a', 0x00E2},
    {'~', 'a', 0x00E3},
    {'"', 'a', 0x00E4},
    {'r', 'a', 0x00E5},
    /** 0x00E6 */
    {'c', 'c', 0x00E7},
    {'`', 'e', 0x00E8},
    {'\'', 'e', 0x00E9},
    {'^', 'e', 0x00EA},
    {'"', 'e', 0x00EB},
    {'`', 'i', 0x00EC},
    {'\'', 'i', 0x00ED},
    {'^', 'i', 0x00EE},
    {'"', 'i', 0x00EF},
    /** 0x00F0 */
    {'~', 'n', 0x00F1},
    {'`', 'o', 0x00F2},
    {'\'', 'o', 0x00F3},
    {'^', 'o', 0x00F4},
    {'~', 'o', 0x00F5},
    {'"', 'o', 0x00F6},
    /** 0x00F7 Division Sign */
    /** 0x00F8: see EncoderLaTeXCharacterCommand */
    {'`', 'u', 0x00F9},
    {'\'', 'u', 0x00FA},
    {'^', 'u', 0x00FB},
    {'"', 'u', 0x00FC},
    {'\'', 'y', 0x00FD},
    /** 0x00FE Thorn */
    {'"', 'y', 0x00FF},
    {'=', 'A', 0x0100},
    {'=', 'a', 0x0101},
    {'u', 'A', 0x0102},
    {'u', 'a', 0x0103},
    {'c', 'A', 0x0104},
    {'c', 'a', 0x0105},
    {'\'', 'C', 0x0106},
    {'\'', 'c', 0x0107},
    /** 0x0108 */
    /** 0x0109 */
    {'.', 'C', 0x010A},
    {'.', 'c', 0x010B},
    {'v', 'C', 0x010C},
    {'v', 'c', 0x010D},
    {'v', 'D', 0x010E},
    /** 0x010F */
    /** 0x0110 */
    /** 0x0111 */
    {'=', 'E', 0x0112},
    {'=', 'e', 0x0113},
    /** 0x0114 */
    /** 0x0115 */
    {'.', 'E', 0x0116},
    {'.', 'e', 0x0117},
    {'c', 'E', 0x0118},
    {'c', 'e', 0x0119},
    {'v', 'E', 0x011A},
    {'v', 'e', 0x011B},
    /** 0x011C */
    /** 0x011D */
    {'u', 'G', 0x011E},
    {'u', 'g', 0x011F},
    {'.', 'G', 0x0120},
    {'.', 'g', 0x0121},
    /** 0x0122 */
    /** 0x0123 */
    /** 0x0124 */
    /** 0x0125 */
    /** 0x0126 */
    /** 0x0127 */
    {'~', 'I', 0x0128},
    {'~', 'i', 0x0129},
    {'=', 'I', 0x012A},
    {'=', 'i', 0x012B},
    {'u', 'I', 0x012C},
    {'u', 'i', 0x012D},
    /** 0x012E */
    /** 0x012F */
    {'.', 'I', 0x0130},
    /** 0x0131 */
    /** 0x0132 */
    /** 0x0133 */
    /** 0x0134 */
    /** 0x0135 */
    /** 0x0136 */
    /** 0x0137 */
    /** 0x0138 */
    {'\'', 'L', 0x0139},
    {'\'', 'l', 0x013A},
    /** 0x013B */
    /** 0x013C */
    /** 0x013D */
    /** 0x013E */
    /** 0x013F */
    /** 0x0140 */
    /** 0x0141 */
    /** 0x0142 */
    {'\'', 'N', 0x0143},
    {'\'', 'n', 0x0144},
    /** 0x0145 */
    /** 0x0146 */
    {'v', 'N', 0x0147},
    {'v', 'n', 0x0148},
    /** 0x0149 */
    /** 0x014A */
    /** 0x014B */
    {'=', 'O', 0x014C},
    {'=', 'o', 0x014D},
    {'u', 'O', 0x014E},
    {'u', 'o', 0x014F},
    {'H', 'O', 0x0150},
    {'H', 'o', 0x0151},
    /** 0x0152 */
    /** 0x0153 */
    {'\'', 'R', 0x0154},
    {'\'', 'r', 0x0155},
    /** 0x0156 */
    /** 0x0157 */
    {'v', 'R', 0x0158},
    {'v', 'r', 0x0159},
    {'\'', 'S', 0x015A},
    {'\'', 's', 0x015B},
    /** 0x015C */
    /** 0x015D */
    {'c', 'S', 0x015E},
    {'c', 's', 0x015F},
    {'v', 'S', 0x0160},
    {'v', 's', 0x0161},
    /** 0x0162 */
    /** 0x0163 */
    {'v', 'T', 0x0164},
    /** 0x0165 */
    /** 0x0166 */
    /** 0x0167 */
    {'~', 'U', 0x0168},
    {'~', 'u', 0x0169},
    {'=', 'U', 0x016A},
    {'=', 'u', 0x016B},
    {'u', 'U', 0x016C},
    {'u', 'u', 0x016D},
    {'r', 'U', 0x016E},
    {'r', 'u', 0x016F},
    {'H', 'U', 0x0170},
    {'H', 'u', 0x0171},
    /** 0x0172 */
    /** 0x0173 */
    /** 0x0174 */
    /** 0x0175 */
    /** 0x0176 */
    /** 0x0177 */
    {'"', 'Y', 0x0178},
    {'\'', 'Z', 0x0179},
    {'\'', 'z', 0x017A},
    {'.', 'Z', 0x017B},
    {'.', 'z', 0x017C},
    {'v', 'Z', 0x017D},
    {'v', 'z', 0x017E},
    /** 0x017F */
    /** 0x0180 */
    {'v', 'A', 0x01CD},
    {'v', 'a', 0x01CE},
    {'v', 'G', 0x01E6},
    {'v', 'g', 0x01E7},
    {'\'', 'F', 0x01F4},
    {'\'', 'f', 0x01F5},
    {'.', 'A', 0x0226},
    {'.', 'a', 0x0227},
    {'=', 'Y', 0x0232},
    {'=', 'y', 0x0233},
    {'.', 'O', 0x022E},
    {'.', 'o', 0x022F},
    {'.', 'B', 0x1E02},
    {'.', 'b', 0x1E03},
    {'d', 'B', 0x1E04},
    {'d', 'b', 0x1E05},
    {'.', 'D', 0x1E0A},
    {'.', 'd', 0x1E0B},
    {'d', 'D', 0x1E0C},
    {'d', 'd', 0x1E0D},
    {'.', 'E', 0x1E1E},
    {'.', 'e', 0x1E1F},
    {'.', 'H', 0x1E22},
    {'.', 'h', 0x1E23},
    {'d', 'H', 0x1E24},
    {'d', 'h', 0x1E25},
    {'"', 'H', 0x1E26},
    {'"', 'h', 0x1E27},
    {'d', 'K', 0x1E32},
    {'d', 'k', 0x1E33},
    {'d', 'L', 0x1E36},
    {'d', 'l', 0x1E37},
    {'.', 'M', 0x1E40},
    {'.', 'm', 0x1E41},
    {'d', 'M', 0x1E42},
    {'d', 'm', 0x1E43},
    {'.', 'N', 0x1E44},
    {'.', 'n', 0x1E45},
    {'.', 'N', 0x1E46},
    {'.', 'n', 0x1E47},
    {'.', 'P', 0x1E56},
    {'.', 'p', 0x1E57},
    {'.', 'R', 0x1E58},
    {'.', 'r', 0x1E59},
    {'d', 'R', 0x1E5A},
    {'d', 'r', 0x1E5B},
    {'.', 'S', 0x1E60},
    {'.', 's', 0x1E61},
    {'d', 'S', 0x1E62},
    {'d', 's', 0x1E63},
    {'.', 'T', 0x1E6A},
    {'.', 't', 0x1E6B},
    {'d', 'T', 0x1E6C},
    {'d', 't', 0x1E6D},
    {'d', 'V', 0x1E7E},
    {'d', 'v', 0x1E7F},
    {'`', 'W', 0x1E80},
    {'`', 'w', 0x1E81},
    {'\'', 'W', 0x1E82},
    {'\'', 'w', 0x1E83},
    {'"', 'W', 0x1E84},
    {'"', 'w', 0x1E85},
    {'.', 'W', 0x1E86},
    {'.', 'w', 0x1E87},
    {'d', 'W', 0x1E88},
    {'d', 'w', 0x1E88},
    {'.', 'X', 0x1E8A},
    {'.', 'x', 0x1E8B},
    {'"', 'X', 0x1E8C},
    {'"', 'x', 0x1E8D},
    {'.', 'Y', 0x1E8E},
    {'.', 'y', 0x1E8F},
    {'d', 'Z', 0x1E92},
    {'d', 'z', 0x1E93},
    {'"', 't', 0x1E97},
    {'r', 'w', 0x1E98},
    {'r', 'y', 0x1E99},
    {'d', 'A', 0x1EA0},
    {'d', 'a', 0x1EA1},
    {'d', 'E', 0x1EB8},
    {'d', 'e', 0x1EB9},
    {'d', 'I', 0x1ECA},
    {'d', 'i', 0x1ECB},
    {'d', 'O', 0x1ECC},
    {'d', 'o', 0x1ECD},
    {'d', 'U', 0x1EE4},
    {'d', 'u', 0x1EE5},
    {'`', 'Y', 0x1EF2},
    {'`', 'y', 0x1EF3},
    {'d', 'Y', 0x1EF4},
    {'d', 'y', 0x1EF5},
    {'r', 'q', 0x2019} ///< tricky: this is \rq
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
    ushort unicode;
}
dotlessIJCharacters[] = {
    {'`', 'i', 0x00EC},
    {'\'', 'i', 0x00ED},
    {'^', 'i', 0x00EE},
    {'"', 'i', 0x00EF},
    {'~', 'i', 0x0129},
    {'=', 'i', 0x012B},
    {'u', 'i', 0x012D},
    {'k', 'i', 0x012F},
    {'^', 'j', 0x0135},
    {'v', 'i', 0x01D0},
    {'v', 'j', 0x01F0},
    {'t', 'i', 0x020B}
};
static const int dotlessIJCharactersLen = sizeof(dotlessIJCharacters) / sizeof(dotlessIJCharacters[0]);


/**
 * This lookup allows to quickly find hits in the
 * EncoderLaTeXEscapedCharacter table. This data structure here
 * consists of a number of rows. Each row consists of a
 * modifier (like '"' or 'v') and an array of Unicode chars.
 * Letters 'A'..'Z','a'..'z' are used as index to this array
 * by subtracting 'A', i.e. 'A' gets index 0, 'B' gets index 1,
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
    const char *written;
    ushort unicode;
}
mathCommand[] = {
    {"ell", 0x2113},
    {"rightarrow", 0x2192},
    {"forall", 0x2200},
    {"exists", 0x2203},
    {"in", 0x2208},
    {"ni", 0x220B},
    {"asterisk", 0x2217},
    {"infty", 0x221E},
    {"subset", 0x2282},
    {"supset", 0x2283},
    {"subseteq", 0x2286},
    {"supseteq", 0x2287},
    {"nsubseteq", 0x2288},
    {"nsupseteq", 0x2289},
    {"subsetneq", 0x228A},
    {"supsetneq", 0x228A},
    {"Subset", 0x22D0},
    {"Supset", 0x22D1},
    {"top", 0x22A4},
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
    const char *letters;
    ushort unicode;
}
encoderLaTeXCharacterCommands[] = {
    {"pounds", 0x00A3},
    {"S", 0x00A7},
    {"copyright", 0x00A9},
    {"textregistered", 0x00AE},
    {"textperiodcentered", 0x00B7},
    {"AA", 0x00C5},
    {"AE", 0x00C6},
    {"DH", 0x00D0},
    {"DJ", 0x00D0},
    {"O", 0x00D8},
    {"TH", 0x00DE},
    {"ss", 0x00DF},
    {"aa", 0x00E5},
    {"ae", 0x00E6},
    {"dh", 0x00F0},
    {"o", 0x00F8},
    {"th", 0x00FE},
    {"i", 0x0131},
    {"NG", 0x014A},
    {"ng", 0x014B},
    {"L", 0x0141},
    {"l", 0x0142},
    {"OE", 0x0152},
    {"oe", 0x0153},
    {"j", 0x0237},
    {"ldots", 0x2026}, /** \ldots must be before \l */
    {"grqq", 0x201C},
    {"rqq", 0x201D},
    {"glqq", 0x201E},
    {"frqq", 0x00BB},
    {"flqq", 0x00AB},
    {"rq", 0x2019}, ///< tricky one: 'r' is a valid modifier
    {"lq", 0x2018}
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
    ushort unicode;
    bool useUnicode;
} encoderLaTeXSymbolSequences[] = {
    {"!`", 0x00A1, true},
    {"\"<", 0x00AB, true},
    {"\">", 0x00BB, true},
    {"?`", 0x00BF, true},
    {"---", 0x2014, true},
    {"--", 0x2013, true},
    {"``", 0x201C, true},
    {"''", 0x201D, true},
    {"ff", 0xFB00, false},
    {"fi", 0xFB01, false},
    {"fl", 0xFB02, false},
    {"ffi", 0xFB03, false},
    {"ffl", 0xFB04, false},
    {"ft", 0xFB05, false},
    {"st", 0xFB06, false}
};
static const int encoderLaTeXSymbolSequencesLen = sizeof(encoderLaTeXSymbolSequences) / sizeof(encoderLaTeXSymbolSequences[0]);


EncoderLaTeX::EncoderLaTeX()
{
#ifdef HAVE_ICU
    /// Create an ICO Transliterator, configured to
    /// transliterate virtually anything into plain ASCII
    UErrorCode uec = U_ZERO_ERROR;
    m_trans = icu::Transliterator::createInstance("Any-Latin;Latin-ASCII", UTRANS_FORWARD, uec);
#endif // HAVE_ICU

    /// Initialize lookup table with NULL pointers
    for (int i = 0; i < lookupTableNumModifiers; ++i) lookupTable[i] = nullptr;

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
                lookupTable[lookupTableCount]->unicode[k] = QLatin1Char('A' + k);
            }
            j = lookupTableCount;
            ++lookupTableCount;
        }

        /// Add the letter as of the current row in encoderLaTeXEscapedCharacters
        /// into Unicode char array in the current modifier's row in the lookup table.
        if (isAsciiLetter(encoderLaTeXEscapedCharacters[i].letter)) {
            int pos = encoderLaTeXEscapedCharacters[i].letter - 'A';
            lookupTable[j]->unicode[pos] = QChar(encoderLaTeXEscapedCharacters[i].unicode);
        } else
            qCWarning(LOG_KBIBTEX_IO) << "Cannot handle letter " << encoderLaTeXEscapedCharacters[i].letter;
    }
}

EncoderLaTeX::~EncoderLaTeX()
{
    /// Clean-up memory
    for (int i = lookupTableNumModifiers - 1; i >= 0; --i)
        if (lookupTable[i] != nullptr)
            delete lookupTable[i];

#ifdef HAVE_ICU
    if (m_trans != nullptr)
        delete m_trans;
#endif // HAVE_ICU
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
                int lookupTablePos = modifierInLookupTable(input[i + 2].toLatin1());

                /// Check for spaces between modifier and character, for example
                /// like {\H o}
                int skipSpaces = 0;
                while (i + 3 + skipSpaces < len && input[i + 3 + skipSpaces] == ' ' && skipSpaces < 16) ++skipSpaces;

                if (lookupTablePos >= 0 && i + skipSpaces < len - 4 && isAsciiLetter(input[i + 3 + skipSpaces]) && input[i + 4 + skipSpaces] == '}') {
                    /// If we found a modifier which is followed by
                    /// a letter followed by a closing curly bracket,
                    /// we are looking at something like {\"A}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[input[i + 3 + skipSpaces].toLatin1() - 'A'];
                    if (unicodeLetter.unicode() < 127) {
                        /// This combination of modifier and letter is not known,
                        /// so try to preserve it
                        output.append(input.midRef(i, 5 + skipSpaces));
                        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 5 + skipSpaces);
                    } else
                        output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 4 + skipSpaces;
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 5 && input[i + 3 + skipSpaces] == '\\' && isAsciiLetter(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == '}') {
                    /// This is the case for {\'\i} or alike.
                    bool found = false;
                    for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                        if (dotlessIJCharacters[k].letter == input[i + 4 + skipSpaces].toLatin1() && dotlessIJCharacters[k].modifier == input[i + 2].toLatin1()) {
                            output.append(QChar(dotlessIJCharacters[k].unicode));
                            i += 5 + skipSpaces;
                            found = true;
                        }
                    if (!found)
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 2] << "BACKSLASH" << input[i + 4 + skipSpaces];
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 6 && input[i + 3 + skipSpaces] == '{' && isAsciiLetter(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == '}' && input[i + 6 + skipSpaces] == '}') {
                    /// If we found a modifier which is followed by
                    /// an opening curly bracket followed by a letter
                    /// followed by two closing curly brackets,
                    /// we are looking at something like {\"{A}}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[input[i + 4 + skipSpaces].toLatin1() - 'A'];
                    if (unicodeLetter.unicode() < 127) {
                        /// This combination of modifier and letter is not known,
                        /// so try to preserve it
                        output.append(input.midRef(i, 7 + skipSpaces));
                        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 7 + skipSpaces);
                    } else
                        output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 6 + skipSpaces;
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 7 && input[i + 3 + skipSpaces] == '{' && input[i + 4 + skipSpaces] == '\\' && isAsciiLetter(input[i + 5 + skipSpaces]) && input[i + 6 + skipSpaces] == '}' && input[i + 7 + skipSpaces] == '}') {
                    /// This is the case for {\'{\i}} or alike.
                    bool found = false;
                    for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                        if (dotlessIJCharacters[k].letter == input[i + 5 + skipSpaces].toLatin1() && dotlessIJCharacters[k].modifier == input[i + 2].toLatin1()) {
                            output.append(QChar(dotlessIJCharacters[k].unicode));
                            i += 7 + skipSpaces;
                            found = true;
                        }
                    if (!found)
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 2] << "BACKSLASH {" << input[i + 5 + skipSpaces] << "}";
                } else {
                    /// Now, the case of something like {\AA} is left
                    /// to check for
                    QString alpha = readAlphaCharacters(input, i + 2);
                    int nextPosAfterAlpha = i + 2 + alpha.size();
                    if (nextPosAfterAlpha < input.length() && input[nextPosAfterAlpha] == '}') {
                        /// We are dealing actually with a string like {\AA}
                        /// Check which command it is,
                        /// insert corresponding Unicode character
                        bool foundCommand = false;
                        for (int ci = 0; !foundCommand && ci < encoderLaTeXCharacterCommandsLen; ++ci) {
                            if (QLatin1String(encoderLaTeXCharacterCommands[ci].letters) == alpha) /** note: QStringLiteral won't work here (?) */ {
                                output.append(QChar(encoderLaTeXCharacterCommands[ci].unicode));
                                foundCommand = true;
                            }
                        }

                        /// Check if a math command has been read,
                        /// like \subset
                        /// (automatically skipped if command was found above)
                        for (int k = 0; !foundCommand && k < mathCommandLen; ++k) {
                            if (QLatin1String(mathCommand[k].written) == alpha) /** note: QStringLiteral won't work here (?) */ {
                                if (output.endsWith(QStringLiteral("\\ensuremath"))) {
                                    /// Remove "\ensuremath" right before this math command,
                                    /// it will be re-inserted when exporting/saving the document
                                    output = output.left(output.length() - 11);
                                }
                                output.append(QChar(mathCommand[k].unicode));
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
                        /// Could be something like {\tt filename.txt}
                        /// Keep it as it is
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
            int lookupTablePos = modifierInLookupTable(input[i + 1].toLatin1());

            /// Check for spaces between modifier and character, for example
            /// like \H o
            int skipSpaces = 0;
            while (i + 2 + skipSpaces < len && input[i + 2 + skipSpaces] == ' ' && skipSpaces < 16) ++skipSpaces;

            if (lookupTablePos >= 0 && i + skipSpaces <= len - 3 && isAsciiLetter(input[i + 2 + skipSpaces]) && (i + skipSpaces == len - 3 || input[i + 1] == '"' || input[i + 1] == '\'' || input[i + 1] == '`' || input[i + 1] == '=')) { // TODO more special cases?
                /// We found a special modifier which is followed by
                /// a letter followed by normal text without any
                /// delimiter, so we are looking at something like
                /// \"u inside Kr\"uger
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[input[i + 2 + skipSpaces].toLatin1() - 'A'];
                if (unicodeLetter.unicode() < 127) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(input.midRef(i, 3 + skipSpaces));
                    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 3 + skipSpaces);
                } else
                    output.append(unicodeLetter);
                /// Step over those additional characters
                i += 2 + skipSpaces;
            } else if (lookupTablePos >= 0 && i + skipSpaces <= len - 3 && i + skipSpaces <= len - 3 && isAsciiLetter(input[i + 2 + skipSpaces]) && (i + skipSpaces == len - 3 || input[i + 3 + skipSpaces] == '}' || input[i + 3 + skipSpaces] == '{' || input[i + 3 + skipSpaces] == ' ' || input[i + 3 + skipSpaces] == '\t' || input[i + 3 + skipSpaces] == '\\' || input[i + 3 + skipSpaces] == '\r' || input[i + 3 + skipSpaces] == '\n')) {
                /// We found a modifier which is followed by
                /// a letter followed by a command delimiter such
                /// as a whitespace, so we are looking at something
                /// like \"u followed by a space
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[input[i + 2].toLatin1() - 'A'];
                if (unicodeLetter.unicode() < 127) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(input.midRef(i, 3));
                    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 3);
                } else
                    output.append(unicodeLetter);
                /// Step over those additional characters
                i += 2 + skipSpaces;

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
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 4 && input[i + 2 + skipSpaces] == '{' && isAsciiLetter(input[i + 3 + skipSpaces]) && input[i + 4 + skipSpaces] == '}') {
                /// We found a modifier which is followed by an opening
                /// curly bracket followed a letter followed by a closing
                /// curly bracket, so we are looking at something
                /// like \"{u}
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[input[i + 3 + skipSpaces].toLatin1() - 'A'];
                if (unicodeLetter.unicode() < 127) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(input.midRef(i, 5 + skipSpaces));
                    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 5 + skipSpaces);
                } else
                    output.append(unicodeLetter);
                /// Step over those additional characters
                i += 4 + skipSpaces;
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 3 && input[i + 2 + skipSpaces] == '\\' && isAsciiLetter(input[i + 3 + skipSpaces])) {
                /// This is the case for \'\i or alike.
                bool found = false;
                for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                    if (dotlessIJCharacters[k].letter == input[i + 3 + skipSpaces].toLatin1() && dotlessIJCharacters[k].modifier == input[i + 1].toLatin1()) {
                        output.append(QChar(dotlessIJCharacters[k].unicode));
                        i += 3 + skipSpaces;
                        found = true;
                    }
                if (!found)
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 1] << "BACKSLASH" << input[i + 3 + skipSpaces];
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 5 && input[i + 2 + skipSpaces] == '{' && input[i + 3 + skipSpaces] == '\\' && isAsciiLetter(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == '}') {
                /// This is the case for \'{\i} or alike.
                bool found = false;
                for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                    if (dotlessIJCharacters[k].letter == input[i + 4 + skipSpaces].toLatin1() && dotlessIJCharacters[k].modifier == input[i + 1].toLatin1()) {
                        output.append(QChar(dotlessIJCharacters[k].unicode));
                        i += 5 + skipSpaces;
                        found = true;
                    }
                if (!found)
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 1] << "BACKSLASH {" << input[i + 4 + skipSpaces] << "}";
            } else if (i < len - 1) {
                /// Now, the case of something like \AA is left
                /// to check for
                QString alpha = readAlphaCharacters(input, i + 1);
                int nextPosAfterAlpha = i + 1 + alpha.size();
                if (alpha.size() >= 1 && alpha.at(0).isLetter()) {
                    /// We are dealing actually with a string like \AA or \o
                    /// Check which command it is,
                    /// insert corresponding Unicode character
                    bool foundCommand = false;
                    for (int ci = 0; !foundCommand && ci < encoderLaTeXCharacterCommandsLen; ++ci) {
                        if (QLatin1String(encoderLaTeXCharacterCommands[ci].letters) == alpha) /** note: QStringLiteral won't work here (?) */ {
                            output.append(QChar(encoderLaTeXCharacterCommands[ci].unicode));
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
                    else if (i < len - 1 && input[i + 1] == QChar(0x002c /* comma */)) {
                        /// Found a thin space: \,
                        /// Replacing Latex-like thin space with Unicode thin space
                        output.append(QChar(0x2009));
                        // foundCommand = true; ///< only necessary if more tests will follow in the future
                        ++i;
                    } else {
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
                if (encoderLaTeXSymbolSequences[l].useUnicode && encoderLaTeXSymbolSequences[l].latex[0] == c.toLatin1() && i <= len - (int)qstrlen(encoderLaTeXSymbolSequences[l].latex)) {
                    /// Now actually check if symbol sequence is in input buffer
                    isSymbolSequence = true;
                    for (int p = 1; isSymbolSequence && p < (int)qstrlen(encoderLaTeXSymbolSequences[l].latex); ++p)
                        isSymbolSequence &= encoderLaTeXSymbolSequences[l].latex[p] == input[i + p].toLatin1();
                    if (isSymbolSequence) {
                        /// Ok, found sequence: insert Unicode character in output
                        /// and hop over sequence in input buffer
                        output.append(QChar(encoderLaTeXSymbolSequences[l].unicode));
                        i += qstrlen(encoderLaTeXSymbolSequences[l].latex) - 1;
                        break;
                    }
                }

            if (!isSymbolSequence) {
                /// No symbol sequence found, so just copy input to output
                output.append(c);

                /// Still, check if input character is a dollar sign
                /// without a preceding backslash, means toggling between
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

        output.append(input.midRef(pos, copyBytesCount));
        pos += copyBytesCount;
    }

    return copyBytesCount > 0;
}

QString EncoderLaTeX::encode(const QString &ninput, const TargetEncoding targetEncoding) const
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString input = ninput.normalized(QString::NormalizationForm_C);

    int len = input.length();
    QString output;
    output.reserve(len);
    bool inMathMode = false;

    /// Go through input char by char
    for (int i = 0; i < len; ++i) {
        /**
         * Repeatedly check if input data contains a verbatim command
         * like \url{...}, append it to output, and update i to point
         * to the next character after the verbatim command.
         */
        while (testAndCopyVerbatimCommands(input, i, output));
        if (i >= len) break;

        const QChar c = input[i];

        if (targetEncoding == TargetEncodingASCII && c.unicode() > 127) {
            /// If current char is outside ASCII boundaries ...
            bool found = false;

            /// Handle special cases of i without a dot (\i)
            for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                if (c.unicode() == dotlessIJCharacters[k].unicode) {
                    output.append(QString(QStringLiteral("\\%1{\\%2}")).arg(dotlessIJCharacters[k].modifier).arg(dotlessIJCharacters[k].letter));
                    found = true;
                }

            if (!found) {
                /// ... test if there is a symbol sequence like ---
                /// to encode it
                for (int k = 0; k < encoderLaTeXSymbolSequencesLen; ++k)
                    if (encoderLaTeXSymbolSequences[k].unicode == c.unicode()) {
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
                    if (encoderLaTeXCharacterCommands[k].unicode == c.unicode()) {
                        output.append(QString(QStringLiteral("{\\%1}")).arg(encoderLaTeXCharacterCommands[k].letters));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, neither a character command. Let's test
                /// escaped characters with modifiers like \"a
                for (int k = 0; k < encoderLaTeXEscapedCharactersLen; ++k)
                    if (encoderLaTeXEscapedCharacters[k].unicode == c.unicode()) {
                        const char modifier = encoderLaTeXEscapedCharacters[k].modifier;
                        const QString formatString = (modifier >= 'A' && modifier <= 'Z') || (modifier >= 'a' && modifier <= 'z') ? QStringLiteral("{\\%1 %2}") : QStringLiteral("{\\%1%2}");
                        output.append(formatString.arg(modifier).arg(encoderLaTeXEscapedCharacters[k].letter));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, test for math commands
                for (int k = 0; k < mathCommandLen; ++k)
                    if (mathCommand[k].unicode == c.unicode()) {
                        if (inMathMode)
                            output.append(QString(QStringLiteral("\\%1{}")).arg(mathCommand[k].written));
                        else
                            output.append(QString(QStringLiteral("\\ensuremath{\\%1}")).arg(mathCommand[k].written));
                        found = true;
                        break;
                    }
            }

            if (!found && c.unicode() == 0x2009) {
                /// Thin space
                output.append(QStringLiteral("\\,"));
                found = true;
            }

            if (!found) {
                qCWarning(LOG_KBIBTEX_IO) << "Don't know how to encode Unicode char" << QString(QStringLiteral("0x%1")).arg(c.unicode(), 4, 16, QLatin1Char('0'));
                output.append(c);
            }
        } else {
            /// Current character is normal ASCII
            /// and targetEncoding was set to accept only ASCII characters
            /// -- or -- targetEncoding was set to accept UTF-8 characters

            /// Still, some characters have special meaning
            /// in TeX and have to be preceded with a backslash
            bool found = false;
            for (int k = 0; k < encoderLaTeXProtectedSymbolsLen; ++k)
                if (encoderLaTeXProtectedSymbols[k] == c) {
                    output.append(QLatin1Char('\\'));
                    found = true;
                    break;
                }

            if (!found && !inMathMode)
                for (int k = 0; k < encoderLaTeXProtectedTextOnlySymbolsLen; ++k)
                    if (encoderLaTeXProtectedTextOnlySymbols[k] == c) {
                        output.append(QLatin1Char('\\'));
                        break;
                    }

            /// Dump character to output
            output.append(c);

            /// Finally, check if input character is a dollar sign
            /// without a preceding backslash, means toggling between
            /// text mode and math mode
            if (c == '$' && (i == 0 || input[i - 1] != QLatin1Char('\\')))
                inMathMode = !inMathMode;
        }
    }

    output.squeeze();
    return output;
}

#ifdef HAVE_ICU
QString EncoderLaTeX::convertToPlainAscii(const QString &ninput) const
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

    const int ninputLen = ninput.length();
    /// Make a copy of the input string into an array of UChar
    UChar *uChars = new UChar[ninputLen];
    for (int i = 0; i < ninputLen; ++i)
        uChars[i] = ninput.at(i).unicode();
    /// Create an ICU-specific unicode string
    icu::UnicodeString uString = icu::UnicodeString(uChars, ninputLen);
    /// Perform the actual transliteration, modifying Unicode string
    m_trans->transliterate(uString);
    /// Create regular C++ string from Unicode string
    std::string cppString;
    uString.toUTF8String(cppString);
    /// Clean up any mess
    delete[] uChars;
    /// Convert regular C++ to Qt-specific QString,
    /// should work as cppString contains only ASCII text
    return QString::fromStdString(cppString);
}
#endif // HAVE_ICU

bool EncoderLaTeX::containsOnlyAscii(const QString &ntext)
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString text = ntext.normalized(QString::NormalizationForm_C);

    for (const QChar &c : text)
        if (c.unicode() > 127) return false;
    return true;
}

int EncoderLaTeX::modifierInLookupTable(const char latinModifier) const
{
    for (int m = 0; m < lookupTableNumModifiers && lookupTable[m] != nullptr; ++m)
        if (lookupTable[m]->modifier == latinModifier) return m;
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

const EncoderLaTeX &EncoderLaTeX::instance()
{
    static const EncoderLaTeX self;
    return self;
}
