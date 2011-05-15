/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#include <QRegExp>
#include <QStringList>
#include <QList>

#include <KDebug>

#include "encoderlatex.h"

EncoderLaTeX *encoderLaTeX = NULL;

static struct Decomposition {
    const char *latexCommand;
    unsigned int unicode;
}

decompositions[] = {
    {"`", 0x0300},
    {"'", 0x0301},
    {"^", 0x0302},
    {"~", 0x0303},
    {"=", 0x0304},
    /*{"x", 0x0305},  OVERLINE */
    {"u", 0x0306},
    {".", 0x0307},
    /*{"x", 0x0309},  HOOK ABOVE */
    {"r", 0x030a},
    {"H", 0x030b},
    {"v", 0x030c},
    /*{"x", 0x030d},  VERTICAL LINE ABOVE */
    /*{"x", 0x030e},  DOUBLE VERTICAL LINE ABOVE */
    /*{"x", 0x030f},  DOUBLE GRAVE ACCENT */
    /*{"x", 0x0310},  CANDRABINDU */
    /*{"x", 0x0311},  INVERTED BREVE */
    /*{"x", 0x0312},  TURNED COMMA ABOVE */
    /*{"x", 0x0313},  COMMA ABOVE */
    /*{"x", 0x0314},  REVERSED COMMA ABOVE */
    /*{"x", 0x0315},   */
    /*{"x", 0x0316},   */
    /*{"x", 0x0317},   */
    /*{"x", 0x0318},   */
    /*{"x", 0x0319},   */
    /*{"x", 0x031a},   */
    /*{"x", 0x031b},   */
    /*{"x", 0x031c},   */
    /*{"x", 0x031d},   */
    /*{"x", 0x031e},   */
    /*{"x", 0x031f},   */
    /*{"x", 0x0320},   */
    /*{"x", 0x0321},   */
    /*{"x", 0x0322},   */
    {"d", 0x0323},
    /*{"x", 0x0324},   */
    /*{"x", 0x0325},   */
    /*{"x", 0x0326},   */
    {"d", 0x0327},
    {"k", 0x0328},
    /*{"x", 0x0329},   */
    /*{"x", 0x032a},   */
    /*{"x", 0x032b},   */
    /*{"x", 0x032c},   */
    /*{"x", 0x032d},   */
    /*{"x", 0x032e},   */
    /*{"x", 0x032f},   */
    {"b", 0x0331},
    {"t", 0x0361}
};

static const int decompositionscount = sizeof(decompositions) / sizeof(decompositions[ 0 ]) ;

static const struct EncoderLaTeXCommandMapping {
    const char *letters;
    unsigned int unicode;
}
commandmappingdatalatex[] = {
    {"AA", 0x00C5},
    {"AE", 0x00C6},
    {"ss", 0x00DF},
    {"aa", 0x00E5},
    {"ae", 0x00E6},
    {"OE", 0x0152},
    {"oe", 0x0153},
    {"ldots", 0x2026}, /** \ldots must be before \l */
    {"L", 0x0141},
    {"l", 0x0142},
    {"grqq", 0x201C},
    {"rqq", 0x201D},
    {"glqq", 0x201E},
    {"frqq", 0x00BB},
    {"flqq", 0x00AB},
    {"rq", 0x2019},
    {"lq", 0x2018}
};

static const int commandmappingdatalatexcount = sizeof(commandmappingdatalatex) / sizeof(commandmappingdatalatex[ 0 ]) ;

/** Command can be either
    (1) {embraced}
    (2) delimited by {},
    (3) <space>, line end,
    (4) \following_command (including \<space>, which must be maintained!),
    (5) } (end of entry or group)
 **/
const char *expansionsCmd[] = {"\\{\\\\%1\\}", "\\\\%1\\{\\}", "\\\\%1(\\n|\\r|\\\\|\\})", "\\\\%1\\s"};
static const  int expansionscmdcount = sizeof(expansionsCmd) / sizeof(expansionsCmd[0]);

static const struct EncoderLaTeXModCharMapping {
    const char *modifier;
    const char *letter;
    unsigned int unicode;
}
modcharmappingdatalatex[] = {
    {"\\\\`", "A", 0x00C0},
    {"\\\\'", "A", 0x00C1},
    {"\\\\\\^", "A", 0x00C2},
    {"\\\\~", "A", 0x00C3},
    {"\\\\\"", "A", 0x00C4},
    {"\\\\r", "A", 0x00C5},
    /** 0x00C6 */
    {"\\\\c", "C", 0x00C7},
    {"\\\\`", "E", 0x00C8},
    {"\\\\'", "E", 0x00C9},
    {"\\\\\\^", "E", 0x00CA},
    {"\\\\\"", "E", 0x00CB},
    {"\\\\`", "I", 0x00CC},
    {"\\\\'", "I", 0x00CD},
    {"\\\\\\^", "I", 0x00CE},
    {"\\\\\"", "I", 0x00CF},
    /** 0x00D0 */
    {"\\\\~", "N", 0x00D1},
    {"\\\\`", "O", 0x00D2},
    {"\\\\'", "O", 0x00D3},
    {"\\\\\\^", "O", 0x00D4},
    /** 0x00D5 */
    {"\\\\\"", "O", 0x00D6},
    /** 0x00D7 */
    {"\\\\", "O", 0x00D8},
    {"\\\\`", "U", 0x00D9},
    {"\\\\'", "U", 0x00DA},
    {"\\\\\\^", "U", 0x00DB},
    {"\\\\\"", "U", 0x00DC},
    {"\\\\'", "Y", 0x00DD},
    /** 0x00DE */
    {"\\\\\"", "s", 0x00DF},
    {"\\\\`", "a", 0x00E0},
    {"\\\\'", "a", 0x00E1},
    {"\\\\\\^", "a", 0x00E2},
    {"\\\\~", "a", 0x00E3},
    {"\\\\\"", "a", 0x00E4},
    {"\\\\r", "a", 0x00E5},
    /** 0x00E6 */
    {"\\\\c", "c", 0x00E7},
    {"\\\\`", "e", 0x00E8},
    {"\\\\'", "e", 0x00E9},
    {"\\\\\\^", "e", 0x00EA},
    {"\\\\\"", "e", 0x00EB},
    {"\\\\`", "i", 0x00EC},
    {"\\\\'", "i", 0x00ED},
    {"\\\\'", "\\\\i", 0x00ED},
    {"\\\\\\^", "i", 0x00EE},
    /** 0x00EF */
    /** 0x00F0 */
    {"\\\\~", "n", 0x00F1},
    {"\\\\`", "o", 0x00F2},
    {"\\\\'", "o", 0x00F3},
    {"\\\\\\^", "o", 0x00F4},
    /** 0x00F5 */
    {"\\\\\"", "o", 0x00F6},
    /** 0x00F7 */
    {"\\\\", "o", 0x00F8},
    {"\\\\`", "u", 0x00F9},
    {"\\\\'", "u", 0x00FA},
    {"\\\\\\^", "u", 0x00FB},
    {"\\\\\"", "u", 0x00FC},
    {"\\\\'", "y", 0x00FD},
    /** 0x00FE */
    /** 0x00FF */
    /** 0x0100 */
    /** 0x0101 */
    {"\\\\u", "A", 0x0102},
    {"\\\\u", "a", 0x0103},
    /** 0x0104 */
    /** 0x0105 */
    {"\\\\'", "C", 0x0106},
    {"\\\\'", "c", 0x0107},
    /** 0x0108 */
    /** 0x0109 */
    /** 0x010A */
    /** 0x010B */
    {"\\\\v", "C", 0x010C},
    {"\\\\v", "c", 0x010D},
    {"\\\\v", "D", 0x010E},
    /** 0x010F */
    /** 0x0110 */
    /** 0x0111 */
    /** 0x0112 */
    /** 0x0113 */
    /** 0x0114 */
    /** 0x0115 */
    /** 0x0116 */
    /** 0x0117 */
    {"\\\\c", "E", 0x0118},
    {"\\\\c", "e", 0x0119},
    {"\\\\v", "E", 0x011A},
    {"\\\\v", "e", 0x011B},
    /** 0x011C */
    /** 0x011D */
    {"\\\\u", "G", 0x011E},
    {"\\\\u", "g", 0x011F},
    /** 0x0120 */
    /** 0x0121 */
    /** 0x0122 */
    /** 0x0123 */
    /** 0x0124 */
    /** 0x0125 */
    /** 0x0126 */
    /** 0x0127 */
    /** 0x0128 */
    /** 0x0129 */
    /** 0x012A */
    /** 0x012B */
    {"\\\\u", "I", 0x012C},
    {"\\\\u", "i", 0x012D},
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
    {"\\\\'", "L", 0x0139},
    {"\\\\'", "l", 0x013A},
    /** 0x013B */
    /** 0x013C */
    /** 0x013D */
    /** 0x013E */
    /** 0x013F */
    /** 0x0140 */
    /** 0x0141 */
    /** 0x0142 */
    {"\\\\'", "N", 0x0143},
    {"\\\\'", "n", 0x0144},
    /** 0x0145 */
    /** 0x0146 */
    {"\\\\v", "N", 0x0147},
    {"\\\\v", "n", 0x0148},
    /** 0x0149 */
    /** 0x014A */
    /** 0x014B */
    /** 0x014C */
    /** 0x014D */
    {"\\\\u", "O", 0x014E},
    {"\\\\u", "o", 0x014F},
    {"\\\\H", "O", 0x0150},
    {"\\\\H", "o", 0x0151},
    /** 0x0152 */
    /** 0x0153 */
    {"\\\\'", "R", 0x0154},
    {"\\\\'", "r", 0x0155},
    /** 0x0156 */
    /** 0x0157 */
    {"\\\\v", "R", 0x0158},
    {"\\\\v", "r", 0x0159},
    {"\\\\'", "S", 0x015A},
    {"\\\\'", "s", 0x015B},
    /** 0x015C */
    /** 0x015D */
    {"\\\\c", "S", 0x015E},
    {"\\\\c", "s", 0x015F},
    {"\\\\v", "S", 0x0160},
    {"\\\\v", "s", 0x0161},
    /** 0x0162 */
    /** 0x0163 */
    {"\\\\v", "T", 0x0164},
    /** 0x0165 */
    /** 0x0166 */
    /** 0x0167 */
    /** 0x0168 */
    /** 0x0169 */
    /** 0x016A */
    /** 0x016B */
    {"\\\\u", "U", 0x016C},
    {"\\\\u", "u", 0x016D},
    {"\\\\r", "U", 0x016E},
    {"\\\\r", "u", 0x016F},
    /** 0x0170 */
    /** 0x0171 */
    /** 0x0172 */
    /** 0x0173 */
    /** 0x0174 */
    /** 0x0175 */
    /** 0x0176 */
    /** 0x0177 */
    {"\\\\\"", "Y", 0x0178},
    {"\\\\'", "Z", 0x0179},
    {"\\\\'", "z", 0x017A},
    /** 0x017B */
    /** 0x017C */
    {"\\\\v", "Z", 0x017D},
    {"\\\\v", "z", 0x017E},
    /** 0x017F */
    /** 0x0180 */
    {"\\\\v", "A", 0x01CD},
    {"\\\\v", "a", 0x01CE},
    {"\\\\v", "G", 0x01E6},
    {"\\\\v", "g", 0x01E7}
};

const char *expansionsMod1[] = {"\\{%1\\{%2\\}\\}", "\\{%1 %2\\}", "%1\\{%2\\}"};
static const  int expansionsmod1count = sizeof(expansionsMod1) / sizeof(expansionsMod1[0]);
const char *expansionsMod2[] = {"\\{%1%2\\}", "%1%2\\{\\}", "%1%2"};
static const  int expansionsmod2count = sizeof(expansionsMod2) / sizeof(expansionsMod2[0]);

static const int modcharmappingdatalatexcount = sizeof(modcharmappingdatalatex) / sizeof(modcharmappingdatalatex[ 0 ]) ;

static const struct EncoderLaTeXCharMapping {
    const char *regexp;
    unsigned int unicode;
    const char *latex;
}
charmappingdatalatex[] = {
    {"\\\\#", 0x0023, "\\#"},
    {"\\\\&", 0x0026, "\\&"},
    {"\\\\_", 0x005F, "\\_"},
    {"!`", 0x00A1, "!`"},
    {"\"<", 0x00AB, "\"<"},
    {"\">", 0x00BB, "\">"},
    {"[?]`", 0x00BF, "?`"},
    {"---", 0x2014, "---"}, ///< has to be befor 0x2013, otherwise it would be interpreted as --{}-
    {"--", 0x2013, "--"},
    {"``", 0x201C, "``"},
    {"''", 0x201D, "''"}
};

static const int charmappingdatalatexcount = sizeof(charmappingdatalatex) / sizeof(charmappingdatalatex[ 0 ]) ;

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class EncoderLaTeX::EncoderLaTeXPrivate
{
public:
    struct CombinedMappingItem {
        QRegExp regExp;
        QString latex;
    };

    struct CharMappingItem {
        QRegExp regExp;
        QString unicode;
        QString latex;
    };

    QList<CombinedMappingItem> combinedMapping;
    QList<CharMappingItem> charMapping;

    void buildCombinedMapping() {
        for (int i = 0; i < decompositionscount; i++) {
            CombinedMappingItem item;
            item.regExp = QRegExp("(.)" + QString(QChar(decompositions[i].unicode)));
            item.latex = decompositions[i].latexCommand;
            combinedMapping.append(item);
        }
    }

    void buildCharMapping() {
        /** encoding and decoding for digraphs such as -- or ?` */
        for (int i = 0; i < charmappingdatalatexcount; i++) {
            CharMappingItem charMappingItem;
            charMappingItem.regExp = QRegExp(charmappingdatalatex[ i ].regexp);
            charMappingItem.unicode = QChar(charmappingdatalatex[ i ].unicode);
            charMappingItem.latex = QString(charmappingdatalatex[ i ].latex);
            charMapping.append(charMappingItem);
        }

        /** encoding and decoding for commands such as \AA or \ss */
        for (int i = 0; i < commandmappingdatalatexcount; ++i) {
            /** different types of writing such as {\AA} or \AA{} possible */
            for (int j = 0; j < expansionscmdcount; ++j) {
                CharMappingItem charMappingItem;
                charMappingItem.regExp = QRegExp(QString(expansionsCmd[j]).arg(commandmappingdatalatex[i].letters));
                charMappingItem.unicode = QChar(commandmappingdatalatex[i].unicode);
                if (charMappingItem.regExp.numCaptures() > 0)
                    charMappingItem.unicode += QString("\\1");
                charMappingItem.latex = QString("{\\%1}").arg(commandmappingdatalatex[i].letters);
                charMapping.append(charMappingItem);
            }
        }

        /** encoding and decoding for letters such as \"a */
        for (int i = 0; i < modcharmappingdatalatexcount; ++i) {
            QString modifierRegExp = QString(modcharmappingdatalatex[i].modifier);
            QString modifier = modifierRegExp;
            modifier.replace("\\^", "^").replace("\\\\", "\\");

            /** first batch of replacement rules, where no separator is required between modifier and character (e.g. \"a) */
            if (!modifierRegExp.at(modifierRegExp.length() - 1).isLetter())
                for (int j = 0; j < expansionsmod2count; ++j) {
                    CharMappingItem charMappingItem;
                    charMappingItem.regExp = QRegExp(QString(expansionsMod2[j]).arg(modifierRegExp).arg(modcharmappingdatalatex[i].letter));
                    charMappingItem.unicode = QChar(modcharmappingdatalatex[i].unicode);
                    charMappingItem.latex = QString("{%1%2}").arg(modifier).arg(modcharmappingdatalatex[i].letter);
                    charMapping.append(charMappingItem);
                }

            /** second batch of replacement rules, where a separator is required between modifier and character (e.g. \v{g}) */
            for (int j = 0; j < expansionsmod1count; ++j) {
                CharMappingItem charMappingItem;
                charMappingItem.regExp = QRegExp(QString(expansionsMod1[j]).arg(modifierRegExp).arg(modcharmappingdatalatex[i].letter));
                charMappingItem.unicode = QChar(modcharmappingdatalatex[i].unicode);
                charMappingItem.latex = QString("%1{%2}").arg(modifier).arg(modcharmappingdatalatex[i].letter);
                charMapping.append(charMappingItem);
            }
        }
    }
};

EncoderLaTeX::EncoderLaTeX()
        : Encoder(), d(new EncoderLaTeX::EncoderLaTeXPrivate)
{
    d->buildCharMapping();
    d->buildCombinedMapping();
}

EncoderLaTeX::~EncoderLaTeX()
{
    // nothing
}

QString EncoderLaTeX::decode(const QString & text)
{
    const QString splitMarker = "|KBIBTEX|";

    /** start-stop marker ensures that each text starts and stops
      * with plain text and not with an inline math environment.
      * This invariant is exploited implicitly in the code below. */
    const QString startStopMarker = "|STARTSTOP|";
    QString result = startStopMarker + text + startStopMarker;

    /** Collect (all?) urls from the BibTeX file and store them in urls */
    /** Problem is that the replace function below will replace
      * character sequences in the URL rendering the URL invalid.
      * Later, all URLs will be replaced back to their original
      * in the hope nothing breaks ... */
    QStringList urls;
    QRegExp httpRegExp("(ht|f)tps?://[^\"} ]+");
    httpRegExp.setMinimal(false);
    int pos = 0;
    while (pos >= 0) {
        pos = result.indexOf(httpRegExp, pos);
        if (pos >= 0) {
            ++pos;
            QString url = httpRegExp.cap(0);
            urls << url;
        }
    }

    decomposedUTF8toLaTeX(result);

    /** split text into math and non-math regions */
    QStringList intermediate = result.split('$', QString::SkipEmptyParts);
    QStringList::Iterator it = intermediate.begin();
    while (it != intermediate.end()) {
        /**
         * Sometimes we split strings like "\$", which is not intended.
         * So, we have to manually fix things by checking for strings
         * ending with "\" and append both the removed dollar sign and
         * the following string (which was never supposed to be an
         * independent string). Finally, we remove the unnecessary
         * string and continue.
         */
        if ((*it).endsWith("\\")) {
            QStringList::Iterator cur = it;
            ++it;
            (*cur).append('$').append(*it);
            it = intermediate.erase(it);
        } else
            ++it;
    }

    result = "";
    for (QStringList::Iterator it = intermediate.begin(); it != intermediate.end(); ++it) {
        if (!result.isEmpty()) result.append(splitMarker);
        result.append(*it);

        // skip math regions
        ++it;

        if (it == intermediate.end())
            break;

        if ((*it).length() > 256)
            kWarning() << "Very long math equation using $ found, maybe due to broken inline math: " << (*it).left(48);
    }

    for (QList<EncoderLaTeXPrivate::CharMappingItem>::ConstIterator cmit = d->charMapping.begin(); cmit != d->charMapping.end(); ++cmit)
        result.replace((*cmit).regExp, (*cmit).unicode);
    QStringList transformed = result.split(splitMarker, QString::SkipEmptyParts);

    result = "";
    for (QStringList::Iterator itt = transformed.begin(), iti = intermediate.begin(); itt != transformed.end() && iti != intermediate.end(); ++itt, ++iti) {
        result.append(*itt);

        ++iti;
        if (iti == intermediate.end())
            break;

        result.append("$").append(*iti).append("$");
    }

    /** Reinserting original URLs as explained above */
    pos = 0;
    int idx = 0;
    while (pos >= 0) {
        pos = result.indexOf(httpRegExp, pos);
        if (pos >= 0) {
            ++pos;
            int len = httpRegExp.cap(0).length();
            result = result.left(pos - 1).append(urls[idx++]).append(result.mid(pos + len - 1));
        }
    }

    return result.replace(startStopMarker, "");
}

QString EncoderLaTeX::encode(const QString & text)
{
    const QString splitMarker = "|KBIBTEX|";

    /** start-stop marker ensures that each text starts and stops
      * with plain text and not with an inline math environment.
      * This invariant is exploited implicitly in the code below. */
    const QString startStopMarker = "|STARTSTOP|";
    QString result = startStopMarker + text + startStopMarker;

    /** Collect (all?) urls from the BibTeX file and store them in urls */
    /** Problem is that the replace function below will replace
      * character sequences in the URL rendering the URL invalid.
      * Later, all URLs will be replaced back to their original
      * in the hope nothing breaks ... */
    QStringList urls;
    QRegExp httpRegExp("(ht|f)tps?://[^\"} ]+");
    httpRegExp.setMinimal(false);
    int pos = 0;
    while (pos >= 0) {
        pos = result.indexOf(httpRegExp, pos);
        if (pos >= 0) {
            ++pos;
            QString url = httpRegExp.cap(0);
            urls << url;
        }
    }

    /** split text into math and non-math regions */
    QStringList intermediate = result.split('$', QString::SkipEmptyParts);
    QStringList::Iterator it = intermediate.begin();
    while (it != intermediate.end()) {
        /**
         * Sometimes we split strings like "\$", which is not intended.
         * So, we have to manually fix things by checking for strings
         * ending with "\" and append both the removed dollar sign and
         * the following string (which was never supposed to be an
         * independent string). Finally, we remove the unnecessary
         * string and continue.
         */
        if ((*it).endsWith("\\")) {
            QStringList::Iterator cur = it;
            ++it;
            (*cur).append('$').append(*it);
            it = intermediate.erase(it);
        } else
            ++it;
    }

    result = "";
    for (QStringList::Iterator it = intermediate.begin(); it != intermediate.end(); ++it) {
        if (!result.isEmpty()) result.append(splitMarker);
        result.append(*it);
        ++it;
        if (it == intermediate.end())
            break;
        if ((*it).length() > 256)
            qDebug() << "Very long math equation using $ found, maybe due to broken inline math:" << (*it).left(48) << endl;
    }

    for (QList<EncoderLaTeXPrivate::CharMappingItem>::ConstIterator cmit = d->charMapping.begin(); cmit != d->charMapping.end(); ++cmit)
        result.replace((*cmit).unicode, (*cmit).latex);

    QStringList transformed = result.split(splitMarker, QString::KeepEmptyParts, Qt::CaseSensitive);

    result = "";
    for (QStringList::Iterator itt = transformed.begin(), iti = intermediate.begin(); itt != transformed.end() && iti != intermediate.end(); ++itt, ++iti) {
        result.append(*itt);

        ++iti;
        if (iti == intermediate.end())
            break;

        result.append("$").append(*iti).append("$");
    }

    /** \url accepts unquotet & and _
    May introduce new problem tough */
    if (result.contains("\\url{"))
        result.replace("\\&", "&").replace("\\_", "_").replace(QChar(0x2013), "--").replace("\\#", "#");

    decomposedUTF8toLaTeX(result);

    /** Reinserting original URLs as explained above */
    pos = 0;
    int idx = 0;
    while (pos >= 0) {
        pos = result.indexOf(httpRegExp, pos);
        if (pos >= 0) {
            ++pos;
            int len = httpRegExp.cap(0).length();
            result = result.left(pos - 1).append(urls[idx++]).append(result.mid(pos + len - 1));
        }
    }

    return result.replace(startStopMarker, "");
}

QString EncoderLaTeX::encode(const QString &text, const QChar &replace)
{
    QString result = text;
    for (QList<EncoderLaTeXPrivate::CharMappingItem>::ConstIterator it = d->charMapping.begin(); it != d->charMapping.end(); ++it)
        if ((*it).unicode == replace)
            result.replace((*it).unicode, (*it).latex);
    return result;
}

QString& EncoderLaTeX::decomposedUTF8toLaTeX(QString &text)
{
    for (QList<EncoderLaTeXPrivate::CombinedMappingItem>::Iterator it = d->combinedMapping.begin(); it != d->combinedMapping.end(); ++it) {
        int i = (*it).regExp.indexIn(text);
        while (i >= 0) {
            QString a = (*it).regExp.cap(1);
            text = text.left(i) + "\\" + (*it).latex + "{" + a + "}" + text.mid(i + 2);
            i = (*it).regExp.indexIn(text, i + 1);
        }
    }

    return text;
}


EncoderLaTeX* EncoderLaTeX::currentEncoderLaTeX()
{
    if (encoderLaTeX == NULL)
        encoderLaTeX = new EncoderLaTeX();

    return encoderLaTeX;
}

void EncoderLaTeX::deleteCurrentEncoderLaTeX()
{
    if (encoderLaTeX != NULL) {
        delete encoderLaTeX;
        encoderLaTeX = NULL;
    }
}

QString& EncoderLaTeX::convertToPlainAscii(QString &text)
{
    for (int i = 0; i < modcharmappingdatalatexcount; ++i) {
        QChar c = QChar(modcharmappingdatalatex[i].unicode);
        if (text.indexOf(c) >= 0)
            text = text.replace(c, QString(modcharmappingdatalatex[i].letter));
    }
    for (int i = 0; i < commandmappingdatalatexcount; ++i) {
        QChar c = QChar(commandmappingdatalatex[i].unicode);
        if (text.indexOf(c) >= 0)
            text = text.replace(c, QString(commandmappingdatalatex[i].letters));
    }

    return text;
}
