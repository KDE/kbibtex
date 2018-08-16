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

#include "encoderlatex.h"

#include <QString>

#include "logging_io.h"

inline bool isAsciiLetter(const QChar c) {
    return (c.unicode() >= static_cast<ushort>('A') && c.unicode() <= static_cast<ushort>('Z')) || (c.unicode() >= static_cast<ushort>('a') && c.unicode() <= static_cast<ushort>('z'));
}

inline int asciiLetterOrDigitToPos(const QChar c) {
    static const ushort upperCaseLetterA = QLatin1Char('A').unicode();
    static const ushort upperCaseLetterZ = QLatin1Char('Z').unicode();
    static const ushort lowerCaseLetterA = QLatin1Char('a').unicode();
    static const ushort lowerCaseLetterZ = QLatin1Char('z').unicode();
    static const ushort digit0 = QLatin1Char('0').unicode();
    static const ushort digit9 = QLatin1Char('9').unicode();
    const ushort unicode = c.unicode();
    if (unicode >= upperCaseLetterA && unicode <= upperCaseLetterZ) return unicode - upperCaseLetterA;
    else if (unicode >= lowerCaseLetterA && unicode <= lowerCaseLetterZ) return unicode + 26 - lowerCaseLetterA;
    else if (unicode >= digit0 && unicode <= digit9) return unicode + 52 - digit0;
    else return -1;
}

inline bool isIJ(const QChar c) {
    static const QChar upperCaseLetterI = QLatin1Char('I');
    static const QChar upperCaseLetterJ = QLatin1Char('J');
    static const QChar lowerCaseLetterI = QLatin1Char('i');
    static const QChar lowerCaseLetterJ = QLatin1Char('j');
    return c == upperCaseLetterI || c == upperCaseLetterJ || c == lowerCaseLetterI || c == lowerCaseLetterJ;
}

enum EncoderLaTeXCommandDirection { DirectionCommandToUnicode = 1, DirectionUnicodeToCommand = 2, DirectionBoth = DirectionCommandToUnicode | DirectionUnicodeToCommand };

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
    const QChar modifier;
    const QChar letter;
    const ushort unicode;
}
encoderLaTeXEscapedCharacters[] = {
    {QLatin1Char('`'), QLatin1Char('A'), 0x00C0},
    {QLatin1Char('\''), QLatin1Char('A'), 0x00C1},
    {QLatin1Char('^'), QLatin1Char('A'), 0x00C2},
    {QLatin1Char('~'), QLatin1Char('A'), 0x00C3},
    {QLatin1Char('"'), QLatin1Char('A'), 0x00C4},
    {QLatin1Char('r'), QLatin1Char('A'), 0x00C5},
    /** 0x00C6: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('c'), QLatin1Char('C'), 0x00C7},
    {QLatin1Char('`'), QLatin1Char('E'), 0x00C8},
    {QLatin1Char('\''), QLatin1Char('E'), 0x00C9},
    {QLatin1Char('^'), QLatin1Char('E'), 0x00CA},
    {QLatin1Char('"'), QLatin1Char('E'), 0x00CB},
    {QLatin1Char('`'), QLatin1Char('I'), 0x00CC},
    {QLatin1Char('\''), QLatin1Char('I'), 0x00CD},
    {QLatin1Char('^'), QLatin1Char('I'), 0x00CE},
    {QLatin1Char('"'), QLatin1Char('I'), 0x00CF},
    /** 0x00D0: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('~'), QLatin1Char('N'), 0x00D1},
    {QLatin1Char('`'), QLatin1Char('O'), 0x00D2},
    {QLatin1Char('\''), QLatin1Char('O'), 0x00D3},
    {QLatin1Char('^'), QLatin1Char('O'), 0x00D4},
    {QLatin1Char('~'), QLatin1Char('O'), 0x00D5},
    {QLatin1Char('"'), QLatin1Char('O'), 0x00D6},
    /** 0x00D7: see EncoderLaTeXCharacterCommand */
    /** 0x00D8: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('`'), QLatin1Char('U'), 0x00D9},
    {QLatin1Char('\''), QLatin1Char('U'), 0x00DA},
    {QLatin1Char('^'), QLatin1Char('U'), 0x00DB},
    {QLatin1Char('"'), QLatin1Char('U'), 0x00DC},
    {QLatin1Char('\''), QLatin1Char('Y'), 0x00DD},
    /** 0x00DE: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('"'), QLatin1Char('s'), 0x00DF},
    {QLatin1Char('`'), QLatin1Char('a'), 0x00E0},
    {QLatin1Char('\''), QLatin1Char('a'), 0x00E1},
    {QLatin1Char('^'), QLatin1Char('a'), 0x00E2},
    {QLatin1Char('~'), QLatin1Char('a'), 0x00E3},
    {QLatin1Char('"'), QLatin1Char('a'), 0x00E4},
    {QLatin1Char('r'), QLatin1Char('a'), 0x00E5},
    /** 0x00E6: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('c'), QLatin1Char('c'), 0x00E7},
    {QLatin1Char('`'), QLatin1Char('e'), 0x00E8},
    {QLatin1Char('\''), QLatin1Char('e'), 0x00E9},
    {QLatin1Char('^'), QLatin1Char('e'), 0x00EA},
    {QLatin1Char('"'), QLatin1Char('e'), 0x00EB},
    {QLatin1Char('`'), QLatin1Char('i'), 0x00EC},
    {QLatin1Char('\''), QLatin1Char('i'), 0x00ED},
    {QLatin1Char('^'), QLatin1Char('i'), 0x00EE},
    {QLatin1Char('"'), QLatin1Char('i'), 0x00EF},
    /** 0x00F0: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('~'), QLatin1Char('n'), 0x00F1},
    {QLatin1Char('`'), QLatin1Char('o'), 0x00F2},
    {QLatin1Char('\''), QLatin1Char('o'), 0x00F3},
    {QLatin1Char('^'), QLatin1Char('o'), 0x00F4},
    {QLatin1Char('~'), QLatin1Char('o'), 0x00F5},
    {QLatin1Char('"'), QLatin1Char('o'), 0x00F6},
    /** 0x00F7: see EncoderLaTeXCharacterCommand */
    /** 0x00F8: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('`'), QLatin1Char('u'), 0x00F9},
    {QLatin1Char('\''), QLatin1Char('u'), 0x00FA},
    {QLatin1Char('^'), QLatin1Char('u'), 0x00FB},
    {QLatin1Char('"'), QLatin1Char('u'), 0x00FC},
    {QLatin1Char('\''), QLatin1Char('y'), 0x00FD},
    /** 0x00FE: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('"'), QLatin1Char('y'), 0x00FF},
    {QLatin1Char('='), QLatin1Char('A'), 0x0100},
    {QLatin1Char('='), QLatin1Char('a'), 0x0101},
    {QLatin1Char('u'), QLatin1Char('A'), 0x0102},
    {QLatin1Char('u'), QLatin1Char('a'), 0x0103},
    {QLatin1Char('k'), QLatin1Char('A'), 0x0104},
    {QLatin1Char('k'), QLatin1Char('a'), 0x0105},
    {QLatin1Char('\''), QLatin1Char('C'), 0x0106},
    {QLatin1Char('\''), QLatin1Char('c'), 0x0107},
    {QLatin1Char('^'), QLatin1Char('C'), 0x0108},
    {QLatin1Char('^'), QLatin1Char('c'), 0x0109},
    {QLatin1Char('.'), QLatin1Char('C'), 0x010A},
    {QLatin1Char('.'), QLatin1Char('c'), 0x010B},
    {QLatin1Char('v'), QLatin1Char('C'), 0x010C},
    {QLatin1Char('v'), QLatin1Char('c'), 0x010D},
    {QLatin1Char('v'), QLatin1Char('D'), 0x010E},
    {QLatin1Char('v'), QLatin1Char('d'), 0x010F},
    {QLatin1Char('B'), QLatin1Char('D'), 0x0110},
    {QLatin1Char('B'), QLatin1Char('d'), 0x0111},
    {QLatin1Char('='), QLatin1Char('E'), 0x0112},
    {QLatin1Char('='), QLatin1Char('e'), 0x0113},
    {QLatin1Char('u'), QLatin1Char('E'), 0x0114},
    {QLatin1Char('u'), QLatin1Char('e'), 0x0115},
    {QLatin1Char('.'), QLatin1Char('E'), 0x0116},
    {QLatin1Char('.'), QLatin1Char('e'), 0x0117},
    {QLatin1Char('k'), QLatin1Char('E'), 0x0118},
    {QLatin1Char('k'), QLatin1Char('e'), 0x0119},
    {QLatin1Char('v'), QLatin1Char('E'), 0x011A},
    {QLatin1Char('v'), QLatin1Char('e'), 0x011B},
    {QLatin1Char('^'), QLatin1Char('G'), 0x011C},
    {QLatin1Char('^'), QLatin1Char('g'), 0x011D},
    {QLatin1Char('u'), QLatin1Char('G'), 0x011E},
    {QLatin1Char('u'), QLatin1Char('g'), 0x011F},
    {QLatin1Char('.'), QLatin1Char('G'), 0x0120},
    {QLatin1Char('.'), QLatin1Char('g'), 0x0121},
    {QLatin1Char('c'), QLatin1Char('G'), 0x0122},
    {QLatin1Char('c'), QLatin1Char('g'), 0x0123},
    {QLatin1Char('^'), QLatin1Char('H'), 0x0124},
    {QLatin1Char('^'), QLatin1Char('h'), 0x0125},
    {QLatin1Char('B'), QLatin1Char('H'), 0x0126},
    {QLatin1Char('B'), QLatin1Char('h'), 0x0127},
    {QLatin1Char('~'), QLatin1Char('I'), 0x0128},
    {QLatin1Char('~'), QLatin1Char('i'), 0x0129},
    {QLatin1Char('='), QLatin1Char('I'), 0x012A},
    {QLatin1Char('='), QLatin1Char('i'), 0x012B},
    {QLatin1Char('u'), QLatin1Char('I'), 0x012C},
    {QLatin1Char('u'), QLatin1Char('i'), 0x012D},
    {QLatin1Char('k'), QLatin1Char('I'), 0x012E},
    {QLatin1Char('k'), QLatin1Char('i'), 0x012F},
    {QLatin1Char('.'), QLatin1Char('I'), 0x0130},
    /** 0x0131: see EncoderLaTeXCharacterCommand */
    /** 0x0132: see EncoderLaTeXCharacterCommand */
    /** 0x0133: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('^'), QLatin1Char('J'), 0x012E},
    {QLatin1Char('^'), QLatin1Char('j'), 0x012F},
    {QLatin1Char('c'), QLatin1Char('K'), 0x0136},
    {QLatin1Char('c'), QLatin1Char('k'), 0x0137},
    /** 0x0138: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('\''), QLatin1Char('L'), 0x0139},
    {QLatin1Char('\''), QLatin1Char('l'), 0x013A},
    {QLatin1Char('c'), QLatin1Char('L'), 0x013B},
    {QLatin1Char('c'), QLatin1Char('l'), 0x013C},
    {QLatin1Char('v'), QLatin1Char('L'), 0x013D},
    {QLatin1Char('v'), QLatin1Char('l'), 0x013E},
    {QLatin1Char('.'), QLatin1Char('L'), 0x013F},
    {QLatin1Char('.'), QLatin1Char('l'), 0x0140},
    {QLatin1Char('B'), QLatin1Char('L'), 0x0141},
    {QLatin1Char('B'), QLatin1Char('l'), 0x0142},
    {QLatin1Char('\''), QLatin1Char('N'), 0x0143},
    {QLatin1Char('\''), QLatin1Char('n'), 0x0144},
    {QLatin1Char('c'), QLatin1Char('n'), 0x0145},
    {QLatin1Char('c'), QLatin1Char('n'), 0x0146},
    {QLatin1Char('v'), QLatin1Char('N'), 0x0147},
    {QLatin1Char('v'), QLatin1Char('n'), 0x0148},
    /** 0x0149: TODO n preceded by apostrophe */
    {QLatin1Char('m'), QLatin1Char('N'), 0x014A},
    {QLatin1Char('m'), QLatin1Char('n'), 0x014B},
    {QLatin1Char('='), QLatin1Char('O'), 0x014C},
    {QLatin1Char('='), QLatin1Char('o'), 0x014D},
    {QLatin1Char('u'), QLatin1Char('O'), 0x014E},
    {QLatin1Char('u'), QLatin1Char('o'), 0x014F},
    {QLatin1Char('H'), QLatin1Char('O'), 0x0150},
    {QLatin1Char('H'), QLatin1Char('o'), 0x0151},
    /** 0x0152: see EncoderLaTeXCharacterCommand */
    /** 0x0153: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('\''), QLatin1Char('R'), 0x0154},
    {QLatin1Char('\''), QLatin1Char('r'), 0x0155},
    {QLatin1Char('c'), QLatin1Char('R'), 0x0156},
    {QLatin1Char('c'), QLatin1Char('r'), 0x0157},
    {QLatin1Char('v'), QLatin1Char('R'), 0x0158},
    {QLatin1Char('v'), QLatin1Char('r'), 0x0159},
    {QLatin1Char('\''), QLatin1Char('S'), 0x015A},
    {QLatin1Char('\''), QLatin1Char('s'), 0x015B},
    {QLatin1Char('^'), QLatin1Char('S'), 0x015C},
    {QLatin1Char('^'), QLatin1Char('s'), 0x015D},
    {QLatin1Char('c'), QLatin1Char('S'), 0x015E},
    {QLatin1Char('c'), QLatin1Char('s'), 0x015F},
    {QLatin1Char('v'), QLatin1Char('S'), 0x0160},
    {QLatin1Char('v'), QLatin1Char('s'), 0x0161},
    {QLatin1Char('c'), QLatin1Char('T'), 0x0162},
    {QLatin1Char('c'), QLatin1Char('t'), 0x0163},
    {QLatin1Char('v'), QLatin1Char('T'), 0x0164},
    {QLatin1Char('v'), QLatin1Char('t'), 0x0165},
    {QLatin1Char('B'), QLatin1Char('T'), 0x0166},
    {QLatin1Char('B'), QLatin1Char('t'), 0x0167},
    {QLatin1Char('~'), QLatin1Char('U'), 0x0168},
    {QLatin1Char('~'), QLatin1Char('u'), 0x0169},
    {QLatin1Char('='), QLatin1Char('U'), 0x016A},
    {QLatin1Char('='), QLatin1Char('u'), 0x016B},
    {QLatin1Char('u'), QLatin1Char('U'), 0x016C},
    {QLatin1Char('u'), QLatin1Char('u'), 0x016D},
    {QLatin1Char('r'), QLatin1Char('U'), 0x016E},
    {QLatin1Char('r'), QLatin1Char('u'), 0x016F},
    {QLatin1Char('H'), QLatin1Char('U'), 0x0170},
    {QLatin1Char('H'), QLatin1Char('u'), 0x0171},
    {QLatin1Char('k'), QLatin1Char('U'), 0x0172},
    {QLatin1Char('k'), QLatin1Char('u'), 0x0173},
    {QLatin1Char('^'), QLatin1Char('W'), 0x0174},
    {QLatin1Char('^'), QLatin1Char('w'), 0x0175},
    {QLatin1Char('^'), QLatin1Char('Y'), 0x0176},
    {QLatin1Char('^'), QLatin1Char('y'), 0x0177},
    {QLatin1Char('"'), QLatin1Char('Y'), 0x0178},
    {QLatin1Char('\''), QLatin1Char('Z'), 0x0179},
    {QLatin1Char('\''), QLatin1Char('z'), 0x017A},
    {QLatin1Char('.'), QLatin1Char('Z'), 0x017B},
    {QLatin1Char('.'), QLatin1Char('z'), 0x017C},
    {QLatin1Char('v'), QLatin1Char('Z'), 0x017D},
    {QLatin1Char('v'), QLatin1Char('z'), 0x017E},
    /** 0x017F: TODO long s */
    {QLatin1Char('B'), QLatin1Char('b'), 0x0180},
    {QLatin1Char('m'), QLatin1Char('B'), 0x0181},
    /** 0x0182 */
    /** 0x0183 */
    /** 0x0184 */
    /** 0x0185 */
    {QLatin1Char('m'), QLatin1Char('O'), 0x0186},
    {QLatin1Char('m'), QLatin1Char('C'), 0x0187},
    {QLatin1Char('m'), QLatin1Char('c'), 0x0188},
    {QLatin1Char('M'), QLatin1Char('D'), 0x0189},
    {QLatin1Char('m'), QLatin1Char('D'), 0x018A},
    /** 0x018B */
    /** 0x018C */
    /** 0x018D */
    {QLatin1Char('M'), QLatin1Char('E'), 0x018E},
    /** 0x018F */
    {QLatin1Char('m'), QLatin1Char('E'), 0x0190},
    {QLatin1Char('m'), QLatin1Char('F'), 0x0191},
    {QLatin1Char('m'), QLatin1Char('f'), 0x0192},
    /** 0x0193 */
    {QLatin1Char('m'), QLatin1Char('G'), 0x0194},
    /** 0x0195: see EncoderLaTeXCharacterCommand */
    {QLatin1Char('m'), QLatin1Char('I'), 0x0196},
    {QLatin1Char('B'), QLatin1Char('I'), 0x0197},
    {QLatin1Char('m'), QLatin1Char('K'), 0x0198},
    {QLatin1Char('m'), QLatin1Char('k'), 0x0199},
    {QLatin1Char('B'), QLatin1Char('l'), 0x019A},
    /** 0x019B */
    /** 0x019C */
    {QLatin1Char('m'), QLatin1Char('J'), 0x019D},
    /** 0x019E */
    /** 0x019F */
    /** 0x01A0 */
    /** 0x01A1 */
    /** 0x01A2 */
    /** 0x01A3 */
    {QLatin1Char('m'), QLatin1Char('P'), 0x01A4},
    {QLatin1Char('m'), QLatin1Char('p'), 0x01A5},
    /** 0x01A6 */
    /** 0x01A7 */
    /** 0x01A8 */
    /** 0x01A9: see EncoderLaTeXCharacterCommand */
    /** 0x01AA */
    /** 0x01AB */
    {QLatin1Char('m'), QLatin1Char('T'), 0x01AC},
    {QLatin1Char('m'), QLatin1Char('t'), 0x01AD},
    {QLatin1Char('M'), QLatin1Char('T'), 0x01AE},
    /** 0x01AF */
    /** 0x01B0 */
    {QLatin1Char('m'), QLatin1Char('U'), 0x01B1},
    {QLatin1Char('m'), QLatin1Char('V'), 0x01B2},
    {QLatin1Char('m'), QLatin1Char('Y'), 0x01B3},
    {QLatin1Char('m'), QLatin1Char('y'), 0x01B4},
    {QLatin1Char('B'), QLatin1Char('Z'), 0x01B5},
    {QLatin1Char('B'), QLatin1Char('z'), 0x01B6},
    {QLatin1Char('m'), QLatin1Char('Z'), 0x01B7},
    /** 0x01B8 */
    /** 0x01B9 */
    /** 0x01BA */
    {QLatin1Char('B'), QLatin1Char('2'), 0x01BB},
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
    {QLatin1Char('v'), QLatin1Char('A'), 0x01CD},
    {QLatin1Char('v'), QLatin1Char('a'), 0x01CE},
    {QLatin1Char('v'), QLatin1Char('G'), 0x01E6},
    {QLatin1Char('v'), QLatin1Char('g'), 0x01E7},
    {QLatin1Char('k'), QLatin1Char('O'), 0x01EA},
    {QLatin1Char('k'), QLatin1Char('o'), 0x01EB},
    {QLatin1Char('\''), QLatin1Char('F'), 0x01F4},
    {QLatin1Char('\''), QLatin1Char('f'), 0x01F5},
    {QLatin1Char('.'), QLatin1Char('A'), 0x0226},
    {QLatin1Char('.'), QLatin1Char('a'), 0x0227},
    {QLatin1Char('c'), QLatin1Char('E'), 0x0228},
    {QLatin1Char('c'), QLatin1Char('e'), 0x0229},
    {QLatin1Char('='), QLatin1Char('Y'), 0x0232},
    {QLatin1Char('='), QLatin1Char('y'), 0x0233},
    {QLatin1Char('.'), QLatin1Char('O'), 0x022E},
    {QLatin1Char('.'), QLatin1Char('o'), 0x022F},
    {QLatin1Char('.'), QLatin1Char('B'), 0x1E02},
    {QLatin1Char('.'), QLatin1Char('b'), 0x1E03},
    {QLatin1Char('d'), QLatin1Char('B'), 0x1E04},
    {QLatin1Char('d'), QLatin1Char('b'), 0x1E05},
    {QLatin1Char('.'), QLatin1Char('D'), 0x1E0A},
    {QLatin1Char('.'), QLatin1Char('d'), 0x1E0B},
    {QLatin1Char('d'), QLatin1Char('D'), 0x1E0C},
    {QLatin1Char('d'), QLatin1Char('d'), 0x1E0D},
    {QLatin1Char('c'), QLatin1Char('D'), 0x1E10},
    {QLatin1Char('c'), QLatin1Char('d'), 0x1E11},
    {QLatin1Char('.'), QLatin1Char('E'), 0x1E1E},
    {QLatin1Char('.'), QLatin1Char('e'), 0x1E1F},
    {QLatin1Char('.'), QLatin1Char('H'), 0x1E22},
    {QLatin1Char('.'), QLatin1Char('h'), 0x1E23},
    {QLatin1Char('d'), QLatin1Char('H'), 0x1E24},
    {QLatin1Char('d'), QLatin1Char('h'), 0x1E25},
    {QLatin1Char('"'), QLatin1Char('H'), 0x1E26},
    {QLatin1Char('"'), QLatin1Char('h'), 0x1E27},
    {QLatin1Char('c'), QLatin1Char('H'), 0x1E28},
    {QLatin1Char('c'), QLatin1Char('h'), 0x1E29},
    {QLatin1Char('d'), QLatin1Char('K'), 0x1E32},
    {QLatin1Char('d'), QLatin1Char('k'), 0x1E33},
    {QLatin1Char('d'), QLatin1Char('L'), 0x1E36},
    {QLatin1Char('d'), QLatin1Char('l'), 0x1E37},
    {QLatin1Char('.'), QLatin1Char('M'), 0x1E40},
    {QLatin1Char('.'), QLatin1Char('m'), 0x1E41},
    {QLatin1Char('d'), QLatin1Char('M'), 0x1E42},
    {QLatin1Char('d'), QLatin1Char('m'), 0x1E43},
    {QLatin1Char('.'), QLatin1Char('N'), 0x1E44},
    {QLatin1Char('.'), QLatin1Char('n'), 0x1E45},
    {QLatin1Char('.'), QLatin1Char('N'), 0x1E46},
    {QLatin1Char('.'), QLatin1Char('n'), 0x1E47},
    {QLatin1Char('.'), QLatin1Char('P'), 0x1E56},
    {QLatin1Char('.'), QLatin1Char('p'), 0x1E57},
    {QLatin1Char('.'), QLatin1Char('R'), 0x1E58},
    {QLatin1Char('.'), QLatin1Char('r'), 0x1E59},
    {QLatin1Char('d'), QLatin1Char('R'), 0x1E5A},
    {QLatin1Char('d'), QLatin1Char('r'), 0x1E5B},
    {QLatin1Char('.'), QLatin1Char('S'), 0x1E60},
    {QLatin1Char('.'), QLatin1Char('s'), 0x1E61},
    {QLatin1Char('d'), QLatin1Char('S'), 0x1E62},
    {QLatin1Char('d'), QLatin1Char('s'), 0x1E63},
    {QLatin1Char('.'), QLatin1Char('T'), 0x1E6A},
    {QLatin1Char('.'), QLatin1Char('t'), 0x1E6B},
    {QLatin1Char('d'), QLatin1Char('T'), 0x1E6C},
    {QLatin1Char('d'), QLatin1Char('t'), 0x1E6D},
    {QLatin1Char('d'), QLatin1Char('V'), 0x1E7E},
    {QLatin1Char('d'), QLatin1Char('v'), 0x1E7F},
    {QLatin1Char('`'), QLatin1Char('W'), 0x1E80},
    {QLatin1Char('`'), QLatin1Char('w'), 0x1E81},
    {QLatin1Char('\''), QLatin1Char('W'), 0x1E82},
    {QLatin1Char('\''), QLatin1Char('w'), 0x1E83},
    {QLatin1Char('"'), QLatin1Char('W'), 0x1E84},
    {QLatin1Char('"'), QLatin1Char('w'), 0x1E85},
    {QLatin1Char('.'), QLatin1Char('W'), 0x1E86},
    {QLatin1Char('.'), QLatin1Char('w'), 0x1E87},
    {QLatin1Char('d'), QLatin1Char('W'), 0x1E88},
    {QLatin1Char('d'), QLatin1Char('w'), 0x1E88},
    {QLatin1Char('.'), QLatin1Char('X'), 0x1E8A},
    {QLatin1Char('.'), QLatin1Char('x'), 0x1E8B},
    {QLatin1Char('"'), QLatin1Char('X'), 0x1E8C},
    {QLatin1Char('"'), QLatin1Char('x'), 0x1E8D},
    {QLatin1Char('.'), QLatin1Char('Y'), 0x1E8E},
    {QLatin1Char('.'), QLatin1Char('y'), 0x1E8F},
    {QLatin1Char('d'), QLatin1Char('Z'), 0x1E92},
    {QLatin1Char('d'), QLatin1Char('z'), 0x1E93},
    {QLatin1Char('"'), QLatin1Char('t'), 0x1E97},
    {QLatin1Char('r'), QLatin1Char('w'), 0x1E98},
    {QLatin1Char('r'), QLatin1Char('y'), 0x1E99},
    {QLatin1Char('d'), QLatin1Char('A'), 0x1EA0},
    {QLatin1Char('d'), QLatin1Char('a'), 0x1EA1},
    {QLatin1Char('d'), QLatin1Char('E'), 0x1EB8},
    {QLatin1Char('d'), QLatin1Char('e'), 0x1EB9},
    {QLatin1Char('d'), QLatin1Char('I'), 0x1ECA},
    {QLatin1Char('d'), QLatin1Char('i'), 0x1ECB},
    {QLatin1Char('d'), QLatin1Char('O'), 0x1ECC},
    {QLatin1Char('d'), QLatin1Char('o'), 0x1ECD},
    {QLatin1Char('d'), QLatin1Char('U'), 0x1EE4},
    {QLatin1Char('d'), QLatin1Char('u'), 0x1EE5},
    {QLatin1Char('`'), QLatin1Char('Y'), 0x1EF2},
    {QLatin1Char('`'), QLatin1Char('y'), 0x1EF3},
    {QLatin1Char('d'), QLatin1Char('Y'), 0x1EF4},
    {QLatin1Char('d'), QLatin1Char('y'), 0x1EF5},
    {QLatin1Char('r'), QLatin1Char('q'), 0x2019} ///< tricky: this is \rq
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
    const QChar modifier;
    const QChar letter;
    const ushort unicode;
}
dotlessIJCharacters[] = {
    {QLatin1Char('`'), QLatin1Char('i'), 0x00EC},
    {QLatin1Char('\''), QLatin1Char('i'), 0x00ED},
    {QLatin1Char('^'), QLatin1Char('i'), 0x00EE},
    {QLatin1Char('"'), QLatin1Char('i'), 0x00EF},
    {QLatin1Char('~'), QLatin1Char('i'), 0x0129},
    {QLatin1Char('='), QLatin1Char('i'), 0x012B},
    {QLatin1Char('u'), QLatin1Char('i'), 0x012D},
    {QLatin1Char('k'), QLatin1Char('i'), 0x012F},
    {QLatin1Char('^'), QLatin1Char('j'), 0x0135},
    {QLatin1Char('v'), QLatin1Char('i'), 0x01D0},
    {QLatin1Char('v'), QLatin1Char('j'), 0x01F0},
    {QLatin1Char('G'), QLatin1Char('i'), 0x0209}
};
static const int dotlessIJCharactersLen = sizeof(dotlessIJCharacters) / sizeof(dotlessIJCharacters[0]);


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
}
mathCommand[] = {
    {QStringLiteral("ell"), 0x2113},
    {QStringLiteral("rightarrow"), 0x2192},
    {QStringLiteral("forall"), 0x2200},
    {QStringLiteral("exists"), 0x2203},
    {QStringLiteral("in"), 0x2208},
    {QStringLiteral("ni"), 0x220B},
    {QStringLiteral("asterisk"), 0x2217},
    {QStringLiteral("infty"), 0x221E},
    {QStringLiteral("subset"), 0x2282},
    {QStringLiteral("supset"), 0x2283},
    {QStringLiteral("subseteq"), 0x2286},
    {QStringLiteral("supseteq"), 0x2287},
    {QStringLiteral("nsubseteq"), 0x2288},
    {QStringLiteral("nsupseteq"), 0x2289},
    {QStringLiteral("subsetneq"), 0x228A},
    {QStringLiteral("supsetneq"), 0x228A},
    {QStringLiteral("Subset"), 0x22D0},
    {QStringLiteral("Supset"), 0x22D1},
    {QStringLiteral("top"), 0x22A4}
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
    const QString command;
    const ushort unicode;
}
encoderLaTeXCharacterCommands[] = {
    {QStringLiteral("textexclamdown"), 0x00A1}, // TODO /// recommended to write  !`  instead of  \textexclamdown
    {QStringLiteral("textcent"), 0x00A2},
    {QStringLiteral("pounds"), 0x00A3},
    {QStringLiteral("textsterling"), 0x00A3},
    /** 0x00A4 */
    {QStringLiteral("textyen"), 0x00A5},
    {QStringLiteral("textbrokenbar"), 0x00A6},
    {QStringLiteral("S"), 0x00A7},
    {QStringLiteral("textsection"), 0x00A7},
    /** 0x00A8 */
    {QStringLiteral("copyright"), 0x00A9},
    {QStringLiteral("textcopyright"), 0x00A9},
    {QStringLiteral("textordfeminine"), 0x00AA},
    {QStringLiteral("guillemotleft"), 0x00AB},
    {QStringLiteral("textflqq"), 0x00AB},
    /** 0x00AC */
    /** 0x00AD */
    {QStringLiteral("textregistered"), 0x00AE},
    /** 0x00AF */
    {QStringLiteral("textdegree"), 0x00B0},
    {QStringLiteral("textpm"), 0x00B1},
    {QStringLiteral("textplusminus"), 0x00B1},
    /** 0x00B2 */
    /** 0x00B3 */
    /** 0x00B4 */
    {QStringLiteral("textmu"), 0x00B5},
    {QStringLiteral("textparagraph"), 0x00B6},
    {QStringLiteral("textpilcrow"), 0x00B6},
    {QStringLiteral("textperiodcentered"), 0x00B7},
    {QStringLiteral("textcdot"), 0x00B7},
    {QStringLiteral("textcentereddot"), 0x00B7},
    /** 0x00B8 */
    /** 0x00B9 */
    {QStringLiteral("textordmasculine"), 0x00BA},
    {QStringLiteral("guillemotright"), 0x00BB},
    {QStringLiteral("textfrqq"), 0x00BB},
    {QStringLiteral("textonequarter"), 0x00BC},
    {QStringLiteral("textonehalf"), 0x00BD},
    {QStringLiteral("textthreequarters"), 0x00BE},
    {QStringLiteral("textquestiondown"), 0x00BF}, // TODO /// recommended to write  ?`  instead of  \textquestiondown
    {QStringLiteral("AA"), 0x00C5},
    {QStringLiteral("AE"), 0x00C6},
    {QStringLiteral("DH"), 0x00D0},
    {QStringLiteral("texttimes"), 0x00D7},
    {QStringLiteral("textmultiply"), 0x00D7},
    {QStringLiteral("O"), 0x00D8},
    {QStringLiteral("TH"), 0x00DE},
    {QStringLiteral("Thorn"), 0x00DE},
    {QStringLiteral("textThorn"), 0x00DE},
    {QStringLiteral("ss"), 0x00DF},
    {QStringLiteral("aa"), 0x00E5},
    {QStringLiteral("ae"), 0x00E6},
    {QStringLiteral("dh"), 0x00F0},
    {QStringLiteral("textdiv"), 0x00F7},
    {QStringLiteral("textdivide"), 0x00F7},
    {QStringLiteral("o"), 0x00F8},
    {QStringLiteral("th"), 0x00FE},
    {QStringLiteral("textthorn"), 0x00FE},
    {QStringLiteral("textthornvari"), 0x00FE},
    {QStringLiteral("textthornvarii"), 0x00FE},
    {QStringLiteral("textthornvariii"), 0x00FE},
    {QStringLiteral("textthornvariv"), 0x00FE},
    {QStringLiteral("Aogonek"), 0x0104},
    {QStringLiteral("aogonek"), 0x0105},
    {QStringLiteral("DJ"), 0x0110},
    {QStringLiteral("dj"), 0x0111},
    {QStringLiteral("textcrd"), 0x0111},
    {QStringLiteral("textHslash"), 0x0126},
    {QStringLiteral("textHbar"), 0x0126},
    {QStringLiteral("textcrh"), 0x0127},
    {QStringLiteral("texthbar"), 0x0127},
    {QStringLiteral("i"), 0x0131},
    {QStringLiteral("IJ"), 0x0132},
    {QStringLiteral("ij"), 0x0133},
    {QStringLiteral("textkra"), 0x0138},
    {QStringLiteral("Lcaron"), 0x013D},
    {QStringLiteral("lcaron"), 0x013E},
    {QStringLiteral("L"), 0x0141},
    {QStringLiteral("Lstroke"), 0x0141},
    {QStringLiteral("l"), 0x0142},
    {QStringLiteral("lstroke"), 0x0142},
    {QStringLiteral("textbarl"), 0x0142},
    {QStringLiteral("NG"), 0x014A},
    {QStringLiteral("ng"), 0x014B},
    {QStringLiteral("OE"), 0x0152},
    {QStringLiteral("oe"), 0x0153},
    {QStringLiteral("Racute"), 0x0154},
    {QStringLiteral("racute"), 0x0155},
    {QStringLiteral("Sacute"), 0x015A},
    {QStringLiteral("sacute"), 0x015B},
    {QStringLiteral("Scedilla"), 0x015E},
    {QStringLiteral("scedilla"), 0x015F},
    {QStringLiteral("Scaron"), 0x0160},
    {QStringLiteral("scaron"), 0x0161},
    {QStringLiteral("Tcaron"), 0x0164},
    {QStringLiteral("tcaron"), 0x0165},
    {QStringLiteral("textTstroke"), 0x0166},
    {QStringLiteral("textTbar"), 0x0166},
    {QStringLiteral("textTslash"), 0x0166},
    {QStringLiteral("texttstroke"), 0x0167},
    {QStringLiteral("texttbar"), 0x0167},
    {QStringLiteral("texttslash"), 0x0167},
    {QStringLiteral("Zdotaccent"), 0x017B},
    {QStringLiteral("zdotaccent"), 0x017C},
    {QStringLiteral("Zcaron"), 0x017D},
    {QStringLiteral("zcaron"), 0x017E},
    {QStringLiteral("textlongs"), 0x017F},
    {QStringLiteral("textcrb"), 0x0180},
    {QStringLiteral("textBhook"), 0x0181},
    {QStringLiteral("texthausaB"), 0x0181},
    {QStringLiteral("textOopen"), 0x0186},
    {QStringLiteral("textChook"), 0x0187},
    {QStringLiteral("textchook"), 0x0188},
    {QStringLiteral("texthtc"), 0x0188},
    {QStringLiteral("textDafrican"), 0x0189},
    {QStringLiteral("textDhook"), 0x018A},
    {QStringLiteral("texthausaD"), 0x018A},
    {QStringLiteral("textEreversed"), 0x018E},
    {QStringLiteral("textrevE"), 0x018E},
    {QStringLiteral("textEopen"), 0x0190},
    {QStringLiteral("textFhook"), 0x0191},
    {QStringLiteral("textflorin"), 0x0192},
    {QStringLiteral("textgamma"), 0x0194},
    {QStringLiteral("textGammaafrican"), 0x0194},
    {QStringLiteral("hv"), 0x0195},
    {QStringLiteral("texthvlig"), 0x0195},
    {QStringLiteral("textIotaafrican"), 0x0196},
    {QStringLiteral("textKhook"), 0x0198},
    {QStringLiteral("texthausaK"), 0x0198},
    {QStringLiteral("texthtk"), 0x0199},
    {QStringLiteral("textkhook"), 0x0199},
    {QStringLiteral("textbarl"), 0x019A},
    {QStringLiteral("textcrlambda"), 0x019B},
    {QStringLiteral("textNhookleft"), 0x019D},
    {QStringLiteral("textnrleg"), 0x019E},
    {QStringLiteral("textPUnrleg"), 0x019E},
    {QStringLiteral("Ohorn"), 0x01A0},
    {QStringLiteral("ohorn"), 0x01A1},
    {QStringLiteral("textPhook"), 0x01A4},
    {QStringLiteral("texthtp"), 0x01A5},
    {QStringLiteral("textphook"), 0x01A5},
    {QStringLiteral("ESH"), 0x01A9},
    {QStringLiteral("textEsh"), 0x01A9},
    {QStringLiteral("textlooptoprevsh"), 0x01AA},
    {QStringLiteral("textlhtlongi"), 0x01AA},
    {QStringLiteral("textlhookt"), 0x01AB},
    {QStringLiteral("textThook"), 0x01AC},
    {QStringLiteral("textthook"), 0x01AD},
    {QStringLiteral("texthtt"), 0x01AD},
    {QStringLiteral("textTretroflexhook"), 0x01AE},
    {QStringLiteral("Uhorn"), 0x01AF},
    {QStringLiteral("uhorn"), 0x01B0},
    {QStringLiteral("textupsilon"), 0x01B1},
    {QStringLiteral("textVhook"), 0x01B2},
    {QStringLiteral("textYhook"), 0x01B3},
    {QStringLiteral("textvhook"), 0x01B4},
    {QStringLiteral("Zbar"), 0x01B5},
    {QStringLiteral("zbar"), 0x01B6},
    {QStringLiteral("EZH"), 0x01B7},
    {QStringLiteral("textEzh"), 0x01B7},
    {QStringLiteral("LJ"), 0x01C7},
    {QStringLiteral("Lj"), 0x01C8},
    {QStringLiteral("lj"), 0x01C9},
    {QStringLiteral("NJ"), 0x01CA},
    {QStringLiteral("Nj"), 0x01CB},
    {QStringLiteral("nj"), 0x01CC},
    {QStringLiteral("DZ"), 0x01F1},
    {QStringLiteral("Dz"), 0x01F2},
    {QStringLiteral("dz"), 0x01F3},
    {QStringLiteral("HV"), 0x01F6},
    {QStringLiteral("j"), 0x0237},
    {QStringLiteral("ldots"), 0x2026}, /** \ldots must be before \l */ // TODO comment still true?
    {QStringLiteral("grqq"), 0x201C},
    {QStringLiteral("rqq"), 0x201D},
    {QStringLiteral("glqq"), 0x201E},
    {QStringLiteral("frqq"), 0x00BB},
    {QStringLiteral("flqq"), 0x00AB},
    {QStringLiteral("rq"), 0x2019}, ///< tricky one: 'r' is a valid modifier
    {QStringLiteral("lq"), 0x2018}
};
static const int encoderLaTeXCharacterCommandsLen = sizeof(encoderLaTeXCharacterCommands) / sizeof(encoderLaTeXCharacterCommands[0]);

const QChar EncoderLaTeX::encoderLaTeXProtectedSymbols[] = {QLatin1Char('#'), QLatin1Char('&'), QLatin1Char('%')};
const int EncoderLaTeX::encoderLaTeXProtectedSymbolsLen = sizeof(EncoderLaTeX::encoderLaTeXProtectedSymbols) / sizeof(EncoderLaTeX::encoderLaTeXProtectedSymbols[0]);

const QChar EncoderLaTeX::encoderLaTeXProtectedTextOnlySymbols[] = {QLatin1Char('_')};
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
    const QString latex;
    const ushort unicode;
} encoderLaTeXSymbolSequences[] = {
    {QStringLiteral("!`"), 0x00A1},
    {QStringLiteral("\"<"), 0x00AB},
    {QStringLiteral("\">"), 0x00BB},
    {QStringLiteral("?`"), 0x00BF},
    {QStringLiteral("---"), 0x2014},
    {QStringLiteral("--"), 0x2013},
    {QStringLiteral("``"), 0x201C},
    {QStringLiteral("''"), 0x201D},
    {QStringLiteral("ff"), 0xFB00},
    {QStringLiteral("fi"), 0xFB01},
    {QStringLiteral("fl"), 0xFB02},
    {QStringLiteral("ffi"), 0xFB03},
    {QStringLiteral("ffl"), 0xFB04},
    {QStringLiteral("ft"), 0xFB05},
    {QStringLiteral("st"), 0xFB06}
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
            /// If no special character is known for a letter+modifier
            /// combination, fall back using the ASCII character only
            for (ushort k = 0; k < 26; ++k) {
                lookupTable[lookupTableCount]->unicode[k] = QChar(QLatin1Char('A').unicode() + k);
                lookupTable[lookupTableCount]->unicode[k + 26] = QChar(QLatin1Char('a').unicode() + k);
            }
            for (ushort k = 0; k < 10; ++k)
                lookupTable[lookupTableCount]->unicode[k + 52] = QChar(QLatin1Char('0').unicode() + k);
            j = lookupTableCount;
            ++lookupTableCount;
        }

        /// Add the letter as of the current row in encoderLaTeXEscapedCharacters
        /// into Unicode char array in the current modifier's row in the lookup table.
        int pos = -1;
        if ((pos = asciiLetterOrDigitToPos(encoderLaTeXEscapedCharacters[i].letter)) >= 0)
            lookupTable[j]->unicode[pos] = QChar(encoderLaTeXEscapedCharacters[i].unicode);
        else
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

        if (c == QLatin1Char('{')) {
            /// First case: An opening curly bracket,
            /// which is harmless (see else case), unless ...
            if (i < len - 3 && input[i + 1] == QLatin1Char('\\')) {
                /// ... it continues with a backslash

                /// Next, check if there follows a modifier after the backslash
                /// For example an quotation mark as used in {\"a}
                int lookupTablePos = modifierInLookupTable(input[i + 2].toLatin1());

                /// Check for spaces between modifier and character, for example
                /// like {\H o}
                int skipSpaces = 0;
                while (i + 3 + skipSpaces < len && input[i + 3 + skipSpaces] == QLatin1Char(' ') && skipSpaces < 16) ++skipSpaces;

                if (lookupTablePos >= 0 && i + skipSpaces < len - 4 && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 3 + skipSpaces])) >= 0 && input[i + 4 + skipSpaces] == QLatin1Char('}')) {
                    /// If we found a modifier which is followed by
                    /// a letter followed by a closing curly bracket,
                    /// we are looking at something like {\"A}
                    /// Use lookup table to see what Unicode char this
                    /// represents
                    const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                    if (unicodeLetter.unicode() < 127) {
                        /// This combination of modifier and letter is not known,
                        /// so try to preserve it
                        output.append(input.midRef(i, 5 + skipSpaces));
                        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 5 + skipSpaces);
                    } else
                        output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 4 + skipSpaces;
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 5 && input[i + 3 + skipSpaces] == QLatin1Char('\\') && isIJ(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == QLatin1Char('}')) {
                    /// This is the case for {\'\i} or alike.
                    bool found = false;
                    for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                        if (dotlessIJCharacters[k].letter == input[i + 4 + skipSpaces] && dotlessIJCharacters[k].modifier == input[i + 2]) {
                            output.append(QChar(dotlessIJCharacters[k].unicode));
                            i += 5 + skipSpaces;
                            found = true;
                        }
                    if (!found)
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 2] << "BACKSLASH" << input[i + 4 + skipSpaces];
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 6 && input[i + 3 + skipSpaces] == QLatin1Char('{') && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 4 + skipSpaces])) >= 0 && input[i + 5 + skipSpaces] == QLatin1Char('}') && input[i + 6 + skipSpaces] == QLatin1Char('}')) {
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
                        output.append(input.midRef(i, 7 + skipSpaces));
                        qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 7 + skipSpaces);
                    } else
                        output.append(unicodeLetter);
                    /// Step over those additional characters
                    i += 6 + skipSpaces;
                } else if (lookupTablePos >= 0 && i + skipSpaces < len - 7 && input[i + 3 + skipSpaces] == QLatin1Char('{') && input[i + 4 + skipSpaces] == QLatin1Char('\\') && isIJ(input[i + 5 + skipSpaces]) && input[i + 6 + skipSpaces] == QLatin1Char('}') && input[i + 7 + skipSpaces] == QLatin1Char('}')) {
                    /// This is the case for {\'{\i}} or alike.
                    bool found = false;
                    for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                        if (dotlessIJCharacters[k].letter == input[i + 5 + skipSpaces] && dotlessIJCharacters[k].modifier == input[i + 2]) {
                            output.append(QChar(dotlessIJCharacters[k].unicode));
                            i += 7 + skipSpaces;
                            found = true;
                        }
                    if (!found)
                        qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 2] << "BACKSLASH {" << input[i + 5 + skipSpaces] << "}";
                } else {
                    /// Now, the case of something like {\AA} is left
                    /// to check for
                    const QString alpha = readAlphaCharacters(input, i + 2);
                    int nextPosAfterAlpha = i + 2 + alpha.size();
                    if (nextPosAfterAlpha < input.length() && input[nextPosAfterAlpha] == QLatin1Char('}')) {
                        /// We are dealing actually with a string like {\AA}
                        /// Check which command it is,
                        /// insert corresponding Unicode character
                        bool foundCommand = false;
                        for (int ci = 0; !foundCommand && ci < encoderLaTeXCharacterCommandsLen; ++ci) {
                            if (encoderLaTeXCharacterCommands[ci].command == alpha) {
                                output.append(QChar(encoderLaTeXCharacterCommands[ci].unicode));
                                foundCommand = true;
                            }
                        }

                        /// Check if a math command has been read,
                        /// like \subset
                        /// (automatically skipped if command was found above)
                        for (int k = 0; !foundCommand && k < mathCommandLen; ++k) {
                            if (mathCommand[k].command == alpha) {
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
        } else if (c == QLatin1Char('\\') && i < len - 1) {
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
            while (i + 2 + skipSpaces < len && input[i + 2 + skipSpaces] == QLatin1Char(' ') && skipSpaces < 16) ++skipSpaces;

            if (lookupTablePos >= 0 && i + skipSpaces <= len - 3 && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 2 + skipSpaces])) >= 0 && (i + skipSpaces == len - 3 || input[i + 1] == QLatin1Char('"') || input[i + 1] == QLatin1Char('\'') || input[i + 1] == QLatin1Char('`') || input[i + 1] == QLatin1Char('='))) { // TODO more special cases?
                /// We found a special modifier which is followed by
                /// a letter followed by normal text without any
                /// delimiter, so we are looking at something like
                /// \"u inside Kr\"uger
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
                if (unicodeLetter.unicode() < 127) {
                    /// This combination of modifier and letter is not known,
                    /// so try to preserve it
                    output.append(input.midRef(i, 3 + skipSpaces));
                    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 3 + skipSpaces);
                } else
                    output.append(unicodeLetter);
                /// Step over those additional characters
                i += 2 + skipSpaces;
            } else if (lookupTablePos >= 0 && i + skipSpaces <= len - 3 && i + skipSpaces <= len - 3 && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 2 + skipSpaces])) >= 0 && (i + skipSpaces == len - 3 || input[i + 3 + skipSpaces] == QLatin1Char('}') || input[i + 3 + skipSpaces] == QLatin1Char('{') || input[i + 3 + skipSpaces] == QLatin1Char(' ') || input[i + 3 + skipSpaces] == QLatin1Char('\t') || input[i + 3 + skipSpaces] == QLatin1Char('\\') || input[i + 3 + skipSpaces] == QLatin1Char('\r') || input[i + 3 + skipSpaces] == QLatin1Char('\n'))) {
                /// We found a modifier which is followed by
                /// a letter followed by a command delimiter such
                /// as a whitespace, so we are looking at something
                /// like \"u followed by a space
                /// Use lookup table to see what Unicode char this
                /// represents
                const QChar unicodeLetter = lookupTable[lookupTablePos]->unicode[cachedAsciiLetterOrDigitToPos];
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
                if (input[i + 1] == QLatin1Char(' ') || input[i + 1] == QLatin1Char('\r') || input[i + 1] == QLatin1Char('\n'))
                    ++i;
                else {
                    /// If no whitespace follows, still
                    /// check for extra curly brackets
                    checkForExtraCurlyAtEnd = true;
                }
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 4 && input[i + 2 + skipSpaces] == QLatin1Char('{') && (cachedAsciiLetterOrDigitToPos = asciiLetterOrDigitToPos(input[i + 3 + skipSpaces])) >= 0 && input[i + 4 + skipSpaces] == QLatin1Char('}')) {
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
                    output.append(input.midRef(i, 5 + skipSpaces));
                    qCWarning(LOG_KBIBTEX_IO) << "Don't know how to translate this into Unicode: " << input.mid(i, 5 + skipSpaces);
                } else
                    output.append(unicodeLetter);
                /// Step over those additional characters
                i += 4 + skipSpaces;
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 3 && input[i + 2 + skipSpaces] == QLatin1Char('\\') && isIJ(input[i + 3 + skipSpaces])) {
                /// This is the case for \'\i or alike.
                bool found = false;
                for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                    if (dotlessIJCharacters[k].letter == input[i + 3 + skipSpaces] && dotlessIJCharacters[k].modifier == input[i + 1]) {
                        output.append(QChar(dotlessIJCharacters[k].unicode));
                        i += 3 + skipSpaces;
                        found = true;
                    }
                if (!found)
                    qCWarning(LOG_KBIBTEX_IO) << "Cannot interprete BACKSLASH" << input[i + 1] << "BACKSLASH" << input[i + 3 + skipSpaces];
            } else if (lookupTablePos >= 0 && i + skipSpaces < len - 5 && input[i + 2 + skipSpaces] == QLatin1Char('{') && input[i + 3 + skipSpaces] == QLatin1Char('\\') && isIJ(input[i + 4 + skipSpaces]) && input[i + 5 + skipSpaces] == QLatin1Char('}')) {
                /// This is the case for \'{\i} or alike.
                bool found = false;
                for (int k = 0; !found && k < dotlessIJCharactersLen; ++k)
                    if (dotlessIJCharacters[k].letter == input[i + 4 + skipSpaces] && dotlessIJCharacters[k].modifier == input[i + 1]) {
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
                        if (encoderLaTeXCharacterCommands[ci].command == alpha) {
                            output.append(QChar(encoderLaTeXCharacterCommands[ci].unicode));
                            foundCommand = true;
                        }
                    }

                    if (foundCommand) {
                        /// Now, after a command, a whitespace may follow
                        /// which has to get "eaten" as it acts as a command
                        /// delimiter
                        if (nextPosAfterAlpha < input.length() && (input[nextPosAfterAlpha] == QLatin1Char(' ') || input[nextPosAfterAlpha] == QLatin1Char('\r') || input[nextPosAfterAlpha] == QLatin1Char('\n')))
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
            if (checkForExtraCurlyAtEnd && i < len - 2 && input[i + 1] == QLatin1Char('{') && input[i + 2] == QLatin1Char('}')) i += 2;
        } else {
            /// So far, no opening curly bracket and no backslash
            /// May still be a symbol sequence like ---
            bool isSymbolSequence = false;
            /// Go through all known symbol sequnces
            for (int l = 0; l < encoderLaTeXSymbolSequencesLen; ++l) {
                /// First, check if read input character matches beginning of symbol sequence
                /// and input buffer as enough characters left to potentially contain
                /// symbol sequence
                const int latexLen = encoderLaTeXSymbolSequences[l].latex.length();
                if (encoderLaTeXSymbolSequences[l].latex[0] == c && i <= len - latexLen) {
                    /// Now actually check if symbol sequence is in input buffer
                    isSymbolSequence = true;
                    for (int p = 1; isSymbolSequence && p < latexLen; ++p)
                        isSymbolSequence &= encoderLaTeXSymbolSequences[l].latex[p] == input[i + p];
                    if (isSymbolSequence) {
                        /// Ok, found sequence: insert Unicode character in output
                        /// and hop over sequence in input buffer
                        output.append(QChar(encoderLaTeXSymbolSequences[l].unicode));
                        i += encoderLaTeXSymbolSequences[l].latex.length() - 1;
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
                if (c == QLatin1Char('$') && (i == 0 || input[i - 1] != QLatin1Char('\\')))
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
    if (pos < input.length() - 6 && input.mid(pos, 5) == QStringLiteral("\\url{")) {
        copyBytesCount = 5;
        openedClosedCurlyBrackets = 1;
    }

    if (copyBytesCount > 0) {
        while (openedClosedCurlyBrackets > 0 && pos + copyBytesCount < input.length()) {
            ++copyBytesCount;
            if (input[pos + copyBytesCount] == QLatin1Char('{') && input[pos + copyBytesCount - 1] != QLatin1Char('\\')) ++openedClosedCurlyBrackets;
            else if (input[pos + copyBytesCount] == QLatin1Char('}') && input[pos + copyBytesCount - 1] != QLatin1Char('\\')) --openedClosedCurlyBrackets;
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
                        for (int l = 0; l < encoderLaTeXSymbolSequences[k].latex.length(); ++l)
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
                        output.append(QString(QStringLiteral("{\\%1}")).arg(encoderLaTeXCharacterCommands[k].command));
                        found = true;
                        break;
                    }
            }

            if (!found) {
                /// Ok, neither a character command. Let's test
                /// escaped characters with modifiers like \"a
                for (int k = 0; k < encoderLaTeXEscapedCharactersLen; ++k)
                    if (encoderLaTeXEscapedCharacters[k].unicode == c.unicode()) {
                        const QChar modifier = encoderLaTeXEscapedCharacters[k].modifier;
                        const QString formatString = isAsciiLetter(modifier) ? QStringLiteral("{\\%1 %2}") : QStringLiteral("{\\%1%2}");
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
                            output.append(QString(QStringLiteral("\\%1{}")).arg(mathCommand[k].command));
                        else
                            output.append(QString(QStringLiteral("\\ensuremath{\\%1}")).arg(mathCommand[k].command));
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
            if (c == QLatin1Char('$') && (i == 0 || input[i - 1] != QLatin1Char('\\')))
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
