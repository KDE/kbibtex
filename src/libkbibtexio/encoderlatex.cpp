/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QDebug>

#include <encoderlatex.h>

using namespace KBibTeX::IO;

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
    {"\"", 0x0308},
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
    {"L", 0x0141},
    {"l", 0x0142},
    {"grqq", 0x201C},
    {"glqq", 0x201E},
    {"frqq", 0x00BB},
    {"flqq", 0x00AB},

// awk -F '[{}\\\\]+' '/DeclareUnicodeCharacter/ { print "{\""$4"\", 0x"$3"},"}' /usr/share/texmf-dist/tex/latex/base/t2aenc.dfu | grep '0x04' | sort -r -f
    {"cyrzhdsc", 0x0497},
    {"CYRZHDSC", 0x0496},
    {"cyrzh", 0x0436},
    {"CYRZH", 0x0416},
    {"cyrzdsc", 0x0499},
    {"CYRZDSC", 0x0498},
    {"cyrz", 0x0437},
    {"CYRZ", 0x0417},
    {"cyryu", 0x044E},
    {"CYRYU", 0x042E},
    {"cyryo", 0x0451},
    {"CYRYO", 0x0401},
    {"cyryi", 0x0457},
    {"CYRYI", 0x0407},
    {"cyryhcrs", 0x04B1},
    {"CYRYHCRS", 0x04B0},
    {"cyrya", 0x044F},
    {"CYRYA", 0x042F},
    {"cyry", 0x04AF},
    {"CYRY", 0x04AE},
    {"cyrv", 0x0432},
    {"CYRV", 0x0412},
    {"cyrushrt", 0x045E},
    {"CYRUSHRT", 0x040E},
    {"cyru", 0x0443},
    {"CYRU", 0x0423},
    {"cyrtshe", 0x045B},
    {"CYRTSHE", 0x040B},
    {"cyrtdsc", 0x04AD},
    {"CYRTDSC", 0x04AC},
    {"cyrt", 0x0442},
    {"CYRT", 0x0422},
    {"cyrshha", 0x04BB},
    {"CYRSHHA", 0x04BA},
    {"cyrshch", 0x0449},
    {"CYRSHCH", 0x0429},
    {"cyrsh", 0x0448},
    {"CYRSH", 0x0428},
    {"cyrsftsn", 0x044C},
    {"CYRSFTSN", 0x042C},
    {"cyrsdsc", 0x04AB},
    {"CYRSDSC", 0x04AA},
    {"cyrschwa", 0x04D9},
    {"CYRSCHWA", 0x04D8},
    {"cyrs", 0x0441},
    {"CYRS", 0x0421},
    {"cyrr", 0x0440},
    {"CYRR", 0x0420},
    {"CYRpalochka", 0x04C0},
    {"cyrp", 0x043F},
    {"CYRP", 0x041F},
    {"cyrotld", 0x04E9},
    {"CYROTLD", 0x04E8},
    {"cyro", 0x043E},
    {"CYRO", 0x041E},
    {"cyrnje", 0x045A},
    {"CYRNJE", 0x040A},
    {"cyrng", 0x04A5},
    {"CYRNG", 0x04A4},
    {"cyrndsc", 0x04A3},
    {"CYRNDSC", 0x04A2},
    {"cyrn", 0x043D},
    {"CYRN", 0x041D},
    {"cyrm", 0x043C},
    {"CYRM", 0x041C},
    {"cyrlje", 0x0459},
    {"CYRLJE", 0x0409},
    {"cyrl", 0x043B},
    {"CYRL", 0x041B},
    {"cyrkvcrs", 0x049D},
    {"CYRKVCRS", 0x049C},
    {"cyrkdsc", 0x049B},
    {"CYRKDSC", 0x049A},
    {"cyrk", 0x043A},
    {"CYRK", 0x041A},
    {"cyrje", 0x0458},
    {"CYRJE", 0x0408},
    {"cyrishrt", 0x0439},
    {"CYRISHRT", 0x0419},
    {"cyrii", 0x0456},
    {"CYRII", 0x0406},
    {"cyrie", 0x0454},
    {"CYRIE", 0x0404},
    {"cyri", 0x0438},
    {"CYRI", 0x0418},
    {"cyrhrdsn", 0x044A},
    {"CYRHRDSN", 0x042A},
    {"cyrhdsc", 0x04B3},
    {"CYRHDSC", 0x04B2},
    {"cyrh", 0x0445},
    {"CYRH", 0x0425},
    {"cyrgup", 0x0491},
    {"CYRGUP", 0x0490},
    {"cyrghcrs", 0x0493},
    {"CYRGHCRS", 0x0492},
    {"cyrg", 0x0433},
    {"CYRG", 0x0413},
    {"cyrf", 0x0444},
    {"CYRF", 0x0424},
    {"cyrery", 0x044B},
    {"CYRERY", 0x042B},
    {"cyrerev", 0x044D},
    {"CYREREV", 0x042D},
    {"cyre", 0x0435},
    {"CYRE", 0x0415},
    {"cyrdzhe", 0x045F},
    {"CYRDZHE", 0x040F},
    {"cyrdze", 0x0455},
    {"CYRDZE", 0x0405},
    {"cyrdje", 0x0452},
    {"CYRDJE", 0x0402},
    {"cyrd", 0x0434},
    {"CYRD", 0x0414},
    {"cyrchvcrs", 0x04B9},
    {"CYRCHVCRS", 0x04B8},
    {"cyrchrdsc", 0x04B7},
    {"CYRCHRDSC", 0x04B6},
    {"cyrch", 0x0447},
    {"CYRCH", 0x0427},
    {"cyrc", 0x0446},
    {"CYRC", 0x0426},
    {"cyrb", 0x0431},
    {"CYRB", 0x0411},
    {"cyrae", 0x04D5},
    {"CYRAE", 0x04D4},
    {"cyra", 0x0430},
    {"CYRA", 0x0410}
};

static const int commandmappingdatalatexcount = sizeof(commandmappingdatalatex) / sizeof(commandmappingdatalatex[ 0 ]) ;

const char *expansionsCmd[] = {"\\{\\\\%1\\}", "\\\\%1\\{\\}", "\\\\%1"};
static const  int expansionscmdcount = sizeof(expansionsCmd) / sizeof(expansionsCmd[0]);

static const struct EncoderLaTeXModCharMapping {
    const char *modifier;
    const char *letter;
    unsigned int unicode;
}
modcharmappingdatalatex[] = {
    {"\\\\`", "A", 0x00C0},
    {"\\\\'", "A", 0x00C1},
    {"\\\\\\^", "A", 0x00C21},
    {"\\\\~", "A", 0x00C3},
    {"\\\\\"", "A", 0x00C4},
    {"\\\\r", "A", 0x00C5},
    {"\\\\c", "C", 0x00C7},
    {"\\\\`", "E", 0x00C8},
    {"\\\\'", "E", 0x00C9},
    {"\\\\\\^", "E", 0x00CA},
    {"\\\\\"", "E", 0x00CB},
    {"\\\\'", "I", 0x00CD},
    {"\\\\\\^", "I", 0x00CE},
    {"\\\\~", "N", 0x00D1},
    {"\\\\`", "O", 0x00D2},
    {"\\\\'", "O", 0x00D3},
    {"\\\\\\^", "O", 0x00D4},
    {"\\\\\"", "O", 0x00D6},
    {"\\\\", "O", 0x00D8},
    {"\\\\`", "U", 0x00D9},
    {"\\\\'", "U", 0x00DA},
    {"\\\\\\^", "U", 0x00DB},
    {"\\\\\"", "U", 0x00DC},
    {"\\\\'", "Y", 0x00DD},
    {"\\\\\"", "s", 0x00DF},
    {"\\\\`", "a", 0x00E0},
    {"\\\\'", "a", 0x00E1},
    {"\\\\\\^", "a", 0x00E2},
    {"\\\\~", "a", 0x00E3},
    {"\\\\\"", "a", 0x00E4},
    {"\\\\r", "a", 0x00E5},
    {"\\\\c", "c", 0x00E7},
    {"\\\\`", "e", 0x00E8},
    {"\\\\'", "e", 0x00E9},
    {"\\\\\\^", "e", 0x00EA},
    {"\\\\\"", "e", 0x00EB},
    {"\\\\'", "i", 0x00ED},
    {"\\\\'", "\\\\i", 0x00ED},
    {"\\\\\\^", "i", 0x00EE},
    {"\\\\~", "n", 0x00F1},
    {"\\\\`", "o", 0x00F2},
    {"\\\\'", "o", 0x00F3},
    {"\\\\\\^", "o", 0x00F4},
    {"\\\\\"", "o", 0x00F6},
    {"\\\\", "o", 0x00F8},
    {"\\\\`", "u", 0x00F9},
    {"\\\\'", "u", 0x00FA},
    {"\\\\\\^", "u", 0x00FB},
    {"\\\\\"", "u", 0x00FC},
    {"\\\\'", "y", 0x00FD},
    {"\\\\u", "A", 0x0102},
    {"\\\\u", "a", 0x0103},
    {"\\\\'", "C", 0x0106},
    {"\\\\'", "c", 0x0107},
    {"\\\\v", "E", 0x010A},
    {"\\\\v", "e", 0x010B},
    {"\\\\v", "C", 0x010C},
    {"\\\\v", "c", 0x010D},
    {"\\\\c", "E", 0x0118},
    {"\\\\c", "e", 0x0119},
    {"\\\\u", "G", 0x011E},
    {"\\\\u", "g", 0x011F},
    {"\\\\'", "N", 0x0143},
    {"\\\\'", "n", 0x0144},
    {"\\\\H", "O", 0x0150},
    {"\\\\H", "o", 0x0151},
    {"\\\\'", "S", 0x015A},
    {"\\\\'", "s", 0x015B},
    {"\\\\c", "S", 0x015E},
    {"\\\\c", "s", 0x015F},
    {"\\\\v", "S", 0x0160},
    {"\\\\v", "s", 0x0161},
    {"\\\\r", "U", 0x016E},
    {"\\\\r", "u", 0x016F},
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
    /*{"\\\\&", 0x0026, "\\&"},*/
    {"!`", 0x00A1, "!`"},
    {"\"<", 0x00AB, "\"<"},
    {"\">", 0x00BB, "\">"},
    {"[?]`", 0x00BF, "?`"},
    {"--", 0x2013, "--"}
};

static const int charmappingdatalatexcount = sizeof(charmappingdatalatex) / sizeof(charmappingdatalatex[ 0 ]) ;

EncoderLaTeX::EncoderLaTeX()
        : Encoder()
{
    buildCharMapping();
    buildCombinedMapping();
}

EncoderLaTeX::~EncoderLaTeX()
{
    // nothing
}

QString EncoderLaTeX::decode(const QString & text)
{
    QString result = text;
    decomposedUTF8toLaTeX(result);


    // split text into math and non-math regions
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
            intermediate.erase(it);
            it = cur;
        } else
            ++it;
    }

    if (intermediate.size() > 100)
        qWarning("This BibTeX source code contains many inline math fragements (%d) which thwarts KBibTeX when parsing the code.", intermediate.size());
    for (QStringList::Iterator it = intermediate.begin(); it != intermediate.end(); it++) {
        for (QLinkedList<CharMappingItem>::ConstIterator cmit = m_charMapping.begin(); cmit != m_charMapping.end(); ++cmit)
            (*it).replace((*cmit).regExp, (*cmit).unicode);

        // skip math regions
        it++;

        if (it == intermediate.end())
            break;

        if ((*it).length() > 256)
            qWarning() << "Very long math equation using $ found, maybe due to broken inline math: " << (*it).left(48);
    }

    result = intermediate.join("$");

    return result;
}

QString EncoderLaTeX::encode(const QString & text)
{
    QString result = text;
    bool beginningQuotationNext = TRUE;

    // result.replace( "\\", "\\\\" );

    for (QLinkedList<CharMappingItem>::ConstIterator it = m_charMapping.begin(); it != m_charMapping.end(); ++it)
        result.replace((*it).unicode, (*it).latex);

    for (int i = 0; i < result.length(); i++)
        if (result.at(i) == '"' && (i == 0 || result.at(i - 1) != '\\')) {
            if (beginningQuotationNext)
                result.replace(i, 1, "``");
            else
                result.replace(i, 1, "''");

            beginningQuotationNext = !beginningQuotationNext;
        }

    /** \url accepts unquotet &
       May introduce new problem tough */
    if (result.contains("\\url{"))
        result.replace("\\&", "&");

    decomposedUTF8toLaTeX(result);

    return result;
}

QString EncoderLaTeX::encode(const QString &text, const QChar &replace)
{
    QString result = text;
    for (QLinkedList<CharMappingItem>::ConstIterator it = m_charMapping.begin(); it != m_charMapping.end(); ++it)
        if ((*it).unicode == replace)
            result.replace((*it).unicode, (*it).latex);
    return result;
}

QString EncoderLaTeX::encodeSpecialized(const QString & text, const EntryField::FieldType fieldType)

{
    QString result = encode(text);

    switch (fieldType) {
    case EntryField::ftPages:
        result.replace(QChar(0x2013), "--");

        break;

    default:
        break;
    }

    return result;
}

QString& EncoderLaTeX::decomposedUTF8toLaTeX(QString &text)
{
    for (QLinkedList<CombinedMappingItem>::Iterator it = m_combinedMapping.begin(); it != m_combinedMapping.end(); ++it) {
        int i = (*it).regExp.indexIn(text);
        while (i >= 0) {
            QString a = (*it).regExp.cap(1);
            text = text.left(i) + "\\" + (*it).latex + "{" + a + "}" + text.mid(i + 2);
            i = (*it).regExp.indexIn(text, i + 1);
        }
    }

    return text;
}

void EncoderLaTeX::buildCombinedMapping()
{
    for (int i = 0; i < decompositionscount; i++) {
        CombinedMappingItem item;
        item.regExp = QRegExp("(.)" + QString(QChar(decompositions[i].unicode)));
        item.latex = decompositions[i].latexCommand;
        m_combinedMapping.append(item);
    }
}

void EncoderLaTeX::buildCharMapping()
{
    /** encoding and decoding for digraphs such as -- or ?` */
    for (int i = 0; i < charmappingdatalatexcount; i++) {
        CharMappingItem charMappingItem;
        charMappingItem.regExp = QRegExp(charmappingdatalatex[ i ].regexp);
        charMappingItem.unicode = QChar(charmappingdatalatex[ i ].unicode);
        charMappingItem.latex = QString(charmappingdatalatex[ i ].latex);
        m_charMapping.append(charMappingItem);
    }

    /** encoding and decoding for commands such as \AA or \ss */
    for (int i = 0; i < commandmappingdatalatexcount; ++i) {
        /** different types of writing such as {\AA} or \AA{} possible */
        for (int j = 0; j < expansionscmdcount; ++j) {
            CharMappingItem charMappingItem;
            charMappingItem.regExp = QRegExp(QString(expansionsCmd[j]).arg(commandmappingdatalatex[i].letters));
            charMappingItem.unicode = QChar(commandmappingdatalatex[i].unicode);
            charMappingItem.latex = QString("{\\%1}").arg(commandmappingdatalatex[i].letters);
            m_charMapping.append(charMappingItem);
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
                m_charMapping.append(charMappingItem);
            }

        /** second batch of replacement rules, where a separator is required between modifier and character (e.g. \v{g}) */
        for (int j = 0; j < expansionsmod1count; ++j) {
            CharMappingItem charMappingItem;
            charMappingItem.regExp = QRegExp(QString(expansionsMod1[j]).arg(modifierRegExp).arg(modcharmappingdatalatex[i].letter));
            charMappingItem.unicode = QChar(modcharmappingdatalatex[i].unicode);
            charMappingItem.latex = QString("%1{%2}").arg(modifier).arg(modcharmappingdatalatex[i].letter);
            m_charMapping.append(charMappingItem);
        }
    }
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
