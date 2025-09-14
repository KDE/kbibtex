/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QStack>

#include "logging_io.h"

inline bool isAsciiLetter(const QChar c) {
    static const ushort upperCaseLetterA = QChar(u'A').unicode();
    static const ushort upperCaseLetterZ = QChar(u'Z').unicode();
    static const ushort lowerCaseLetterA = QChar(u'a').unicode();
    static const ushort lowerCaseLetterZ = QChar(u'z').unicode();
    const ushort unicode = c.unicode();
    return (unicode >= upperCaseLetterA && unicode <= upperCaseLetterZ) || (unicode >= lowerCaseLetterA && unicode <= lowerCaseLetterZ);
}

inline int asciiLetterOrDigitToPos(const QChar c) {
    static const ushort upperCaseLetterA = QChar(u'A').unicode();
    static const ushort upperCaseLetterZ = QChar(u'Z').unicode();
    static const ushort lowerCaseLetterA = QChar(u'a').unicode();
    static const ushort lowerCaseLetterZ = QChar(u'z').unicode();
    static const ushort digit0 = QChar(u'0').unicode();
    static const ushort digit9 = QChar(u'9').unicode();
    const ushort unicode = c.unicode();
    if (unicode >= upperCaseLetterA && unicode <= upperCaseLetterZ) return unicode - upperCaseLetterA;
    else if (unicode >= lowerCaseLetterA && unicode <= lowerCaseLetterZ) return unicode + 26 - lowerCaseLetterA;
    else if (unicode >= digit0 && unicode <= digit9) return unicode + 52 - digit0;
    else return -1;
}

inline bool isIJ(const QChar c) {
    static const QChar upperCaseLetterI = u'I';
    static const QChar upperCaseLetterJ = u'J';
    static const QChar lowerCaseLetterI = u'i';
    static const QChar lowerCaseLetterJ = u'j';
    return c == upperCaseLetterI || c == upperCaseLetterJ || c == lowerCaseLetterI || c == lowerCaseLetterJ;
}

enum EncoderLaTeXCommandDirection {
    DirectionCommandToUnicode = 1, //< A mapping between command and unicode value may be used in the direction from command to unicode value
    DirectionUnicodeToCommand = 2, //< A mapping between command and unicode value may be used in the direction from unicode value to command
    DirectionBoth = DirectionCommandToUnicode | DirectionUnicodeToCommand
};

/**
 * General documentation on this topic:
 *   https://www.latex-project.org/help/documentation/encguide.pdf
 *   https://mirror.hmc.edu/ctan/macros/xetex/latex/xecjk/xunicode-symbols.pdf
 *   ftp://ftp.dante.de/tex-archive/biblio/biber/documentation/utf8-macro-map.html
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
    const QChar modifier;
    const QChar letter;
    const ushort unicode;
    const EncoderLaTeXCommandDirection direction;
}
encoderLaTeXEscapedCharacters[] = {
    {u'`', u'A', 0x00C0, DirectionBoth},
    {u'\'', u'A', 0x00C1, DirectionBoth},
    {u'^', u'A', 0x00C2, DirectionBoth},
    {u'~', u'A', 0x00C3, DirectionBoth},
    {u'"', u'A', 0x00C4, DirectionBoth},
    {u'r', u'A', 0x00C5, DirectionBoth},
    /** 0x00C6: see EncoderLaTeXCharacterCommand */
    {u'c', u'C', 0x00C7, DirectionBoth},
    {u'`', u'E', 0x00C8, DirectionBoth},
    {u'\'', u'E', 0x00C9, DirectionBoth},
    {u'^', u'E', 0x00CA, DirectionBoth},
    {u'"', u'E', 0x00CB, DirectionBoth},
    {u'`', u'I', 0x00CC, DirectionBoth},
    {u'\'', u'I', 0x00CD, DirectionBoth},
    {u'^', u'I', 0x00CE, DirectionBoth},
    {u'"', u'I', 0x00CF, DirectionBoth},
    /** 0x00D0: see EncoderLaTeXCharacterCommand */
    {u'~', u'N', 0x00D1, DirectionBoth},
    {u'`', u'O', 0x00D2, DirectionBoth},
    {u'\'', u'O', 0x00D3, DirectionBoth},
    {u'^', u'O', 0x00D4, DirectionBoth},
    {u'~', u'O', 0x00D5, DirectionBoth},
    {u'"', u'O', 0x00D6, DirectionBoth},
    /** 0x00D7: see EncoderLaTeXCharacterCommand */
    /** 0x00D8: see EncoderLaTeXCharacterCommand */
    {u'`', u'U', 0x00D9, DirectionBoth},
    {u'\'', u'U', 0x00DA, DirectionBoth},
    {u'^', u'U', 0x00DB, DirectionBoth},
    {u'"', u'U', 0x00DC, DirectionBoth},
    {u'\'', u'Y', 0x00DD, DirectionBoth},
    /** 0x00DE: see EncoderLaTeXCharacterCommand */
    {u'"', u's', 0x00DF, DirectionBoth},
    {u'`', u'a', 0x00E0, DirectionBoth},
    {u'\'', u'a', 0x00E1, DirectionBoth},
    {u'^', u'a', 0x00E2, DirectionBoth},
    {u'~', u'a', 0x00E3, DirectionBoth},
    {u'"', u'a', 0x00E4, DirectionBoth},
    {u'r', u'a', 0x00E5, DirectionBoth},
    /** 0x00E6: see EncoderLaTeXCharacterCommand */
    {u'c', u'c', 0x00E7, DirectionBoth},
    {u'`', u'e', 0x00E8, DirectionBoth},
    {u'\'', u'e', 0x00E9, DirectionBoth},
    {u'^', u'e', 0x00EA, DirectionBoth},
    {u'"', u'e', 0x00EB, DirectionBoth},
    {u'`', u'i', 0x00EC, DirectionBoth},
    {u'\'', u'i', 0x00ED, DirectionBoth},
    {u'^', u'i', 0x00EE, DirectionBoth},
    {u'"', u'i', 0x00EF, DirectionBoth},
    /** 0x00F0: see EncoderLaTeXCharacterCommand */
    {u'~', u'n', 0x00F1, DirectionBoth},
    {u'`', u'o', 0x00F2, DirectionBoth},
    {u'\'', u'o', 0x00F3, DirectionBoth},
    {u'^', u'o', 0x00F4, DirectionBoth},
    {u'~', u'o', 0x00F5, DirectionBoth},
    {u'"', u'o', 0x00F6, DirectionBoth},
    /** 0x00F7: see EncoderLaTeXCharacterCommand */
    /** 0x00F8: see EncoderLaTeXCharacterCommand */
    {u'`', u'u', 0x00F9, DirectionBoth},
    {u'\'', u'u', 0x00FA, DirectionBoth},
    {u'^', u'u', 0x00FB, DirectionBoth},
    {u'"', u'u', 0x00FC, DirectionBoth},
    {u'\'', u'y', 0x00FD, DirectionBoth},
    /** 0x00FE: see EncoderLaTeXCharacterCommand */
    {u'"', u'y', 0x00FF, DirectionBoth},
    {u'=', u'A', 0x0100, DirectionBoth},
    {u'=', u'a', 0x0101, DirectionBoth},
    {u'u', u'A', 0x0102, DirectionBoth},
    {u'u', u'a', 0x0103, DirectionBoth},
    {u'k', u'A', 0x0104, DirectionBoth},
    {u'k', u'a', 0x0105, DirectionBoth},
    {u'\'', u'C', 0x0106, DirectionBoth},
    {u'\'', u'c', 0x0107, DirectionBoth},
    {u'^', u'C', 0x0108, DirectionBoth},
    {u'^', u'c', 0x0109, DirectionBoth},
    {u'.', u'C', 0x010A, DirectionBoth},
    {u'.', u'c', 0x010B, DirectionBoth},
    {u'v', u'C', 0x010C, DirectionBoth},
    {u'v', u'c', 0x010D, DirectionBoth},
    {u'v', u'D', 0x010E, DirectionBoth},
    {u'v', u'd', 0x010F, DirectionBoth},
    {u'B', u'D', 0x0110, DirectionCommandToUnicode}, //< 'African D', command provided by package 'fc' (command seems to be the same as \M{D})
    {u'B', u'd', 0x0111, DirectionCommandToUnicode}, //< 'African d' (?), command provided by package 'fc'
    {u'=', u'E', 0x0112, DirectionBoth},
    {u'=', u'e', 0x0113, DirectionBoth},
    {u'u', u'E', 0x0114, DirectionBoth},
    {u'u', u'e', 0x0115, DirectionBoth},
    {u'.', u'E', 0x0116, DirectionBoth},
    {u'.', u'e', 0x0117, DirectionBoth},
    {u'k', u'E', 0x0118, DirectionBoth},
    {u'k', u'e', 0x0119, DirectionBoth},
    {u'v', u'E', 0x011A, DirectionBoth},
    {u'v', u'e', 0x011B, DirectionBoth},
    {u'^', u'G', 0x011C, DirectionBoth},
    {u'^', u'g', 0x011D, DirectionBoth},
    {u'u', u'G', 0x011E, DirectionBoth},
    {u'u', u'g', 0x011F, DirectionBoth},
    {u'.', u'G', 0x0120, DirectionBoth},
    {u'.', u'g', 0x0121, DirectionBoth},
    {u'c', u'G', 0x0122, DirectionBoth},
    {u'c', u'g', 0x0123, DirectionBoth},
    {u'^', u'H', 0x0124, DirectionBoth},
    {u'^', u'h', 0x0125, DirectionBoth},
    {u'B', u'H', 0x0126, DirectionCommandToUnicode},
    {u'B', u'h', 0x0127, DirectionCommandToUnicode},
    {u'~', u'I', 0x0128, DirectionBoth},
    {u'~', u'i', 0x0129, DirectionBoth},
    {u'=', u'I', 0x012A, DirectionBoth},
    {u'=', u'i', 0x012B, DirectionBoth},
    {u'u', u'I', 0x012C, DirectionBoth},
    {u'u', u'i', 0x012D, DirectionBoth},
    {u'k', u'I', 0x012E, DirectionBoth},
    {u'k', u'i', 0x012F, DirectionBoth},
    {u'.', u'I', 0x0130, DirectionBoth},
    /** 0x0131: see EncoderLaTeXCharacterCommand */
    /** 0x0132: see EncoderLaTeXCharacterCommand */
    /** 0x0133: see EncoderLaTeXCharacterCommand */
    {u'^', u'J', 0x012E, DirectionBoth},
    {u'^', u'j', 0x012F, DirectionBoth},
    {u'c', u'K', 0x0136, DirectionBoth},
    {u'c', u'k', 0x0137, DirectionBoth},
    /** 0x0138: see EncoderLaTeXCharacterCommand */
    {u'\'', u'L', 0x0139, DirectionBoth},
    {u'\'', u'l', 0x013A, DirectionBoth},
    {u'c', u'L', 0x013B, DirectionBoth},
    {u'c', u'l', 0x013C, DirectionBoth},
    {u'v', u'L', 0x013D, DirectionBoth},
    {u'v', u'l', 0x013E, DirectionBoth},
    {u'.', u'L', 0x013F, DirectionBoth},
    {u'.', u'l', 0x0140, DirectionBoth},
    {u'B', u'L', 0x0141, DirectionCommandToUnicode},
    {u'B', u'l', 0x0142, DirectionCommandToUnicode},
    {u'\'', u'N', 0x0143, DirectionBoth},
    {u'\'', u'n', 0x0144, DirectionBoth},
    {u'c', u'n', 0x0145, DirectionBoth},
    {u'c', u'n', 0x0146, DirectionBoth},
    {u'v', u'N', 0x0147, DirectionBoth},
    {u'v', u'n', 0x0148, DirectionBoth},
    /** 0x0149: TODO n preceded by apostrophe */
    {u'm', u'N', 0x014A, DirectionCommandToUnicode},
    {u'm', u'n', 0x014B, DirectionCommandToUnicode},
    {u'=', u'O', 0x014C, DirectionBoth},
    {u'=', u'o', 0x014D, DirectionBoth},
    {u'u', u'O', 0x014E, DirectionBoth},
    {u'u', u'o', 0x014F, DirectionBoth},
    {u'H', u'O', 0x0150, DirectionBoth},
    {u'H', u'o', 0x0151, DirectionBoth},
    /** 0x0152: see EncoderLaTeXCharacterCommand */
    /** 0x0153: see EncoderLaTeXCharacterCommand */
    {u'\'', u'R', 0x0154, DirectionBoth},
    {u'\'', u'r', 0x0155, DirectionBoth},
    {u'c', u'R', 0x0156, DirectionBoth},
    {u'c', u'r', 0x0157, DirectionBoth},
    {u'v', u'R', 0x0158, DirectionBoth},
    {u'v', u'r', 0x0159, DirectionBoth},
    {u'\'', u'S', 0x015A, DirectionBoth},
    {u'\'', u's', 0x015B, DirectionBoth},
    {u'^', u'S', 0x015C, DirectionBoth},
    {u'^', u's', 0x015D, DirectionBoth},
    {u'c', u'S', 0x015E, DirectionBoth},
    {u'c', u's', 0x015F, DirectionBoth},
    {u'v', u'S', 0x0160, DirectionBoth},
    {u'v', u's', 0x0161, DirectionBoth},
    {u'c', u'T', 0x0162, DirectionBoth},
    {u'c', u't', 0x0163, DirectionBoth},
    {u'v', u'T', 0x0164, DirectionBoth},
    {u'v', u't', 0x0165, DirectionBoth},
    {u'B', u'T', 0x0166, DirectionCommandToUnicode},
    {u'B', u't', 0x0167, DirectionCommandToUnicode},
    {u'~', u'U', 0x0168, DirectionBoth},
    {u'~', u'u', 0x0169, DirectionBoth},
    {u'=', u'U', 0x016A, DirectionBoth},
    {u'=', u'u', 0x016B, DirectionBoth},
    {u'u', u'U', 0x016C, DirectionBoth},
    {u'u', u'u', 0x016D, DirectionBoth},
    {u'r', u'U', 0x016E, DirectionBoth},
    {u'r', u'u', 0x016F, DirectionBoth},
    {u'H', u'U', 0x0170, DirectionBoth},
    {u'H', u'u', 0x0171, DirectionBoth},
    {u'k', u'U', 0x0172, DirectionBoth},
    {u'k', u'u', 0x0173, DirectionBoth},
    {u'^', u'W', 0x0174, DirectionBoth},
    {u'^', u'w', 0x0175, DirectionBoth},
    {u'^', u'Y', 0x0176, DirectionBoth},
    {u'^', u'y', 0x0177, DirectionBoth},
    {u'"', u'Y', 0x0178, DirectionBoth},
    {u'\'', u'Z', 0x0179, DirectionBoth},
    {u'\'', u'z', 0x017A, DirectionBoth},
    {u'.', u'Z', 0x017B, DirectionBoth},
    {u'.', u'z', 0x017C, DirectionBoth},
    {u'v', u'Z', 0x017D, DirectionBoth},
    {u'v', u'z', 0x017E, DirectionBoth},
    /** 0x017F: TODO long s */
    {u'B', u'b', 0x0180, DirectionCommandToUnicode},
    {u'm', u'B', 0x0181, DirectionCommandToUnicode},
    /** 0x0182 */
    /** 0x0183 */
    /** 0x0184 */
    /** 0x0185 */
    {u'm', u'O', 0x0186, DirectionCommandToUnicode},
    {u'm', u'C', 0x0187, DirectionCommandToUnicode},
    {u'm', u'c', 0x0188, DirectionCommandToUnicode},
    {u'M', u'D', 0x0189, DirectionBoth}, //< 'African D', command provided by package 'fc' (command seems to be the same as \B{D})
    {u'm', u'D', 0x018A, DirectionCommandToUnicode},
    /** 0x018B */
    /** 0x018C */
    /** 0x018D */
    {u'M', u'E', 0x018E, DirectionCommandToUnicode},
    /** 0x018F */
    {u'm', u'E', 0x0190, DirectionCommandToUnicode},
    {u'm', u'F', 0x0191, DirectionCommandToUnicode},
    {u'm', u'f', 0x0192, DirectionCommandToUnicode},
    /** 0x0193 */
    {u'm', u'G', 0x0194, DirectionCommandToUnicode},
    /** 0x0195: see EncoderLaTeXCharacterCommand */
    {u'm', u'I', 0x0196, DirectionCommandToUnicode},
    {u'B', u'I', 0x0197, DirectionCommandToUnicode},
    {u'm', u'K', 0x0198, DirectionCommandToUnicode},
    {u'm', u'k', 0x0199, DirectionCommandToUnicode},
    {u'B', u'l', 0x019A, DirectionCommandToUnicode},
    /** 0x019B */
    /** 0x019C */
    {u'm', u'J', 0x019D, DirectionCommandToUnicode},
    /** 0x019E */
    /** 0x019F */
    /** 0x01A0 */
    /** 0x01A1 */
    /** 0x01A2 */
    /** 0x01A3 */
    {u'm', u'P', 0x01A4, DirectionCommandToUnicode},
    {u'm', u'p', 0x01A5, DirectionCommandToUnicode},
    /** 0x01A6 */
    /** 0x01A7 */
    /** 0x01A8 */
    /** 0x01A9: see EncoderLaTeXCharacterCommand */
    /** 0x01AA */
    /** 0x01AB */
    {u'm', u'T', 0x01AC, DirectionCommandToUnicode},
    {u'm', u't', 0x01AD, DirectionCommandToUnicode},
    {u'M', u'T', 0x01AE, DirectionCommandToUnicode},
    /** 0x01AF */
    /** 0x01B0 */
    {u'm', u'U', 0x01B1, DirectionCommandToUnicode},
    {u'm', u'V', 0x01B2, DirectionCommandToUnicode},
    {u'm', u'Y', 0x01B3, DirectionCommandToUnicode},
    {u'm', u'y', 0x01B4, DirectionCommandToUnicode},
    {u'B', u'Z', 0x01B5, DirectionCommandToUnicode},
    {u'B', u'z', 0x01B6, DirectionCommandToUnicode},
    {u'm', u'Z', 0x01B7, DirectionCommandToUnicode},
    /** 0x01B8 */
    /** 0x01B9 */
    /** 0x01BA */
    {u'B', u'2', 0x01BB, DirectionCommandToUnicode},
    /** 0x01BC */
    /** 0x01BD */
    /** 0x01BE */
    /** 0x01BF */
    /** 0x01C0 */
    /** 0x01C1 */
    /** 0x01C2 */
    /** 0x01C3 */
    /** 0x01C4 */
    /** 0x01C5 */
    /** 0x01C6 */
    /** 0x01C7 */
    /** 0x01C8 */
    /** 0x01C9 */
    /** 0x01CA */
    /** 0x01CB */
    /** 0x01CC */
    {u'v', u'A', 0x01CD, DirectionBoth},
    {u'v', u'a', 0x01CE, DirectionBoth},
    {u'v', u'G', 0x01E6, DirectionBoth},
    {u'v', u'g', 0x01E7, DirectionBoth},
    {u'k', u'O', 0x01EA, DirectionBoth},
    {u'k', u'o', 0x01EB, DirectionBoth},
    {u'\'', u'F', 0x01F4, DirectionBoth},
    {u'\'', u'f', 0x01F5, DirectionBoth},
    {u'.', u'A', 0x0226, DirectionBoth},
    {u'.', u'a', 0x0227, DirectionBoth},
    {u'c', u'E', 0x0228, DirectionBoth},
    {u'c', u'e', 0x0229, DirectionBoth},
    {u'=', u'Y', 0x0232, DirectionBoth},
    {u'=', u'y', 0x0233, DirectionBoth},
    {u'.', u'O', 0x022E, DirectionBoth},
    {u'.', u'o', 0x022F, DirectionBoth},
    {u'M', u'd', 0x0256, DirectionBoth}, //< 'African d', command provided by package 'fc' (may be same as \B{d} ?)
    {u'.', u'B', 0x1E02, DirectionBoth},
    {u'.', u'b', 0x1E03, DirectionBoth},
    {u'd', u'B', 0x1E04, DirectionBoth},
    {u'd', u'b', 0x1E05, DirectionBoth},
    {u'.', u'D', 0x1E0A, DirectionBoth},
    {u'.', u'd', 0x1E0B, DirectionBoth},
    {u'd', u'D', 0x1E0C, DirectionBoth},
    {u'd', u'd', 0x1E0D, DirectionBoth},
    {u'c', u'D', 0x1E10, DirectionBoth},
    {u'c', u'd', 0x1E11, DirectionBoth},
    {u'.', u'E', 0x1E1E, DirectionBoth},
    {u'.', u'e', 0x1E1F, DirectionBoth},
    {u'.', u'H', 0x1E22, DirectionBoth},
    {u'.', u'h', 0x1E23, DirectionBoth},
    {u'd', u'H', 0x1E24, DirectionBoth},
    {u'd', u'h', 0x1E25, DirectionBoth},
    {u'"', u'H', 0x1E26, DirectionBoth},
    {u'"', u'h', 0x1E27, DirectionBoth},
    {u'c', u'H', 0x1E28, DirectionBoth},
    {u'c', u'h', 0x1E29, DirectionBoth},
    {u'd', u'K', 0x1E32, DirectionBoth},
    {u'd', u'k', 0x1E33, DirectionBoth},
    {u'd', u'L', 0x1E36, DirectionBoth},
    {u'd', u'l', 0x1E37, DirectionBoth},
    {u'.', u'M', 0x1E40, DirectionBoth},
    {u'.', u'm', 0x1E41, DirectionBoth},
    {u'd', u'M', 0x1E42, DirectionBoth},
    {u'd', u'm', 0x1E43, DirectionBoth},
    {u'.', u'N', 0x1E44, DirectionBoth},
    {u'.', u'n', 0x1E45, DirectionBoth},
    {u'.', u'N', 0x1E46, DirectionBoth},
    {u'.', u'n', 0x1E47, DirectionBoth},
    {u'.', u'P', 0x1E56, DirectionBoth},
    {u'.', u'p', 0x1E57, DirectionBoth},
    {u'.', u'R', 0x1E58, DirectionBoth},
    {u'.', u'r', 0x1E59, DirectionBoth},
    {u'd', u'R', 0x1E5A, DirectionBoth},
    {u'd', u'r', 0x1E5B, DirectionBoth},
    {u'.', u'S', 0x1E60, DirectionBoth},
    {u'.', u's', 0x1E61, DirectionBoth},
    {u'd', u'S', 0x1E62, DirectionBoth},
    {u'd', u's', 0x1E63, DirectionBoth},
    {u'.', u'T', 0x1E6A, DirectionBoth},
    {u'.', u't', 0x1E6B, DirectionBoth},
    {u'd', u'T', 0x1E6C, DirectionBoth},
    {u'd', u't', 0x1E6D, DirectionBoth},
    {u'd', u'V', 0x1E7E, DirectionBoth},
    {u'd', u'v', 0x1E7F, DirectionBoth},
    {u'`', u'W', 0x1E80, DirectionBoth},
    {u'`', u'w', 0x1E81, DirectionBoth},
    {u'\'', u'W', 0x1E82, DirectionBoth},
    {u'\'', u'w', 0x1E83, DirectionBoth},
    {u'"', u'W', 0x1E84, DirectionBoth},
    {u'"', u'w', 0x1E85, DirectionBoth},
    {u'.', u'W', 0x1E86, DirectionBoth},
    {u'.', u'w', 0x1E87, DirectionBoth},
    {u'd', u'W', 0x1E88, DirectionBoth},
    {u'd', u'w', 0x1E88, DirectionBoth},
    {u'.', u'X', 0x1E8A, DirectionBoth},
    {u'.', u'x', 0x1E8B, DirectionBoth},
    {u'"', u'X', 0x1E8C, DirectionBoth},
    {u'"', u'x', 0x1E8D, DirectionBoth},
    {u'.', u'Y', 0x1E8E, DirectionBoth},
    {u'.', u'y', 0x1E8F, DirectionBoth},
    {u'd', u'Z', 0x1E92, DirectionBoth},
    {u'd', u'z', 0x1E93, DirectionBoth},
    {u'"', u't', 0x1E97, DirectionBoth},
    {u'r', u'w', 0x1E98, DirectionBoth},
    {u'r', u'y', 0x1E99, DirectionBoth},
    {u'd', u'A', 0x1EA0, DirectionBoth},
    {u'd', u'a', 0x1EA1, DirectionBoth},
    {u'd', u'E', 0x1EB8, DirectionBoth},
    {u'd', u'e', 0x1EB9, DirectionBoth},
    {u'd', u'I', 0x1ECA, DirectionBoth},
    {u'd', u'i', 0x1ECB, DirectionBoth},
    {u'd', u'O', 0x1ECC, DirectionBoth},
    {u'd', u'o', 0x1ECD, DirectionBoth},
    {u'd', u'U', 0x1EE4, DirectionBoth},
    {u'd', u'u', 0x1EE5, DirectionBoth},
    {u'`', u'Y', 0x1EF2, DirectionBoth},
    {u'`', u'y', 0x1EF3, DirectionBoth},
    {u'd', u'Y', 0x1EF4, DirectionBoth},
    {u'd', u'y', 0x1EF5, DirectionBoth},
    {u'r', u'q', 0x2019, DirectionCommandToUnicode} ///< tricky: this is \rq
};


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
static const struct DotlessIJCharacter {
    const QChar modifier;
    const QChar letter;
    const ushort unicode;
    const EncoderLaTeXCommandDirection direction;
}
dotlessIJCharacters[] = {
    {u'`', u'i', 0x00EC, DirectionBoth},
    {u'\'', u'i', 0x00ED, DirectionBoth},
    {u'^', u'i', 0x00EE, DirectionBoth},
    {u'"', u'i', 0x00EF, DirectionBoth},
    {u'~', u'i', 0x0129, DirectionBoth},
    {u'=', u'i', 0x012B, DirectionBoth},
    {u'u', u'i', 0x012D, DirectionBoth},
    {u'k', u'i', 0x012F, DirectionBoth},
    {u'^', u'j', 0x0135, DirectionBoth},
    {u'v', u'i', 0x01D0, DirectionBoth},
    {u'v', u'j', 0x01F0, DirectionBoth},
    {u'G', u'i', 0x0209, DirectionCommandToUnicode}
};


/**
 * This lookup allows to quickly find hits in the
 * EncoderLaTeXEscapedCharacter table. This data structure here
 * consists of a number of rows. Each row consists of a
 * modifier (like '"' or 'v') and an array of Unicode chars.
 * Letters 'A'..'Z','a'..'z','0'..'9' are used as index to this
 * array by invocing asciiLetterOrDigitToPos().
 * This data structure is built in the constructor.
 */
static const int lookupTableNumModifiers = 32;
static const int lookupTableNumCharacters = 26 * 2 + 10;
static struct EncoderLaTeXEscapedCharacterLookupTableRow {
    QChar modifier;
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
    const QString command;
    const ushort unicode;
    const EncoderLaTeXCommandDirection direction;
}
mathCommands[] = {
    {QStringLiteral("pm"), 0x00B1, DirectionBoth},
    {QStringLiteral("mu"), 0x00B5, DirectionUnicodeToCommand}, //< Unicode's micro symbol becomes Greek letter
    {QStringLiteral("times"), 0x00D7, DirectionBoth},
    {QStringLiteral("div"), 0x00F7, DirectionBoth},
    {QStringLiteral("phi"), 0x0278, DirectionBoth}, ///< see also 0x03C6 (GREEK SMALL LETTER PHI)
    {QStringLiteral("Alpha"), 0x0391, DirectionBoth},
    {QStringLiteral("Beta"), 0x0392, DirectionBoth},
    {QStringLiteral("Gamma"), 0x0393, DirectionBoth},
    {QStringLiteral("Delta"), 0x0394, DirectionBoth},
    {QStringLiteral("Epsilon"), 0x0395, DirectionBoth},
    {QStringLiteral("Zeta"), 0x0396, DirectionBoth},
    {QStringLiteral("Eta"), 0x0397, DirectionBoth},
    {QStringLiteral("Theta"), 0x0398, DirectionBoth},
    {QStringLiteral("Iota"), 0x0399, DirectionBoth},
    {QStringLiteral("Kappa"), 0x039A, DirectionBoth},
    {QStringLiteral("Lamda"), 0x039B, DirectionCommandToUnicode}, ///< \Lamda does not exist, this is mostly for spelling errors
    {QStringLiteral("Lambda"), 0x039B, DirectionBoth},
    {QStringLiteral("Mu"), 0x039C, DirectionBoth},
    {QStringLiteral("Nu"), 0x039D, DirectionBoth},
    {QStringLiteral("Xi"), 0x039E, DirectionBoth},
    {QStringLiteral("Omicron"), 0x039F, DirectionBoth},
    {QStringLiteral("Pi"), 0x03A0, DirectionBoth},
    {QStringLiteral("Rho"), 0x03A1, DirectionBoth},
    {QStringLiteral("Sigma"), 0x03A3, DirectionBoth},
    {QStringLiteral("Tau"), 0x03A4, DirectionBoth},
    {QStringLiteral("Upsilon"), 0x03A5, DirectionBoth},
    {QStringLiteral("Phi"), 0x03A6, DirectionBoth},
    {QStringLiteral("Chi"), 0x03A7, DirectionBoth},
    {QStringLiteral("Psi"), 0x03A8, DirectionBoth},
    {QStringLiteral("Omega"), 0x03A9, DirectionBoth},
    {QStringLiteral("alpha"), 0x03B1, DirectionBoth},
    {QStringLiteral("beta"), 0x03B2, DirectionBoth},
    {QStringLiteral("gamma"), 0x03B3, DirectionBoth},
    {QStringLiteral("delta"), 0x03B4, DirectionBoth},
    {QStringLiteral("varepsilon"), 0x03B5, DirectionBoth},
    {QStringLiteral("zeta"), 0x03B6, DirectionBoth},
    {QStringLiteral("eta"), 0x03B7, DirectionBoth},
    {QStringLiteral("theta"), 0x03B8, DirectionBoth},
    {QStringLiteral("iota"), 0x03B9, DirectionBoth},
    {QStringLiteral("kappa"), 0x03BA, DirectionBoth},
    {QStringLiteral("lamda"), 0x03BB, DirectionCommandToUnicode}, ///< \lamda does not exist, this is mostly for spelling errors
    {QStringLiteral("lambda"), 0x03BB, DirectionBoth},
    {QStringLiteral("mu"), 0x03BC, DirectionBoth},
    {QStringLiteral("nu"), 0x03BD, DirectionBoth},
    {QStringLiteral("xi"), 0x03BE, DirectionBoth},
    {QStringLiteral("omicron"), 0x03BF, DirectionBoth},
    {QStringLiteral("pi"), 0x03C0, DirectionBoth},
    {QStringLiteral("rho"), 0x03C1, DirectionBoth},
    {QStringLiteral("varsigma"), 0x03C2, DirectionBoth},
    {QStringLiteral("sigma"), 0x03C3, DirectionBoth},
    {QStringLiteral("tau"), 0x03C4, DirectionBoth},
    {QStringLiteral("upsilon"), 0x03C5, DirectionBoth},
    {QStringLiteral("varphi"), 0x03C6, DirectionBoth}, ///< see also 0x0278 (LATIN SMALL LETTER PHI)
    {QStringLiteral("chi"), 0x03C7, DirectionBoth},
    {QStringLiteral("psi"), 0x03C8, DirectionBoth},
    {QStringLiteral("omega"), 0x03C9, DirectionBoth},
    {QStringLiteral("vartheta"), 0x03D1, DirectionBoth},
    {QStringLiteral("varpi"), 0x03D6, DirectionBoth},
    {QStringLiteral("digamma"), 0x03DC, DirectionBoth},
    {QStringLiteral("varkappa"), 0x03F0, DirectionBoth},
    {QStringLiteral("varrho"), 0x03F1, DirectionBoth},
    {QStringLiteral("epsilon"), 0x03F5, DirectionBoth},
    {QStringLiteral("backepsilon"), 0x03F6, DirectionBoth},
    {QStringLiteral("aleph"), 0x05D0, DirectionBoth},
    {QStringLiteral("dagger"), 0x2020, DirectionBoth},
    {QStringLiteral("ddagger"), 0x2021, DirectionBoth},
    {QStringLiteral("mathbb{C}"), 0x2102, DirectionBoth},
    {QStringLiteral("ell"), 0x2113, DirectionBoth},
    {QStringLiteral("mho"), 0x2127, DirectionBoth},
    {QStringLiteral("beth"), 0x2136, DirectionBoth},
    {QStringLiteral("gimel"), 0x2137, DirectionBoth},
    {QStringLiteral("daleth"), 0x2138, DirectionBoth},
    {QStringLiteral("rightarrow"), 0x2192, DirectionBoth},
    {QStringLiteral("forall"), 0x2200, DirectionBoth},
    {QStringLiteral("complement"), 0x2201, DirectionBoth},
    {QStringLiteral("partial"), 0x2202, DirectionBoth},
    {QStringLiteral("exists"), 0x2203, DirectionBoth},
    {QStringLiteral("nexists"), 0x2204, DirectionBoth},
    {QStringLiteral("varnothing"), 0x2205, DirectionBoth},
    {QStringLiteral("nabla"), 0x2207, DirectionBoth},
    {QStringLiteral("in"), 0x2208, DirectionBoth},
    {QStringLiteral("notin"), 0x2209, DirectionBoth},
    {QStringLiteral("ni"), 0x220B, DirectionBoth},
    {QStringLiteral("not\\ni"), 0x220C, DirectionBoth},
    {QStringLiteral("asterisk"), 0x2217, DirectionCommandToUnicode},
    {QStringLiteral("infty"), 0x221E, DirectionBoth},
    {QStringLiteral("leq"), 0x2264, DirectionBoth},
    {QStringLiteral("geq"), 0x2265, DirectionBoth},
    {QStringLiteral("lneq"), 0x2268, DirectionBoth},
    {QStringLiteral("gneq"), 0x2269, DirectionBoth},
    {QStringLiteral("ll"), 0x226A, DirectionBoth},
    {QStringLiteral("gg"), 0x226B, DirectionBoth},
    {QStringLiteral("nless"), 0x226E, DirectionBoth},
    {QStringLiteral("ngtr"), 0x226F, DirectionBoth},
    {QStringLiteral("nleq"), 0x2270, DirectionBoth},
    {QStringLiteral("ngeq"), 0x2271, DirectionBoth},
    {QStringLiteral("subset"), 0x2282, DirectionBoth},
    {QStringLiteral("supset"), 0x2283, DirectionBoth},
    {QStringLiteral("subseteq"), 0x2286, DirectionBoth},
    {QStringLiteral("supseteq"), 0x2287, DirectionBoth},
    {QStringLiteral("nsubseteq"), 0x2288, DirectionBoth},
    {QStringLiteral("nsupseteq"), 0x2289, DirectionBoth},
    {QStringLiteral("subsetneq"), 0x228A, DirectionBoth},
    {QStringLiteral("supsetneq"), 0x228A, DirectionBoth},
    {QStringLiteral("Subset"), 0x22D0, DirectionBoth},
    {QStringLiteral("Supset"), 0x22D1, DirectionBoth},
    {QStringLiteral("lll"), 0x22D8, DirectionBoth},
    {QStringLiteral("ggg"), 0x22D9, DirectionBoth},
    {QStringLiteral("top"), 0x22A4, DirectionBoth},
    {QStringLiteral("bot"), 0x22A5, DirectionBoth},
};


/**
 * This data structure holds commands representing a single
 * character. For example, it maps \AA to A with a ring (Nordic
 * letter) and back. The structure is a table with two columns:
 * (1) the command's name without a backslash (in the example
 * before the 'AA') (2) the Unicode character described by a
 * hexcode.
 */
static const struct EncoderLaTeXCharacterCommand {
    const QString command;
    const ushort unicode;
    const EncoderLaTeXCommandDirection direction;
}
encoderLaTeXCharacterCommands[] = {
    {QStringLiteral("textexclamdown"), 0x00A1, DirectionCommandToUnicode},
    {QStringLiteral("textcent"), 0x00A2, DirectionBoth},
    {QStringLiteral("pounds"), 0x00A3, DirectionBoth},
    {QStringLiteral("textsterling"), 0x00A3, DirectionBoth},
    /** 0x00A4 */
    {QStringLiteral("textyen"), 0x00A5, DirectionBoth},
    {QStringLiteral("textbrokenbar"), 0x00A6, DirectionBoth},
    {QStringLiteral("S"), 0x00A7, DirectionBoth},
    {QStringLiteral("textsection"), 0x00A7, DirectionBoth},
    /** 0x00A8 */
    {QStringLiteral("copyright"), 0x00A9, DirectionBoth},
    {QStringLiteral("textcopyright"), 0x00A9, DirectionBoth},
    {QStringLiteral("textordfeminine"), 0x00AA, DirectionBoth},
    {QStringLiteral("guillemotleft"), 0x00AB, DirectionCommandToUnicode},
    {QStringLiteral("textflqq"), 0x00AB, DirectionCommandToUnicode},
    {QStringLiteral("flqq"), 0x00AB, DirectionBoth},
    /** 0x00AC */
    /** 0x00AD */
    {QStringLiteral("textregistered"), 0x00AE, DirectionBoth},
    /** 0x00AF */
    {QStringLiteral("textdegree"), 0x00B0, DirectionBoth},
    {QStringLiteral("textpm"), 0x00B1, DirectionBoth},
    {QStringLiteral("textplusminus"), 0x00B1, DirectionCommandToUnicode},
    /** 0x00B2 */
    /** 0x00B3 */
    /** 0x00B4 */
    // Notes about Unicode U+00B5 ('micro sign'):
    // - Derived from the Greek 'mu' but used as a SI prefix meaning 'one millionth'
    // - Unicode differs between this symbol and a 'real' Greek 'mu' which has position U+03BC
    // - There are more lower case 'mu' in Unicode for mathematics (bold, italics, sans-serif, ...)
    //   at position U+1D6CD and later; those are not supported at all by KBibTeX
    {QStringLiteral("textmu"), 0x00B5, DirectionUnicodeToCommand},
    {QStringLiteral("textparagraph"), 0x00B6, DirectionBoth},
    {QStringLiteral("textpilcrow"), 0x00B6, DirectionBoth},
    {QStringLiteral("textperiodcentered"), 0x00B7, DirectionCommandToUnicode},
    {QStringLiteral("textcdot"), 0x00B7, DirectionBoth},
    {QStringLiteral("textcentereddot"), 0x00B7, DirectionCommandToUnicode},
    /** 0x00B8 */
    /** 0x00B9 */
    {QStringLiteral("textordmasculine"), 0x00BA, DirectionBoth},
    {QStringLiteral("guillemotright"), 0x00BB, DirectionCommandToUnicode},
    {QStringLiteral("textfrqq"), 0x00BB, DirectionCommandToUnicode},
    {QStringLiteral("frqq"), 0x00BB, DirectionBoth},
    {QStringLiteral("textonequarter"), 0x00BC, DirectionBoth},
    {QStringLiteral("textonehalf"), 0x00BD, DirectionBoth},
    {QStringLiteral("textthreequarters"), 0x00BE, DirectionBoth},
    {QStringLiteral("textquestiondown"), 0x00BF, DirectionCommandToUnicode}, // TODO /// recommended to write  ?`  instead of  \textquestiondown
    {QStringLiteral("AA"), 0x00C5, DirectionBoth},
    {QStringLiteral("AE"), 0x00C6, DirectionBoth},
    {QStringLiteral("DH"), 0x00D0, DirectionBoth},
    {QStringLiteral("texttimes"), 0x00D7, DirectionBoth},
    {QStringLiteral("textmultiply"), 0x00D7, DirectionCommandToUnicode},
    {QStringLiteral("O"), 0x00D8, DirectionBoth},
    {QStringLiteral("TH"), 0x00DE, DirectionBoth},
    {QStringLiteral("Thorn"), 0x00DE, DirectionCommandToUnicode},
    {QStringLiteral("textThorn"), 0x00DE, DirectionCommandToUnicode},
    {QStringLiteral("ss"), 0x00DF, DirectionBoth},
    {QStringLiteral("aa"), 0x00E5, DirectionBoth},
    {QStringLiteral("ae"), 0x00E6, DirectionBoth},
    {QStringLiteral("dh"), 0x00F0, DirectionBoth},
    {QStringLiteral("textdiv"), 0x00F7, DirectionBoth},
    {QStringLiteral("textdivide"), 0x00F7, DirectionCommandToUnicode},
    {QStringLiteral("o"), 0x00F8, DirectionBoth},
    {QStringLiteral("th"), 0x00FE, DirectionBoth},
    {QStringLiteral("textthorn"), 0x00FE, DirectionCommandToUnicode},
    {QStringLiteral("textthornvari"), 0x00FE, DirectionCommandToUnicode},
    {QStringLiteral("textthornvarii"), 0x00FE, DirectionCommandToUnicode},
    {QStringLiteral("textthornvariii"), 0x00FE, DirectionCommandToUnicode},
    {QStringLiteral("textthornvariv"), 0x00FE, DirectionCommandToUnicode},
    {QStringLiteral("Aogonek"), 0x0104, DirectionCommandToUnicode},
    {QStringLiteral("aogonek"), 0x0105, DirectionCommandToUnicode},
    {QStringLiteral("DJ"), 0x0110, DirectionBoth},
    {QStringLiteral("dj"), 0x0111, DirectionBoth},
    {QStringLiteral("textcrd"), 0x0111, DirectionCommandToUnicode},
    {QStringLiteral("textHslash"), 0x0126, DirectionCommandToUnicode},
    {QStringLiteral("textHbar"), 0x0126, DirectionCommandToUnicode},
    {QStringLiteral("textcrh"), 0x0127, DirectionCommandToUnicode},
    {QStringLiteral("texthbar"), 0x0127, DirectionCommandToUnicode},
    {QStringLiteral("i"), 0x0131, DirectionBoth},
    {QStringLiteral("IJ"), 0x0132, DirectionBoth},
    {QStringLiteral("ij"), 0x0133, DirectionBoth},
    {QStringLiteral("textkra"), 0x0138, DirectionCommandToUnicode},
    {QStringLiteral("Lcaron"), 0x013D, DirectionCommandToUnicode},
    {QStringLiteral("lcaron"), 0x013E, DirectionCommandToUnicode},
    {QStringLiteral("L"), 0x0141, DirectionBoth},
    {QStringLiteral("Lstroke"), 0x0141, DirectionCommandToUnicode},
    {QStringLiteral("l"), 0x0142, DirectionBoth},
    {QStringLiteral("lstroke"), 0x0142, DirectionCommandToUnicode},
    {QStringLiteral("textbarl"), 0x0142, DirectionCommandToUnicode},
    {QStringLiteral("NG"), 0x014A, DirectionBoth},
    {QStringLiteral("ng"), 0x014B, DirectionBoth},
    {QStringLiteral("OE"), 0x0152, DirectionBoth},
    {QStringLiteral("oe"), 0x0153, DirectionBoth},
    {QStringLiteral("Racute"), 0x0154, DirectionCommandToUnicode},
    {QStringLiteral("racute"), 0x0155, DirectionCommandToUnicode},
    {QStringLiteral("Sacute"), 0x015A, DirectionCommandToUnicode},
    {QStringLiteral("sacute"), 0x015B, DirectionCommandToUnicode},
    {QStringLiteral("Scedilla"), 0x015E, DirectionCommandToUnicode},
    {QStringLiteral("scedilla"), 0x015F, DirectionCommandToUnicode},
    {QStringLiteral("Scaron"), 0x0160, DirectionCommandToUnicode},
    {QStringLiteral("scaron"), 0x0161, DirectionCommandToUnicode},
    {QStringLiteral("Tcaron"), 0x0164, DirectionCommandToUnicode},
    {QStringLiteral("tcaron"), 0x0165, DirectionCommandToUnicode},
    {QStringLiteral("textTstroke"), 0x0166, DirectionCommandToUnicode},
    {QStringLiteral("textTbar"), 0x0166, DirectionCommandToUnicode},
    {QStringLiteral("textTslash"), 0x0166, DirectionCommandToUnicode},
    {QStringLiteral("texttstroke"), 0x0167, DirectionCommandToUnicode},
    {QStringLiteral("texttbar"), 0x0167, DirectionCommandToUnicode},
    {QStringLiteral("texttslash"), 0x0167, DirectionCommandToUnicode},
    {QStringLiteral("Zdotaccent"), 0x017B, DirectionCommandToUnicode},
    {QStringLiteral("zdotaccent"), 0x017C, DirectionCommandToUnicode},
    {QStringLiteral("Zcaron"), 0x017D, DirectionCommandToUnicode},
    {QStringLiteral("zcaron"), 0x017E, DirectionCommandToUnicode},
    {QStringLiteral("textlongs"), 0x017F, DirectionCommandToUnicode},
    {QStringLiteral("textcrb"), 0x0180, DirectionCommandToUnicode},
    {QStringLiteral("textBhook"), 0x0181, DirectionCommandToUnicode},
    {QStringLiteral("texthausaB"), 0x0181, DirectionCommandToUnicode},
    {QStringLiteral("textOopen"), 0x0186, DirectionCommandToUnicode},
    {QStringLiteral("textChook"), 0x0187, DirectionCommandToUnicode},
    {QStringLiteral("textchook"), 0x0188, DirectionCommandToUnicode},
    {QStringLiteral("texthtc"), 0x0188, DirectionCommandToUnicode},
    {QStringLiteral("textDafrican"), 0x0189, DirectionCommandToUnicode},
    {QStringLiteral("textDhook"), 0x018A, DirectionCommandToUnicode},
    {QStringLiteral("texthausaD"), 0x018A, DirectionCommandToUnicode},
    {QStringLiteral("textEreversed"), 0x018E, DirectionCommandToUnicode},
    {QStringLiteral("textrevE"), 0x018E, DirectionCommandToUnicode},
    {QStringLiteral("textEopen"), 0x0190, DirectionCommandToUnicode},
    {QStringLiteral("textFhook"), 0x0191, DirectionCommandToUnicode},
    {QStringLiteral("textflorin"), 0x0192, DirectionBoth},
    {QStringLiteral("textgamma"), 0x0194, DirectionCommandToUnicode},
    {QStringLiteral("textGammaafrican"), 0x0194, DirectionCommandToUnicode},
    {QStringLiteral("hv"), 0x0195, DirectionCommandToUnicode},
    {QStringLiteral("texthvlig"), 0x0195, DirectionCommandToUnicode},
    {QStringLiteral("textIotaafrican"), 0x0196, DirectionCommandToUnicode},
    {QStringLiteral("textKhook"), 0x0198, DirectionCommandToUnicode},
    {QStringLiteral("texthausaK"), 0x0198, DirectionCommandToUnicode},
    {QStringLiteral("texthtk"), 0x0199, DirectionCommandToUnicode},
    {QStringLiteral("textkhook"), 0x0199, DirectionCommandToUnicode},
    {QStringLiteral("textbarl"), 0x019A, DirectionCommandToUnicode},
    {QStringLiteral("textcrlambda"), 0x019B, DirectionCommandToUnicode},
    {QStringLiteral("textNhookleft"), 0x019D, DirectionCommandToUnicode},
    {QStringLiteral("textnrleg"), 0x019E, DirectionCommandToUnicode},
    {QStringLiteral("textPUnrleg"), 0x019E, DirectionCommandToUnicode},
    {QStringLiteral("Ohorn"), 0x01A0, DirectionCommandToUnicode},
    {QStringLiteral("ohorn"), 0x01A1, DirectionCommandToUnicode},
    {QStringLiteral("textPhook"), 0x01A4, DirectionCommandToUnicode},
    {QStringLiteral("texthtp"), 0x01A5, DirectionCommandToUnicode},
    {QStringLiteral("textphook"), 0x01A5, DirectionCommandToUnicode},
    {QStringLiteral("ESH"), 0x01A9, DirectionCommandToUnicode},
    {QStringLiteral("textEsh"), 0x01A9, DirectionCommandToUnicode},
    {QStringLiteral("textlooptoprevsh"), 0x01AA, DirectionCommandToUnicode},
    {QStringLiteral("textlhtlongi"), 0x01AA, DirectionCommandToUnicode},
    {QStringLiteral("textlhookt"), 0x01AB, DirectionCommandToUnicode},
    {QStringLiteral("textThook"), 0x01AC, DirectionCommandToUnicode},
    {QStringLiteral("textthook"), 0x01AD, DirectionCommandToUnicode},
    {QStringLiteral("texthtt"), 0x01AD, DirectionCommandToUnicode},
    {QStringLiteral("textTretroflexhook"), 0x01AE, DirectionCommandToUnicode},
    {QStringLiteral("Uhorn"), 0x01AF, DirectionCommandToUnicode},
    {QStringLiteral("uhorn"), 0x01B0, DirectionCommandToUnicode},
    {QStringLiteral("textupsilon"), 0x01B1, DirectionCommandToUnicode},
    {QStringLiteral("textVhook"), 0x01B2, DirectionCommandToUnicode},
    {QStringLiteral("textYhook"), 0x01B3, DirectionCommandToUnicode},
    {QStringLiteral("textvhook"), 0x01B4, DirectionCommandToUnicode},
    {QStringLiteral("Zbar"), 0x01B5, DirectionCommandToUnicode},
    {QStringLiteral("zbar"), 0x01B6, DirectionCommandToUnicode},
    {QStringLiteral("EZH"), 0x01B7, DirectionCommandToUnicode},
    {QStringLiteral("textEzh"), 0x01B7, DirectionCommandToUnicode},
    {QStringLiteral("LJ"), 0x01C7, DirectionCommandToUnicode},
    {QStringLiteral("Lj"), 0x01C8, DirectionCommandToUnicode},
    {QStringLiteral("lj"), 0x01C9, DirectionCommandToUnicode},
    {QStringLiteral("NJ"), 0x01CA, DirectionCommandToUnicode},
    {QStringLiteral("Nj"), 0x01CB, DirectionCommandToUnicode},
    {QStringLiteral("nj"), 0x01CC, DirectionCommandToUnicode},
    {QStringLiteral("DZ"), 0x01F1, DirectionCommandToUnicode},
    {QStringLiteral("Dz"), 0x01F2, DirectionCommandToUnicode},
    {QStringLiteral("dz"), 0x01F3, DirectionCommandToUnicode},
    {QStringLiteral("HV"), 0x01F6, DirectionCommandToUnicode},
    {QStringLiteral("j"), 0x0237, DirectionBoth},
    // Notes about Unicode U+03BC ('Greek small letter mu'):
    // - Unicode differs between this symbol and a 'micro' (SI-prefix) which has position U+00B5
    // - There are more lower case 'mu' in Unicode for mathematics (bold, italics, sans-serif, ...)
    //   at position U+1D6CD and later; those are not supported at all by KBibTeX
    // - LaTeX package 'textcomp' provides command '\textmu' but no other Greek letters
    // - LaTeX package 'textgreek' provides commands for all Greek letters (e.g. '\textpi') but
    //   to avoid conflicts with 'textcomp', the command for 'mu' is '\textmugreek'
    {QStringLiteral("textmugreek"), 0x03BC, DirectionCommandToUnicode},
    {QStringLiteral("textmu"), 0x03BC, DirectionBoth},
    {QStringLiteral("ldots"), 0x2026, DirectionBoth},
    {QStringLiteral("grqq"), 0x201C, DirectionCommandToUnicode},
    {QStringLiteral("textquotedblleft"), 0x201C, DirectionCommandToUnicode},
    {QStringLiteral("rqq"), 0x201D, DirectionCommandToUnicode},
    {QStringLiteral("textquotedblright"), 0x201D, DirectionCommandToUnicode},
    {QStringLiteral("glqq"), 0x201E, DirectionCommandToUnicode},
    {QStringLiteral("SS"), 0x1E9E, DirectionBoth},
    {QStringLiteral("textendash"), 0x2013, DirectionCommandToUnicode},
    {QStringLiteral("textemdash"), 0x2014, DirectionCommandToUnicode},
    {QStringLiteral("textquoteleft"), 0x2018, DirectionCommandToUnicode},
    {QStringLiteral("lq"), 0x2018, DirectionBoth},
    {QStringLiteral("textquoteright"), 0x2019, DirectionCommandToUnicode},
    {QStringLiteral("rq"), 0x2019, DirectionBoth}, ///< tricky one: 'r' is a valid modifier
    {QStringLiteral("quotesinglbase"), 0x201A, DirectionBoth},
    {QStringLiteral("quotedblbase"), 0x201E, DirectionBoth},
    {QStringLiteral("textbullet "), 0x2022, DirectionBoth},
    {QStringLiteral("guilsinglleft "), 0x2039, DirectionBoth},
    {QStringLiteral("guilsinglright "), 0x203A, DirectionBoth},
    {QStringLiteral("textcelsius"), 0x2103, DirectionBoth},
    {QStringLiteral("textleftarrow"), 0x2190, DirectionBoth},
    {QStringLiteral("textuparrow"), 0x2191, DirectionBoth},
    {QStringLiteral("textrightarrow"), 0x2192, DirectionBoth},
    {QStringLiteral("textdownarrow"), 0x2193, DirectionBoth}
};

const QChar EncoderLaTeX::encoderLaTeXProtectedSymbols[] = {u'#', u'&', u'%'};

const QChar EncoderLaTeX::encoderLaTeXProtectedTextOnlySymbols[] = {u'_'};


/**
 * This data structure holds LaTeX symbol sequences (without
 * any backslash) that represent a single Unicode character.
 * For example, it maps --- to an 'em dash' and back.
 * The structure is a table with two columns: (1) the symbol
 * sequence (in the example before the '---') (2) the Unicode
 * character described by a hexcode.
 */
static const struct EncoderLaTeXSymbolSequence {
    const QString latex;
    const ushort unicode;
    const EncoderLaTeXCommandDirection direction;
}
encoderLaTeXSymbolSequences[] = {
    {QStringLiteral("!`"), 0x00A1, DirectionBoth},
    {QStringLiteral("\"<"), 0x00AB, DirectionBoth},
    {QStringLiteral("\">"), 0x00BB, DirectionBoth},
    {QStringLiteral("?`"), 0x00BF, DirectionBoth},
    {QStringLiteral("---"), 0x2014, DirectionBoth}, ///< --- must come before --
    {QStringLiteral("--"), 0x2013, DirectionBoth},
    {QStringLiteral("``"), 0x201C, DirectionBoth},
    {QStringLiteral("''"), 0x201D, DirectionBoth},
    {QStringLiteral("ff"), 0xFB00, DirectionUnicodeToCommand},
    {QStringLiteral("fi"), 0xFB01, DirectionUnicodeToCommand},
    {QStringLiteral("fl"), 0xFB02, DirectionUnicodeToCommand},
    {QStringLiteral("ffi"), 0xFB03, DirectionUnicodeToCommand},
    {QStringLiteral("ffl"), 0xFB04, DirectionUnicodeToCommand},
    {QStringLiteral("ft"), 0xFB05, DirectionUnicodeToCommand},
    {QStringLiteral("st"), 0xFB06, DirectionUnicodeToCommand}
};


EncoderLaTeX::EncoderLaTeX()
        : Encoder()
{
    /// Initialize lookup table with NULL pointers
    for (int i = 0; i < lookupTableNumModifiers; ++i) lookupTable[i] = nullptr;

    int lookupTableCount = 0;
    /// Go through all table rows of encoderLaTeXEscapedCharacters
    for (const EncoderLaTeXEscapedCharacter &encoderLaTeXEscapedCharacter : encoderLaTeXEscapedCharacters) {
        /// Check if this row's modifier is already known
        bool knownModifier = false;
        int j;
        for (j = lookupTableCount - 1; j >= 0; --j) {
            knownModifier |= lookupTable[j]->modifier == encoderLaTeXEscapedCharacter.modifier;
            if (knownModifier) break;
        }

        if (!knownModifier) {
            /// Ok, this row's modifier appeared for the first time,
            /// therefore initialize memory structure, i.e. row in lookupTable
            lookupTable[lookupTableCount] = new EncoderLaTeXEscapedCharacterLookupTableRow;
            lookupTable[lookupTableCount]->modifier = encoderLaTeXEscapedCharacter.modifier;
            /// If no special character is known for a letter+modifier
            /// combination, fall back using the ASCII character only
            for (ushort k = 0; k < 26; ++k) {
                lookupTable[lookupTableCount]->unicode[k] = QChar(QChar(u'A').unicode() + k);
                lookupTable[lookupTableCount]->unicode[k + 26] = QChar(QChar(u'a').unicode() + k);
            }
            for (ushort k = 0; k < 10; ++k)
                lookupTable[lookupTableCount]->unicode[k + 52] = QChar(QChar(u'0').unicode() + k);
            j = lookupTableCount;
            ++lookupTableCount;
        }

        /// Add the letter as of the current row in encoderLaTeXEscapedCharacters
        /// into Unicode char array in the current modifier's row in the lookup table.
        int pos = -1;
        if ((pos = asciiLetterOrDigitToPos(encoderLaTeXEscapedCharacter.letter)) >= 0)
            lookupTable[j]->unicode[pos] = QChar(encoderLaTeXEscapedCharacter.unicode);
        else
            qCWarning(LOG_KBIBTEX_IO) << "Cannot handle letter " << encoderLaTeXEscapedCharacter.letter;
    }
}

EncoderLaTeX::~EncoderLaTeX()
{
    /// Clean-up memory
    for (int i = lookupTableNumModifiers - 1; i >= 0; --i)
        if (lookupTable[i] != nullptr)
            delete lookupTable[i];
}

QString EncoderLaTeX::decode(const QString &input) const
{
    const int len = input.length();
    QString output;
    output.reserve(((len >> 10) + 2) << 10); // reserving multiples of 1024 Bytes
    enum MathMode {
        MathModeNone = 0, MathModeDollar, MathModeEnsureMath
    };
    QStack<MathMode> currentMathMode;
#define currentMathModeTop()  (currentMathMode.empty()?MathModeNone:currentMathMode.top())
    int openCurlyBracketCounterEnsureMath = 0;
    QStack<int> popEnsureMathAtOpenCurlyBacketCounter;
    int cachedAsciiLetterOrDigitToPos = -1;

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
        const QChar c = input[i];

        if (c == u'{') {
            /// First case: An opening curly bracket,
            /// which is harmless (see else case), unless ...
            if (i < len - 3 && input[i + 1] == u'\\') {
                /// ... it continues with a backslash

                /// Next, check if there follows a modifier after the backslash
                /// For example an quotation mark as used in {\"a}
                const int lookupTablePos = modifierInLookupTable(input[i + 2]);

                /// Check for spaces between modifier and character, for example
                /// like {\H o}
                int skipSpaces = 0;
                while (i + 3 + skipSpaces < len && input[i + 3 + skipSpaces] == u' ' && skipSpaces < 16) ++skipSpaces;

                bool found = false;
                if (lookupTablePos >= 0 && (skipSpaces > 0 || !input[i + 2].isLetter()) && i + skipSpaces < len - 4 && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 3 + skipSpaces])) >= 0 && input[i + 4 + skipSpaces] == u'}') {
                    /// If we found a modifier which is followed by
                    /// a letter followed by a closing curly bracket,
                    /// we are looking at something like {\"A}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                    if (unicodeLetter.unicode() >= 127) {
                        output.append(unicodeLetter);
                        /// Step over those additional characters
                        i += 4 + skipSpaces;
                        found = true;
                    }
                    /// Don't print any warnings yet, as this if-case may got triggered by e.g. \mu
                    /// ('m' is a potential modifier, yet \mu should be recognized as Greek letter later)
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 5 && input[i + 3 + skipSpaces] == u'\\' && isIJ(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == u'}') {
                    /// This is the case for {\'\i} or alike.
                    for (const DotlessIJCharacter &dotlessIJCharacter : dotlessIJCharacters)
                        if ((dotlessIJCharacter.direction & DirectionCommandToUnicode) && dotlessIJCharacter.letter == input[i + 4 + skipSpaces] && dotlessIJCharacter.modifier == input[i + 2]) {
                            output.append(QChar(dotlessIJCharacter.unicode));
                            found = true;
                            break;
                        }
                    if (!found) {
                        /// This combination of modifier and letter is not known,
                        /// so try to preserve it
                        output.append(QStringView{input}.mid(i, 6 + skipSpaces));
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot interpret BACKSLASH" << input[i + 2] << "BACKSLASH" << input[i + 4 + skipSpaces];
                    }
                    /// Step over those additional characters
                    i += 5 + skipSpaces;
                    found = true;
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 6 && input[i + 3 + skipSpaces] == u'{' && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 4 + skipSpaces])) >= 0 && input[i + 5 + skipSpaces] == u'}' && input[i + 6 + skipSpaces] == u'}') {
                    /// If we found a modifier which is followed by
                    /// an opening curly bracket followed by a letter
                    /// followed by two closing curly brackets,
                    /// we are looking at something like {\"{A}}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                    if (unicodeLetter.unicode() < 127) {
                        /// This combination of modifier and letter is not known,
                        /// so try to preserve it
                        output.append(QStringView{input}.mid(i, 7 + skipSpaces));
                        qCDebug(LOG_KBIBTEX_IO) << input.mid(qMax(0, i - 5), 10);
                        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 7 + skipSpaces);
                    } else
                        output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 6 + skipSpaces;
                    found = true;
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 7 && input[i + 3 + skipSpaces] == u'{' && input[i + 4 + skipSpaces] == u'\\' && isIJ(input[i + 5 + skipSpaces]) && input[i + 6 + skipSpaces] == u'}' && input[i + 7 + skipSpaces] == u'}') {
                    /// This is the case for {\'{\i}} or alike.
                    for (const DotlessIJCharacter &dotlessIJCharacter : dotlessIJCharacters)
                        if ((dotlessIJCharacter.direction & DirectionCommandToUnicode) && dotlessIJCharacter.letter == input[i + 5 + skipSpaces] && dotlessIJCharacter.modifier == input[i + 2]) {
                            output.append(QChar(dotlessIJCharacter.unicode));
                            found = true;
                            break;
                        }
                    if (!found) {
                        /// This combination of modifier and letter is not known,
                        /// so try to preserve it
                        output.append(QStringView{input}.mid(i, 8 + skipSpaces));
                        qCDebug(LOG_KBIBTEX_IO) << input.mid(qMax(0, i - 5), 10);
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot interpret BACKSLASH" << input[i + 2] << "BACKSLASH {" << input[i + 5 + skipSpaces] << "}";
                    }
                    /// Step over those additional characters
                    i += 7 + skipSpaces;
                    found = true;
                }

                if (!found) {
                    /// Now, either some two-letter command like {\AA} or {\mu} is left
                    /// to check for or there is completely unsuppored command sequence,
                    /// but which then should be kept unmodified
                    const QString alpha = readAlphaCharacters(input, i + 2);
                    int nextPosAfterAlpha = i + 2 + alpha.size();
                    if (nextPosAfterAlpha < input.length() && input[nextPosAfterAlpha] == u'}') {
                        /// We may deal with a string like {\AA} or {\mu}
                        /// Check which command it is, then insert corresponding Unicode character
                        found = false;
                        for (const EncoderLaTeXCharacterCommand &encoderLaTeXCharacterCommand : encoderLaTeXCharacterCommands) {
                            if ((encoderLaTeXCharacterCommand.direction & DirectionCommandToUnicode) && encoderLaTeXCharacterCommand.command == alpha) {
                                output.append(QChar(encoderLaTeXCharacterCommand.unicode));
                                found = true;
                                break;
                            }
                        }

                        /// Check if a math command has been read,
                        /// like \subset
                        /// (automatically skipped if command was found above)
                        if (!found)
                            for (const MathCommand &mathCommand : mathCommands) {
                                if ((mathCommand.direction & DirectionCommandToUnicode) && mathCommand.command == alpha) {
                                    output.append(QChar(mathCommand.unicode));
                                    found = true;
                                    break;
                                }
                            }

                        if (!found) {
                            /// Dealing with a string like {\noopsort}
                            /// (see BibTeX documentation where this gets explained)
                            output.append(QStringView{input}.mid(i, 3 + alpha.size()));
                        }
                        i = nextPosAfterAlpha;
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
        } else if (c == u'\\' && i < len - 1) {
            /// Second case: A backslash as in \"o

            /// Sometimes such command are closed with just {},
            /// so remember if to check for that
            bool checkForExtraCurlyAtEnd = false;

            /// Check if there follows a modifier after the backslash
            /// For example an quotation mark as used in \"a
            const int lookupTablePos = modifierInLookupTable(input[i + 1]);

            /// Check for spaces between modifier and character, for example
            /// like \H o
            int skipSpaces = 0;
            while (i + 2 + skipSpaces < len && input[i + 2 + skipSpaces] == u' ' && skipSpaces < 16) ++skipSpaces;

            bool found = false;
            if (lookupTablePos >= 0 && (skipSpaces > 0 || !input[i + 1].isLetter()) && i + skipSpaces <= len - 3 && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 2 + skipSpaces])) >= 0) {
                /// We found a special modifier which is followed by
                /// a letter followed by normal text without any
                /// delimiter, so we are looking at something like
                /// \"u inside Kr\"uger
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                if (unicodeLetter.unicode() > 127) {
                    output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 2 + skipSpaces;
                    found = true;
                }
                /// Don't print any warnings yet, as this if-case may got triggered by e.g. \mu
                /// ('m' is a potential modifier, yet \mu should be recognized as Greek letter later)
            } else if (lookupTablePos >= 0 && (skipSpaces > 0 || !input[i + 1].isLetter()) && i + skipSpaces <= len - 3 && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 2 + skipSpaces])) >= 0 && (i + skipSpaces == len - 3 || input[i + 3 + skipSpaces] == u'}' || input[i + 3 + skipSpaces] == u'{' || input[i + 3 + skipSpaces] == u' ' || input[i + 3 + skipSpaces] == u'\t' || input[i + 3 + skipSpaces] == u'\\' || input[i + 3 + skipSpaces] == u'\r' || input[i + 3 + skipSpaces] == u'\n')) {
                /// We found a modifier which is followed by
                /// a letter followed by a command delimiter such
                /// as a whitespace, so we are looking at something
                /// like \"u followed by a space or another delimiter
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                if (unicodeLetter.unicode() >= 127) {
                    output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 2 + skipSpaces;
                    found = true;

                    if (input[i + 1] != u' ' && input[i + 1] != u'\r' && input[i + 1] != u'\n') {
                        /// If no whitespace follows, still
                        /// check for extra curly brackets
                        checkForExtraCurlyAtEnd = true;
                    }
                }
                /// Don't print any warnings yet, as this if-case may got triggered by e.g. \mu
                /// ('m' is a potential modifier, yet \mu should be recognized as Greek letter later)
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 4 && input[i + 2 + skipSpaces] == u'{' && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 3 + skipSpaces])) >= 0 && input[i + 4 + skipSpaces] == u'}') {
                /// We found a modifier which is followed by an opening
                /// curly bracket followed a letter followed by a closing
                /// curly bracket, so we are looking at something
                /// like \"{u}
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                if (unicodeLetter.unicode() < 127) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(QStringView{input}.mid(i, 5 + skipSpaces));
                    qCDebug(LOG_KBIBTEX_IO) << input.mid(qMax(0, i - 5), 10);
                    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 5 + skipSpaces);
                } else
                    output.append(unicodeLetter);
                /// Step over those additional characters
                i += 4 + skipSpaces;
                found = true;
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 3 && input[i + 2 + skipSpaces] == u'\\' && isIJ(input[i + 3 + skipSpaces])) {
                /// This is the case for \'\i or alike.
                for (const DotlessIJCharacter &dotlessIJCharacter : dotlessIJCharacters)
                    if ((dotlessIJCharacter.direction & DirectionCommandToUnicode) && dotlessIJCharacter.letter == input[i + 3 + skipSpaces] && dotlessIJCharacter.modifier == input[i + 1]) {
                        output.append(QChar(dotlessIJCharacter.unicode));
                        found = true;
                        break;
                    }
                if (!found) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(QStringView{input}.mid(i, 4 + skipSpaces));
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot interpret BACKSLASH" << input[i + 1] << "BACKSLASH" << input[i + 3 + skipSpaces];
                }
                /// Step over those additional characters
                i += 3 + skipSpaces;
                found = true;
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 5 && input[i + 2 + skipSpaces] == u'{' && input[i + 3 + skipSpaces] == u'\\' && isIJ(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == u'}') {
                /// This is the case for \'{\i} or alike.
                for (const DotlessIJCharacter &dotlessIJCharacter : dotlessIJCharacters)
                    if ((dotlessIJCharacter.direction & DirectionCommandToUnicode) && dotlessIJCharacter.letter == input[i + 4 + skipSpaces] && dotlessIJCharacter.modifier == input[i + 1]) {
                        output.append(QChar(dotlessIJCharacter.unicode));
                        found = true;
                        break;
                    }
                if (!found) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(QStringView{input}.mid(i, 6 + skipSpaces));
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot interpret BACKSLASH" << input[i + 1] << "BACKSLASH {" << input[i + 4 + skipSpaces] << "}";
                }
                /// Step over those additional characters
                i += 5 + skipSpaces;
                found = true;
            }

            if (!found && i < len - 1) {
                /// Now, the case of something like \AA is left
                /// to check for
                const QString alpha = readAlphaCharacters(input, i + 1);
                int nextPosAfterAlpha = i + alpha.size();
                if (alpha.size() >= 1 && alpha.at(0).isLetter()) {
                    /// We are dealing actually with a string like \AA or \o
                    /// Check which command it is,
                    /// insert corresponding Unicode character
                    for (const EncoderLaTeXCharacterCommand &encoderLaTeXCharacterCommand : encoderLaTeXCharacterCommands) {
                        if ((encoderLaTeXCharacterCommand.direction & DirectionCommandToUnicode) && encoderLaTeXCharacterCommand.command == alpha) {
                            output.append(QChar(encoderLaTeXCharacterCommand.unicode));
                            found = true;
                            break;
                        }
                    }

                    /// Check if a math command has been read,
                    /// like \subset
                    /// (automatically skipped if command was found above)
                    if (!found)
                        for (const MathCommand &mathCommand : mathCommands) {
                            if ((mathCommand.direction & DirectionCommandToUnicode) && mathCommand.command == alpha) {
                                if (currentMathModeTop() == MathModeNone)
                                    qCDebug(LOG_KBIBTEX_IO) << "Found math mode command" << QString(QStringLiteral("\\%1")).arg(alpha) << "outside of a math expression";
                                output.append(QChar(mathCommand.unicode));
                                found = true;
                                break;
                            }
                        }

                    if (found) {
                        /// Now, after a command, a whitespace may follow
                        /// which has to get "eaten" as it acts as a command
                        /// delimiter
                        if (nextPosAfterAlpha + 1 < input.length() && (input[nextPosAfterAlpha + 1] == u' ' || input[nextPosAfterAlpha + 1] == u'\r' || input[nextPosAfterAlpha + 1] == u'\n'))
                            ++nextPosAfterAlpha;
                        else {
                            /// If no whitespace follows, still
                            /// check for extra curly brackets
                            checkForExtraCurlyAtEnd = true;
                        }
                    } else {
                        /// No command found? Just copy input char to output
                        output.append(QStringView{input}.mid(i, 1 + alpha.size()));

                        if (alpha == QStringLiteral("ensuremath") && input[nextPosAfterAlpha + 1] == u'{') {
                            currentMathMode.push(MathModeEnsureMath);
                            popEnsureMathAtOpenCurlyBacketCounter.push(openCurlyBracketCounterEnsureMath);
                            ++openCurlyBracketCounterEnsureMath;
                            output.append(u'{');
                            ++nextPosAfterAlpha;
                        }
                    }
                    i = nextPosAfterAlpha;
                } else {
                    /// Maybe we are dealing with a string like \& or \_
                    /// Check which command it is
                    found = false;
                    for (const QChar &encoderLaTeXProtectedSymbol : encoderLaTeXProtectedSymbols)
                        if (encoderLaTeXProtectedSymbol == input[i + 1]) {
                            output.append(encoderLaTeXProtectedSymbol);
                            found = true;
                            break;
                        }

                    if (!found && currentMathModeTop() == MathModeNone)
                        for (const QChar &encoderLaTeXProtectedTextOnlySymbol : encoderLaTeXProtectedTextOnlySymbols)
                            if (encoderLaTeXProtectedTextOnlySymbol == input[i + 1]) {
                                output.append(encoderLaTeXProtectedTextOnlySymbol);
                                found = true;
                                break;
                            }

                    /// If command has been found, nothing has to be done
                    /// except for hopping over this backslash
                    if (found)
                        ++i;
                    else if (i < len - 1 && input[i + 1] == QChar(0x002c /* comma */)) {
                        /// Found a thin space: \,
                        /// Replacing Latex-like thin space with Unicode thin space
                        output.append(QChar(0x2009));
                        // found = true; ///< only necessary if more tests will follow in the future
                        ++i;
                        found = true;
                    } else {
                        /// Nothing special, copy input char to output
                        output.append(c);
                        found = true;
                    }
                }
            } else if (!found) {
                /// Nothing special, copy input char to output
                output.append(c);
            }

            /// Finally, check if there may be extra curly brackets
            /// like {} and hop over them
            if (checkForExtraCurlyAtEnd && i < len - 2 && input[i + 1] == u'{' && input[i + 2] == u'}')
                i += 2;
        } else {
            /// So far, no opening curly bracket and no backslash
            /// May still be a symbol sequence like ---
            bool isSymbolSequence = false;
            /// Go through all known symbol sequnces
            for (const EncoderLaTeXSymbolSequence &encoderLaTeXSymbolSequence : encoderLaTeXSymbolSequences) {
                /// First, check if read input character matches beginning of symbol sequence
                /// and input buffer as enough characters left to potentially contain
                /// symbol sequence
                const int latexLen = encoderLaTeXSymbolSequence.latex.length();
                if ((encoderLaTeXSymbolSequence.direction & DirectionCommandToUnicode) && encoderLaTeXSymbolSequence.latex[0] == c && i <= len - latexLen) {
                    /// Now actually check if symbol sequence is in input buffer
                    isSymbolSequence = true;
                    for (int p = 1; isSymbolSequence && p < latexLen; ++p)
                        isSymbolSequence &= encoderLaTeXSymbolSequence.latex[p] == input[i + p];
                    if (isSymbolSequence) {
                        /// Ok, found sequence: insert Unicode character in output
                        /// and hop over sequence in input buffer
                        output.append(QChar(encoderLaTeXSymbolSequence.unicode));
                        i += encoderLaTeXSymbolSequence.latex.length() - 1;
                        break;
                    }
                }
            }

            if (!isSymbolSequence) {
                /// No symbol sequence found, so just copy input to output
                output.append(c);

                /// Still, check if input character is a dollar sign
                /// without a preceding backslash, means toggling between
                /// text mode and math mode
                if (c == u'$' && (i == 0 || input[i - 1] != u'\\')) {
                    if (currentMathModeTop() == MathModeDollar)
                        currentMathMode.pop(); //< the Dollar sign that got just read closes the math mode
                    else
                        currentMathMode.push(MathModeDollar); //< the Dollar sign that got just read starts a new math mode
                }
                if (currentMathModeTop() == MathModeEnsureMath) {
                    if (c == u'{')
                        ++openCurlyBracketCounterEnsureMath;
                    else if (c == u'}')
                        --openCurlyBracketCounterEnsureMath;
                    if (!popEnsureMathAtOpenCurlyBacketCounter.empty() && openCurlyBracketCounterEnsureMath == popEnsureMathAtOpenCurlyBacketCounter.top()) {
                        popEnsureMathAtOpenCurlyBacketCounter.pop();
                        currentMathMode.pop();
                    }
                }
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
    if (pos < input.length() - 6 && QStringView{input}.mid(pos, 5) == QStringLiteral("\\url{")) {
        copyBytesCount = 5;
        openedClosedCurlyBrackets = 1;
    }

    if (copyBytesCount > 0) {
        while (openedClosedCurlyBrackets > 0 && pos + copyBytesCount < input.length()) {
            ++copyBytesCount;
            if (input[pos + copyBytesCount] == u'{' && input[pos + copyBytesCount - 1] != u'\\') ++openedClosedCurlyBrackets;
            else if (input[pos + copyBytesCount] == u'}' && input[pos + copyBytesCount - 1] != u'\\') --openedClosedCurlyBrackets;
        }

        output.append(QStringView{input}.mid(pos, copyBytesCount));
        pos += copyBytesCount;
    }

    return copyBytesCount > 0;
}

QString EncoderLaTeX::encode(const QString &ninput, const TargetEncoding targetEncoding) const
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString input = ninput.normalized(QString::NormalizationForm_C);
    if (targetEncoding == Encoder::TargetEncoding::RAW)
        // Nothing more to do?
        return input;

    int len = input.length();
    QString output;
    output.reserve(((len >> 10) + 2) << 10); // reserving multiples of 1024 Bytes
    enum MathMode {
        MathModeNone = 0, MathModeDollar, MathModeEnsureMath
    };
    QStack<MathMode> currentMathMode;
#define currentMathModeTop()  (currentMathMode.empty()?MathModeNone:currentMathMode.top())
    int openCurlyBracketCounterEnsureMath = 0;
    QStack<int> popEnsureMathAtOpenCurlyBacketCounter;

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

        if (targetEncoding == TargetEncoding::ASCII && c.unicode() > 127) {
            /// If current char is outside ASCII boundaries ...
            bool found = false;

            if (!found && !currentMathMode.empty()) {
                /// Ok, test for math commands if already in math mode
                for (const MathCommand &mathCommand : mathCommands)
                    if ((mathCommand.direction & DirectionUnicodeToCommand) && mathCommand.unicode == c.unicode()) {
                        output.append(QString(QStringLiteral("\\%1")).arg(mathCommand.command));
                        const QChar peekAhead = i < len - 1 ? input[i + 1] : QChar();
                        if (peekAhead != u'\\' && peekAhead != u'}' && peekAhead != u'$') {
                            // Between current command and following character a separator is necessary
                            // FIXME This peek-ahead won't do its job properly, as it is not yet known
                            // whether the next character will be kept as-is or rewritten to, for example, a LaTeX command
                            // Example: if the complete input string is '$µµ$' and the current variable 'c' comes from
                            // the first 'µ', it will assume that curly brackets are necessary, thus the final output
                            // becomes '$\mu{}\mu$ despite that '$\mu\mu$' would have been a better output.
                            output.append(QStringLiteral("{}"));
                        }
                        found = true;
                        break;
                    }
            }

            /// Handle special cases of i without a dot (\i)
            for (const DotlessIJCharacter &dotlessIJCharacter : dotlessIJCharacters)
                if ((dotlessIJCharacter.direction & DirectionUnicodeToCommand) && c.unicode() == dotlessIJCharacter.unicode) {
                    // FIXME Find a better solution, as the curly brackets are unnecessary in some situations
                    // e.g. '{\'\i}{\'\i}' should better be '{\'\i\'\i}'
                    output.append(QString(QStringLiteral("{\\%1\\%2}")).arg(dotlessIJCharacter.modifier, dotlessIJCharacter.letter));
                    found = true;
                    break;
                }

            if (!found) {
                /// ... test if there is a symbol sequence like ---
                /// to encode it
                for (const EncoderLaTeXSymbolSequence &encoderLaTeXSymbolSequence : encoderLaTeXSymbolSequences)
                    if (encoderLaTeXSymbolSequence.unicode == c.unicode() && (encoderLaTeXSymbolSequence.direction & DirectionUnicodeToCommand)) {
                        for (int l = 0; l < encoderLaTeXSymbolSequence.latex.length(); ++l)
                            output.append(encoderLaTeXSymbolSequence.latex[l]);
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, no symbol sequence. Let's test character
                /// commands like \ss
                for (const EncoderLaTeXCharacterCommand &encoderLaTeXCharacterCommand : encoderLaTeXCharacterCommands)
                    if (encoderLaTeXCharacterCommand.unicode == c.unicode() && (encoderLaTeXCharacterCommand.direction & DirectionUnicodeToCommand)) {
                        // FIXME Find a better solution, as the curly brackets are unnecessary in some situations
                        // e.g. '{\command}{\command}' should better be '{\command\command}'
                        output.append(QString(QStringLiteral("{\\%1}")).arg(encoderLaTeXCharacterCommand.command));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, neither a character command. Let's test
                /// escaped characters with modifiers like \"a
                for (const EncoderLaTeXEscapedCharacter &encoderLaTeXEscapedCharacter : encoderLaTeXEscapedCharacters)
                    if ((encoderLaTeXEscapedCharacter.direction & DirectionUnicodeToCommand) && encoderLaTeXEscapedCharacter.unicode == c.unicode()) {
                        // FIXME Find a better solution, as the curly brackets are unnecessary in some situations
                        // e.g. '{\"a}{\"a}' should better be '{\"a\"a}'
                        const QString formatString = isAsciiLetter(encoderLaTeXEscapedCharacter.modifier) ? QStringLiteral("{\\%1 %2}") : QStringLiteral("{\\%1%2}");
                        output.append(formatString.arg(encoderLaTeXEscapedCharacter.modifier).arg(encoderLaTeXEscapedCharacter.letter));
                        found = true;
                        break;
                    }
            }

            if (!found && currentMathMode.empty()) {
                /// Ok, test for math commands, even if outside of a math mode, then enter math mode for this character
                for (const MathCommand &mathCommand : mathCommands)
                    if ((mathCommand.direction & DirectionUnicodeToCommand) && mathCommand.unicode == c.unicode()) {
                        // FIXME Find a better solution, as the \ensuremath should span several characters
                        // e.g. '\ensuremath{\alpha}\ensuremath{\alpha}' should better be '\ensuremath{\alpha\alpha}'
                        output.append(QString(QStringLiteral("\\ensuremath{\\%1}")).arg(mathCommand.command));
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
                qCDebug(LOG_KBIBTEX_IO) << input.mid(qMax(0, i - 5), 10);
                qCWarning(LOG_KBIBTEX_IO) << "Don't know how to encode Unicode char" << QString(QStringLiteral("0x%1")).arg((uint)c.unicode(), 4, 16, QChar(u'0'));
                output.append(c);
            }
        } else if ((targetEncoding == TargetEncoding::ASCII && c.unicode() <= 127)
                   || targetEncoding == TargetEncoding::UTF8
                   /** but not  targetEncoding == TargetEncoding::RAW */) {
            /// Current character is normal ASCII
            /// and targetEncoding was set to accept only ASCII characters
            /// -- or -- targetEncoding was set to accept UTF-8 characters

            /// Still, some characters have special meaning
            /// in TeX and have to be preceded with a backslash
            bool found = false;
            for (const QChar &encoderLaTeXProtectedSymbol : encoderLaTeXProtectedSymbols)
                if (encoderLaTeXProtectedSymbol == c) {
                    output.append(u'\\').append(c);
                    found = true;
                    break;
                }

            if (!found && !currentMathMode.empty()) {
                /// Ok, test for math commands if already in math mode
                for (const MathCommand &mathCommand : mathCommands)
                    if ((mathCommand.direction & DirectionUnicodeToCommand) && mathCommand.unicode == c.unicode()) {
                        output.append(QString(QStringLiteral("\\%1")).arg(mathCommand.command));
                        const QChar peekAhead = i < len - 1 ? input[i + 1] : QChar();
                        if (peekAhead != u'\\' && peekAhead != u'}' && peekAhead != u'$') {
                            // Between current command and following character a separator is necessary
                            // FIXME This peek-ahead won't do its job properly, as it is not yet known
                            // whether the next character will be kept as-is or rewritten to, for example, a LaTeX command
                            // Example: if the complete input string is '$µµ$' and the current variable 'c' comes from
                            // the first 'µ', it will assume that curly brackets are necessary, thus the final output
                            // becomes '$\mu{}\mu$ despite that '$\mu\mu$' would have been a better output.
                            output.append(QStringLiteral("{}"));
                        }
                        found = true;
                        break;
                    }
            }

            if (!found && currentMathMode.empty())
                for (const QChar &encoderLaTeXProtectedTextOnlySymbol : encoderLaTeXProtectedTextOnlySymbols)
                    if (encoderLaTeXProtectedTextOnlySymbol == c) {
                        output.append(u'\\').append(c);
                        found = true;
                        break;
                    }

            if (!found) {
                /// Well, either this is not a special character or
                /// we do not know what to do with it, so just dump it into the output
                output.append(c);
                found = true;
            }

            /// Finally, check if input character is a dollar sign
            /// without a preceding backslash, means toggling between
            /// text mode and math mode
            if (c == u'$' && (i == 0 || input[i - 1] != u'\\')) {
                if (currentMathMode.empty())
                    currentMathMode.push(MathModeDollar);
                else if (currentMathModeTop() == MathModeDollar)
                    currentMathMode.pop();
                else if (currentMathModeTop() == MathModeEnsureMath)
                    currentMathMode.push(MathModeDollar);
            } else if (output.right(12) == QStringLiteral("\\ensuremath{")) {
                currentMathMode.push(MathModeEnsureMath);
                popEnsureMathAtOpenCurlyBacketCounter.push(openCurlyBracketCounterEnsureMath);
                // ++openCurlyBracketCounterEnsureMath; //< not necessary as right below both
                /// 'currentMathModeTop() == MathModeEnsureMath' and 'c == u'{''
                /// will be true
            }
            if (currentMathModeTop() == MathModeEnsureMath) {
                if (c == u'{')
                    ++openCurlyBracketCounterEnsureMath;
                else if (c == u'}')
                    --openCurlyBracketCounterEnsureMath;
                if (!popEnsureMathAtOpenCurlyBacketCounter.empty() && openCurlyBracketCounterEnsureMath == popEnsureMathAtOpenCurlyBacketCounter.top()) {
                    popEnsureMathAtOpenCurlyBacketCounter.pop();
                    currentMathMode.pop();
                }
            }
        }
    }

    output.squeeze();
    return output;
}

int EncoderLaTeX::modifierInLookupTable(const QChar modifier) const
{
    for (int m = 0; m < lookupTableNumModifiers && lookupTable[m] != nullptr; ++m)
        if (lookupTable[m]->modifier == modifier) return m;
    return -1;
}

QString EncoderLaTeX::readAlphaCharacters(const QString &base, int startFrom) const
{
    const int len = base.size();
    for (int j = startFrom; j < len; ++j) {
        if (!isAsciiLetter(base[j]))
            return base.mid(startFrom, j - startFrom);
    }
    return base.mid(startFrom);
}

const EncoderLaTeX &EncoderLaTeX::instance()
{
    static const EncoderLaTeX self;
    return self;
}
