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

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocale>

#include "encoderlatex.h"
#include "idsuggestions.h"

class IdSuggestions::IdSuggestionsPrivate
{
private:
    IdSuggestions *p;
    KSharedConfigPtr config;
    const KConfigGroup group;

public:

    IdSuggestionsPrivate(IdSuggestions *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), group(config, IdSuggestions::configGroupName) {
        // nothing
    }

    QString normalizeText(const QString& input) const {
        static const QRegExp unwantedChars = QRegExp("[^-_:/=+a-zA-Z0-9]+");
        return EncoderLaTeX::instance()->convertToPlainAscii(input).replace(unwantedChars, QLatin1String(""));
    }

    QStringList authorsLastName(const Entry &entry) const {
        Value value;
        if (entry.contains(Entry::ftAuthor))
            value = entry.value(Entry::ftAuthor);
        if (value.isEmpty() && entry.contains(Entry::ftEditor))
            value = entry.value(Entry::ftEditor);
        if (value.isEmpty())
            return QStringList();

        QStringList result;
        foreach(QSharedPointer<const ValueItem> item, value) {
            QSharedPointer<const Person> person = item.dynamicCast<const Person>();
            if (!person.isNull()) {
                const QString lastName = normalizeText(person->lastName());
                if (!lastName.isEmpty())
                    result << lastName;
            }
        }

        return result;
    }

    inline int extractYear(const Entry &entry) const {
        bool ok = false;
        int result = PlainTextValue::text(entry.value(Entry::ftYear)).toInt(&ok);
        return ok ? result : -1;
    }

    QString translateTitleToken(const Entry &entry, const QString& token, bool removeSmallWords) const {
        struct IdSuggestionTokenInfo tti = p->evalToken(token);
        const QStringList smallWords = QStringList() << QLatin1String("and") << QLatin1String("on") << QLatin1String("in") << QLatin1String("the") << QLatin1String("of") << QLatin1String("at") << QLatin1String("a") << QLatin1String("an") << QLatin1String("with") << QLatin1String("for") << QLatin1String("from");

        QString result;
        bool first = true;
        static const QRegExp sequenceOfSpaces(QLatin1String("\\s+"));
        QStringList titleWords = PlainTextValue::text(entry.value(Entry::ftTitle)).split(sequenceOfSpaces, QString::SkipEmptyParts);
        for (QStringList::ConstIterator it = titleWords.begin(); it != titleWords.end(); ++it) {
            if (first)
                first = false;
            else
                result.append(tti.inBetween);

            QString lowerText = (*it).toLower();
            if (!removeSmallWords || !smallWords.contains(lowerText))
                result.append(normalizeText(*it).left(tti.len));
        }

        if (tti.toUpper)
            result = result.toUpper();
        else if (tti.toLower)
            result = result.toLower();

        return result;
    }

    QString translateAuthorsToken(const Entry &entry, const QString &token, Authors selectAuthors) const {
        struct IdSuggestionTokenInfo ati = p->evalToken(token);
        QString result;
        bool first = true, firstInserted = true;
        QStringList authors = authorsLastName(entry);
        for (QStringList::ConstIterator it = authors.begin(); it != authors.end(); ++it) {
            QString author =  normalizeText(*it).left(ati.len);
            if (selectAuthors == aAll || (selectAuthors == aOnlyFirst && first) || (selectAuthors == aNotFirst && !first)) {
                if (!firstInserted)
                    result.append(ati.inBetween);
                result.append(author);
                firstInserted = false;
            }
            first = false;
        }

        if (ati.toUpper)
            result = result.toUpper();
        else if (ati.toLower)
            result = result.toLower();

        return result;
    }

    QString translateToken(const Entry &entry, const QString &token) const {
        switch (token[0].toAscii()) {
        case 'a': return translateAuthorsToken(entry, token.mid(1), aOnlyFirst);
        case 'A': return translateAuthorsToken(entry, token.mid(1), aAll);
        case 'z': return translateAuthorsToken(entry, token.mid(1), aNotFirst);
        case 'y': {
            int year = extractYear(entry);
            if (year > -1)
                return QString::number(year % 100 + 100).mid(1);
        }
        case 'Y': {
            int year = extractYear(entry);
            if (year > -1)
                return QString::number(year % 10000 + 10000).mid(1);
        }
        case 't': return translateTitleToken(entry, token.mid(1), false);
        case 'T': return translateTitleToken(entry, token.mid(1), true);
        case '"': return token.mid(1);
        }

        return QString::null;
    }

    QString defaultFormatString() const {
        return group.readEntry(keyDefaultFormatString, defaultDefaultFormatString);
    }

    QStringList formatStringList() const {
        return group.readEntry(keyFormatStringList, defaultFormatStringList);
    }
};

const QString IdSuggestions::keyDefaultFormatString = QLatin1String("DefaultFormatString");
const QString IdSuggestions::defaultDefaultFormatString = QString::null;
const QString IdSuggestions::keyFormatStringList = QLatin1String("FormatStringList");
const QStringList IdSuggestions::defaultFormatStringList = QStringList() << QLatin1String("A") << QLatin1String("A2|y") << QLatin1String("A3|y") << QLatin1String("A4|y|\":|T5") << QLatin1String("al|\":|T") << QLatin1String("al|y") << QLatin1String("al|Y") << QLatin1String("Al\"-|\"-|y") << QLatin1String("Al\"+|Y") << QLatin1String("al|y|T") << QLatin1String("al|Y|T3") << QLatin1String("al|Y|T3l") << QLatin1String("a|\":|Y|\":|T1") << QLatin1String("a|y") << QLatin1String("A|\":|Y");
const QString IdSuggestions::configGroupName = QLatin1String("IdSuggestions");

IdSuggestions::IdSuggestions()
        : d(new IdSuggestionsPrivate(this))
{
    // nothing
}

IdSuggestions::~IdSuggestions()
{
    delete d;
}

QString IdSuggestions::formatId(const Entry &entry, const QString &formatStr) const
{
    QString id;
    QStringList tokenList = formatStr.split(QLatin1Char('|'), QString::SkipEmptyParts);
    foreach(const QString &token, tokenList) {
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
    foreach(const QString &formatString, formatStrings) {
        result << formatId(entry, formatString);
    }
    return result;
}

QStringList IdSuggestions::formatStrToHuman(const QString &formatStr) const
{
    QStringList result;
    QStringList tokenList = formatStr.split(QLatin1Char('|'), QString::SkipEmptyParts);
    foreach(const QString &token, tokenList) {
        QString text;
        if (token[0] == 'a' || token[0] == 'A' || token[0] == 'z') {
            struct IdSuggestionTokenInfo info = evalToken(token.mid(1));
            if (token[0] == 'a') text.append(i18n("First author only"));
            else if (token[0] == 'z') text.append(i18n("All but first author"));
            else text.append(i18n("All authors"));

            int n = info.len;
            if (info.len < 0x00ffffff) text.append(i18np(", but only first letter of each last name", ", but only first %1 letters of each last name", n));
            if (info.toUpper) text.append(i18n(", in upper case"));
            else if (info.toLower) text.append(i18n(", in lower case"));
            if (info.inBetween != QString::null) text.append(i18n(", with '%1' in between", info.inBetween));
        } else if (token[0] == 'y') text.append(i18n("Year (2 digits)"));
        else if (token[0] == 'Y') text.append(i18n("Year (4 digits)"));
        else if (token[0] == 't' || token[0] == 'T') {
            struct IdSuggestionTokenInfo info = evalToken(token.mid(1));
            text.append(i18n("Title"));
            int n = info.len;
            if (info.len < 0x00ffffff) text.append(i18np(", but only first letter of each word", ", but only first %1 letters of each word", n));
            if (info.toUpper) text.append(i18n(", in upper case"));
            else if (info.toLower) text.append(i18n(", in lower case"));
            if (info.inBetween != QString::null) text.append(i18n(", with '%1' in between", info.inBetween));
            if (token[0] == 'T') text.append(i18n(", small words removed"));
        } else if (token[0] == '"') text.append(i18n("Text: '%1'", token.mid(1)));
        else text.append("?");

        result.append(text);
    }

    return result;
}

struct IdSuggestions::IdSuggestionTokenInfo IdSuggestions::evalToken(const QString &token) const {
        int pos = 0;
        struct IdSuggestionTokenInfo result;
        result.len = 0x00ffffff;
        result.toLower = false;
        result.toUpper = false;
        result.inBetween = QString::null;

        if (token.length() > pos) {
            int dv = token[pos].digitValue();
            if (dv > -1) {
                result.len = dv;
                ++pos;
            }
        }

        if (token.length() > pos) {
            result.toLower = token[pos] == 'l';
            result.toUpper = token[pos] == 'u';
            if (result.toUpper || result.toLower)
                ++pos;
        }

        if (token.length() > pos + 1 && token[pos] == '"')
            result.inBetween = token.mid(pos + 1);

        return result;
    }

