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

#include "idsuggestions.h"

#include <QRegularExpression>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "journalabbreviations.h"
#include "encoderlatex.h"

class IdSuggestions::IdSuggestionsPrivate
{
private:
    IdSuggestions *p;
    KSharedConfigPtr config;
    const KConfigGroup group;
    static const QStringList smallWords;

public:

    IdSuggestionsPrivate(IdSuggestions *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), group(config, IdSuggestions::configGroupName) {
        // nothing
    }

    QString normalizeText(const QString &input) const {
        static const QRegExp unwantedChars = QRegExp("[^-_:/=+a-zA-Z0-9]+");
        return EncoderLaTeX::instance().convertToPlainAscii(input).remove(unwantedChars);
    }

    int numberFromEntry(const Entry &entry, const QString &field) const {
        static const QRegularExpression firstDigits(QStringLiteral("^[0-9]+"));
        const QString text = PlainTextValue::text(entry.value(field));
        const QRegularExpressionMatch match = firstDigits.match(text);
        if (!match.hasMatch()) return -1;

        bool ok = false;
        const int result = match.captured(0).toInt(&ok);
        return ok ? result : -1;
    }

    QString pageNumberFromEntry(const Entry &entry) const {
        static const QRegularExpression whitespace(QStringLiteral("[ \t]+"));
        static const QRegularExpression pageNumber(QStringLiteral("[a-z0-9+:]+"), QRegularExpression::CaseInsensitiveOption);
        const QString text = PlainTextValue::text(entry.value(Entry::ftPages)).remove(whitespace).remove(QStringLiteral("mbox"));
        const QRegularExpressionMatch match = pageNumber.match(text);
        if (!match.hasMatch()) return QString();
        return match.captured(0);
    }

    QString translateTitleToken(const Entry &entry, const struct IdSuggestionTokenInfo &tti, bool removeSmallWords) const {
        QString result;
        bool first = true;
        static const QRegExp sequenceOfSpaces(QStringLiteral("\\s+"));
        const QStringList titleWords = PlainTextValue::text(entry.value(Entry::ftTitle)).split(sequenceOfSpaces, QString::SkipEmptyParts);
        int index = 0;
        for (QStringList::ConstIterator it = titleWords.begin(); it != titleWords.end(); ++it, ++index) {
            const QString lowerText = normalizeText(*it).toLower();
            if ((removeSmallWords && smallWords.contains(lowerText)) || index < tti.startWord || index > tti.endWord)
                continue;

            if (first)
                first = false;
            else
                result.append(tti.inBetween);

            QString titleComponent = lowerText.left(tti.len);
            if (tti.caseChange == IdSuggestions::ccToCamelCase)
                titleComponent = titleComponent[0].toUpper() + titleComponent.mid(1);

            result.append(titleComponent);
        }

        switch (tti.caseChange) {
        case IdSuggestions::ccToUpper:
            result = result.toUpper();
            break;
        case IdSuggestions::ccToLower:
            result = result.toLower();
            break;
        case IdSuggestions::ccToCamelCase:
            /// already processed above
        case IdSuggestions::ccNoChange:
            /// nothing
            break;
        }

        return result;
    }

    QString translateAuthorsToken(const Entry &entry, const struct IdSuggestionTokenInfo &ati) const {
        QString result;
        /// Already some author inserted into result?
        bool firstInserted = false;
        /// Get list of authors' last names
        const QStringList authors = entry.authorsLastName();
        /// Keep track of which author (number/position) is processed
        int index = 0;
        /// Go through all authors
        for (QStringList::ConstIterator it = authors.constBegin(); it != authors.constEnd(); ++it, ++index) {
            /// Get current author, normalize name (remove unwanted characters), cut to maximum length
            QString author = normalizeText(*it).left(ati.len);
            /// Check if camel case is requests
            if (ati.caseChange == IdSuggestions::ccToCamelCase) {
                /// Get components of the author's last name
                const QStringList nameComponents = author.split(QStringLiteral(" "), QString::SkipEmptyParts);
                QStringList newNameComponents;
                newNameComponents.reserve(nameComponents.size());
                /// Camel-case each name component
                for (const QString &nameComponent : nameComponents) {
                    newNameComponents.append(nameComponent[0].toUpper() + nameComponent.mid(1));
                }
                /// Re-assemble name from camel-cased components
                author = newNameComponents.join(QStringLiteral(" "));
            }
            if (
                (index >= ati.startWord && index <= ati.endWord) ///< check for requested author range
                || (ati.lastWord && index == authors.count() - 1) ///< explicitly insert last author if requested in lastWord flag
            ) {
                if (firstInserted)
                    result.append(ati.inBetween);
                result.append(author);
                firstInserted = true;
            }
        }

        switch (ati.caseChange) {
        case IdSuggestions::ccToUpper:
            result = result.toUpper();
            break;
        case IdSuggestions::ccToLower:
            result = result.toLower();
            break;
        case IdSuggestions::ccToCamelCase:
            /// already processed above
            break;
        case IdSuggestions::ccNoChange:
            /// nothing
            break;
        }

        return result;
    }

    QString translateJournalToken(const Entry &entry, const struct IdSuggestionTokenInfo &jti, bool removeSmallWords) const {
        static const QRegExp sequenceOfSpaces(QStringLiteral("\\s+"));
        QString journalName = PlainTextValue::text(entry.value(Entry::ftJournal));
        journalName = JournalAbbreviations::self()->toShortName(journalName);
        const QStringList journalWords = journalName.split(sequenceOfSpaces, QString::SkipEmptyParts);
        bool first = true;
        int index = 0;
        QString result;
        for (QStringList::ConstIterator it = journalWords.begin(); it != journalWords.end(); ++it, ++index) {
            QString journalComponent = normalizeText(*it);
            const QString lowerText = journalComponent.toLower();
            if ((removeSmallWords && smallWords.contains(lowerText)) || index < jti.startWord || index > jti.endWord)
                continue;

            if (first)
                first = false;
            else
                result.append(jti.inBetween);

            /// Try to keep sequences of capital letters at the start of the journal name,
            /// those may already be abbreviations.
            unsigned int countCaptialCharsAtStart = 0;
            while (journalComponent[countCaptialCharsAtStart].isUpper()) ++countCaptialCharsAtStart;
            journalComponent = journalComponent.left(qMax(jti.len, countCaptialCharsAtStart));

            if (jti.caseChange == IdSuggestions::ccToCamelCase)
                journalComponent = journalComponent[0].toUpper() + journalComponent.mid(1);

            result.append(journalComponent);
        }

        switch (jti.caseChange) {
        case IdSuggestions::ccToUpper:
            result = result.toUpper();
            break;
        case IdSuggestions::ccToLower:
            result = result.toLower();
            break;
        case IdSuggestions::ccToCamelCase:
            /// already processed above
        case IdSuggestions::ccNoChange:
            /// nothing
            break;
        }

        return result;
    }

    QString translateTypeToken(const Entry &entry, const struct IdSuggestionTokenInfo &eti) const {
        QString entryType(entry.type());

        switch (eti.caseChange) {
        case IdSuggestions::ccToUpper:
            return entryType.toUpper().left(eti.len);
        case IdSuggestions::ccToLower:
            return entryType.toLower().left(eti.len);
        case IdSuggestions::ccToCamelCase:
        {
            if (entryType.isEmpty()) return QString(); ///< empty entry type? Return immediately to avoid problems with entryType[0]
            /// Apply some heuristic replacements to make the entry type look like CamelCase
            entryType = entryType.toLower(); ///< start with lower case
            /// Then, replace known words with their CamelCase variant
            entryType = entryType.replace(QStringLiteral("report"), QStringLiteral("Report")).replace(QStringLiteral("proceedings"), QStringLiteral("Proceedings")).replace(QStringLiteral("thesis"), QStringLiteral("Thesis")).replace(QStringLiteral("book"), QStringLiteral("Book")).replace(QStringLiteral("phd"), QStringLiteral("PhD"));
            /// Finally, guarantee that first letter is upper case
            entryType[0] = entryType[0].toUpper();
            return entryType.left(eti.len);
        }
        default:
            return entryType.left(eti.len);
        }
    }

    QString translateToken(const Entry &entry, const QString &token) const {
        switch (token[0].toLatin1()) {
        case 'a': ///< deprecated but still supported case
        {
            /// Evaluate the token string, store information in struct IdSuggestionTokenInfo ati
            struct IdSuggestionTokenInfo ati = p->evalToken(token.mid(1));
            ati.startWord = ati.endWord = 0; ///< only first author
            return translateAuthorsToken(entry, ati);
        }
        case 'A': {
            /// Evaluate the token string, store information in struct IdSuggestionTokenInfo ati
            const struct IdSuggestionTokenInfo ati = p->evalToken(token.mid(1));
            return translateAuthorsToken(entry, ati);
        }
        case 'z': ///< deprecated but still supported case
        {
            /// Evaluate the token string, store information in struct IdSuggestionTokenInfo ati
            struct IdSuggestionTokenInfo ati = p->evalToken(token.mid(1));
            /// All but first author
            ati.startWord = 1;
            ati.endWord = 0x00ffffff;
            return translateAuthorsToken(entry, ati);
        }
        case 'y': {
            int year = numberFromEntry(entry, Entry::ftYear);
            if (year > -1)
                return QString::number(year % 100 + 100).mid(1);
            break;
        }
        case 'Y': {
            const int year = numberFromEntry(entry, Entry::ftYear);
            if (year > -1)
                return QString::number(year % 10000 + 10000).mid(1);
            break;
        }
        case 't':
        case 'T': {
            /// Evaluate the token string, store information in struct IdSuggestionTokenInfo jti
            const struct IdSuggestionTokenInfo tti = p->evalToken(token.mid(1));
            return translateTitleToken(entry, tti, token[0].isUpper());
        }
        case 'j':
        case 'J': {
            /// Evaluate the token string, store information in struct IdSuggestionTokenInfo jti
            const struct IdSuggestionTokenInfo jti = p->evalToken(token.mid(1));
            return translateJournalToken(entry, jti, token[0].isUpper());
        }
        case 'e': {
            /// Evaluate the token string, store information in struct IdSuggestionTokenInfo eti
            const struct IdSuggestionTokenInfo eti = p->evalToken(token.mid(1));
            return translateTypeToken(entry, eti);
        }
        case 'v': {
            return normalizeText(PlainTextValue::text(entry.value(Entry::ftVolume)));
        }
        case 'p': {
            return pageNumberFromEntry(entry);
        }
        case '"': return token.mid(1);
        }

        return QString();
    }

    QString defaultFormatString() const {
        return group.readEntry(keyDefaultFormatString, defaultDefaultFormatString);
    }

    QStringList formatStringList() const {
        return group.readEntry(keyFormatStringList, defaultFormatStringList);
    }
};

/// List of small words taken from OCLC:
/// https://www.oclc.org/developer/develop/web-services/worldcat-search-api/bibliographic-resource.en.html
const QStringList IdSuggestions::IdSuggestionsPrivate::smallWords = i18nc("Small words that can be removed from titles when generating id suggestions; separated by pipe symbol", "a|als|am|an|are|as|at|auf|aus|be|but|by|das|dass|de|der|des|dich|dir|du|er|es|for|from|had|have|he|her|his|how|ihr|ihre|ihres|im|in|is|ist|it|kein|la|le|les|mein|mich|mir|mit|of|on|sein|sie|that|the|this|to|un|une|von|was|wer|which|wie|wird|with|yousie|that|the|this|to|un|une|von|was|wer|which|wie|wird|with|you").split(QStringLiteral("|"), QString::SkipEmptyParts);


const QString IdSuggestions::keyDefaultFormatString = QStringLiteral("DefaultFormatString");
const QString IdSuggestions::defaultDefaultFormatString = QString();
const QString IdSuggestions::keyFormatStringList = QStringLiteral("FormatStringList");
const QStringList IdSuggestions::defaultFormatStringList = QStringList() << QStringLiteral("A") << QStringLiteral("A2|y") << QStringLiteral("A3|y") << QStringLiteral("A4|y|\":|T5") << QStringLiteral("al|\":|T") << QStringLiteral("al|y") << QStringLiteral("al|Y") << QStringLiteral("Al\"-|\"-|y") << QStringLiteral("Al\"+|Y") << QStringLiteral("al|y|T") << QStringLiteral("al|Y|T3") << QStringLiteral("al|Y|T3l") << QStringLiteral("a|\":|Y|\":|T1") << QStringLiteral("a|y") << QStringLiteral("A|\":|Y");
const QString IdSuggestions::configGroupName = QStringLiteral("IdSuggestions");

IdSuggestions::IdSuggestions()
        : d(new IdSuggestionsPrivate(this))
{
    /// nothing
}

IdSuggestions::~IdSuggestions()
{
    delete d;
}

QString IdSuggestions::formatId(const Entry &entry, const QString &formatStr) const
{
    QString id;
    const QStringList tokenList = formatStr.split(QStringLiteral("|"), QString::SkipEmptyParts);
    for (const QString &token : tokenList) {
        id.append(d->translateToken(entry, token));
    }

    return id;
}

QString IdSuggestions::defaultFormatId(const Entry &entry) const
{
    return formatId(entry, d->defaultFormatString());
}

bool IdSuggestions::hasDefaultFormat() const
{
    return !d->defaultFormatString().isEmpty();
}

bool IdSuggestions::applyDefaultFormatId(Entry &entry) const
{
    const QString dfs = d->defaultFormatString();
    if (!dfs.isEmpty()) {
        entry.setId(defaultFormatId(entry));
        return true;
    } else
        return false;
}

QStringList IdSuggestions::formatIdList(const Entry &entry) const
{
    const QStringList formatStrings = d->formatStringList();
    QStringList result;
    result.reserve(formatStrings.size());
    for (const QString &formatString : formatStrings) {
        result << formatId(entry, formatString);
    }
    return result;
}

QStringList IdSuggestions::formatStrToHuman(const QString &formatStr) const
{
    QStringList result;
    const QStringList tokenList = formatStr.split(QStringLiteral("|"), QString::SkipEmptyParts);
    for (const QString &token : tokenList) {
        QString text;
        if (token[0] == 'a' || token[0] == 'A' || token[0] == 'z') {
            struct IdSuggestionTokenInfo info = evalToken(token.mid(1));
            if (token[0] == 'a')
                info.startWord = info.endWord = 0;
            else if (token[0] == 'z') {
                info.startWord = 1;
                info.endWord = 0x00ffffff;
            }
            text = formatAuthorRange(info.startWord, info.endWord, info.lastWord);

            int n = info.len;
            if (info.len < 0x00ffffff) text.append(i18np(", but only first letter of each last name", ", but only first %1 letters of each last name", n));

            switch (info.caseChange) {
            case IdSuggestions::ccToUpper:
                text.append(i18n(", in upper case"));
                break;
            case IdSuggestions::ccToLower:
                text.append(i18n(", in lower case"));
                break;
            case IdSuggestions::ccToCamelCase:
                text.append(i18n(", in CamelCase"));
                break;
            case IdSuggestions::ccNoChange:
                break;
            }

            if (!info.inBetween.isEmpty()) text.append(i18n(", with '%1' in between", info.inBetween));
        }
        else if (token[0] == 'y')
            text.append(i18n("Year (2 digits)"));
        else if (token[0] == 'Y')
            text.append(i18n("Year (4 digits)"));
        else if (token[0] == 't' || token[0] == 'T') {
            struct IdSuggestionTokenInfo info = evalToken(token.mid(1));
            text.append(i18n("Title"));
            if (info.startWord == 0 && info.endWord <= 0xffff)
                text.append(i18np(", but only the first word", ", but only first %1 words", info.endWord + 1));
            else if (info.startWord > 0 && info.endWord > 0xffff)
                text.append(i18n(", but only starting from word %1", info.startWord + 1));
            else if (info.startWord > 0 && info.endWord <= 0xffff)
                text.append(i18n(", but only from word %1 to word %2", info.startWord + 1, info.endWord + 1));
            if (info.len < 0x00ffffff)
                text.append(i18np(", but only first letter of each word", ", but only first %1 letters of each word", info.len));

            switch (info.caseChange) {
            case IdSuggestions::ccToUpper:
                text.append(i18n(", in upper case"));
                break;
            case IdSuggestions::ccToLower:
                text.append(i18n(", in lower case"));
                break;
            case IdSuggestions::ccToCamelCase:
                text.append(i18n(", in CamelCase"));
                break;
            case IdSuggestions::ccNoChange:
                break;
            }

            if (!info.inBetween.isEmpty()) text.append(i18n(", with '%1' in between", info.inBetween));
            if (token[0] == 'T') text.append(i18n(", small words removed"));
        }
        else if (token[0] == 'j') {
            struct IdSuggestionTokenInfo info = evalToken(token.mid(1));
            text.append(i18n("Journal"));
            if (info.len < 0x00ffffff)
                text.append(i18np(", but only first letter of each word", ", but only first %1 letters of each word", info.len));
            switch (info.caseChange) {
            case IdSuggestions::ccToUpper:
                text.append(i18n(", in upper case"));
                break;
            case IdSuggestions::ccToLower:
                text.append(i18n(", in lower case"));
                break;
            case IdSuggestions::ccToCamelCase:
                text.append(i18n(", in CamelCase"));
                break;
            case IdSuggestions::ccNoChange:
                break;
            }
        } else if (token[0] == 'e') {
            struct IdSuggestionTokenInfo info = evalToken(token.mid(1));
            text.append(i18n("Type"));
            if (info.len < 0x00ffffff)
                text.append(i18np(", but only first letter of each word", ", but only first %1 letters of each word", info.len));
            switch (info.caseChange) {
            case IdSuggestions::ccToUpper:
                text.append(i18n(", in upper case"));
                break;
            case IdSuggestions::ccToLower:
                text.append(i18n(", in lower case"));
                break;
            case IdSuggestions::ccToCamelCase:
                text.append(i18n(", in CamelCase"));
                break;
            default:
                break;
            }
        } else if (token[0] == 'v') {
            text.append(i18n("Volume"));
        } else if (token[0] == 'p') {
            text.append(i18n("First page number"));
        } else if (token[0] == '"')
            text.append(i18n("Text: '%1'", token.mid(1)));
        else
            text.append("?");

        result.append(text);
    }

    return result;
}

QString IdSuggestions::formatAuthorRange(int minValue, int maxValue, bool lastAuthor) {
    if (minValue == 0) {
        if (maxValue == 0) {
            if (lastAuthor)
                return i18n("First and last authors only");
            else
                return i18n("First author only");
        } else if (maxValue > 0xffff)
            return i18n("All authors");
        else {
            if (lastAuthor)
                return i18n("From first author to author %1 and last author", maxValue + 1);
            else
                return i18n("From first author to author %1", maxValue + 1);
        }
    } else if (minValue == 1) {
        if (maxValue > 0xffff)
            return i18n("All but first author");
        else {
            if (lastAuthor)
                return i18n("From author %1 to author %2 and last author", minValue + 1, maxValue + 1);
            else
                return i18n("From author %1 to author %2", minValue + 1, maxValue + 1);
        }
    } else {
        if (maxValue > 0xffff)
            return i18n("From author %1 to last author", minValue + 1);
        else if (lastAuthor)
            return i18n("From author %1 to author %2 and last author", minValue + 1, maxValue + 1);
        else
            return i18n("From author %1 to author %2", minValue + 1, maxValue + 1);
    }
}

struct IdSuggestions::IdSuggestionTokenInfo IdSuggestions::evalToken(const QString &token) const {
    int pos = 0;
    struct IdSuggestionTokenInfo result;
    result.len = 0x00ffffff;
    result.startWord = 0;
    result.endWord = 0x00ffffff;
    result.lastWord = false;
    result.caseChange = IdSuggestions::ccNoChange;
    result.inBetween = QString();

    if (token.length() > pos) {
        int dv = token[pos].digitValue();
        if (dv > -1) {
            result.len = dv;
            ++pos;
        }
    }

    if (token.length() > pos) {
        switch (token[pos].unicode()) {
        case 0x006c: // 'l'
            result.caseChange = IdSuggestions::ccToLower;
            ++pos;
            break;
        case 0x0075: // 'u'
            result.caseChange = IdSuggestions::ccToUpper;
            ++pos;
            break;
        case 0x0063: // 'c'
            result.caseChange = IdSuggestions::ccToCamelCase;
            ++pos;
            break;
        default:
            result.caseChange = IdSuggestions::ccNoChange;
        }
    }

    int dvStart = -1, dvEnd = 0x00ffffff;
    if (token.length() > pos + 2 ///< sufficiently many characters to follow
            && token[pos] == 'w' ///< identifier to start specifying a range of words
            && (dvStart = token[pos + 1].digitValue()) > -1 ///< first word index correctly parsed
            && (
                token[pos + 2] == QLatin1Char('I') ///< infinitely many words
                || (dvEnd = token[pos + 2].digitValue()) > -1) ///< last word index finite and correctly parsed
       ) {
        result.startWord = dvStart;
        result.endWord = dvEnd;
        pos += 3;

        /// Optionally, the last word (e.g. last author) is explicitly requested
        if (token.length() > pos && token[pos] == QLatin1Char('L')) {
            result.lastWord = true;
            ++pos;
        }
    }

    if (token.length() > pos + 1 && token[pos] == '"')
        result.inBetween = token.mid(pos + 1);

    return result;
}
