/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_PROCESSING_IDSUGGESTIONS_H
#define KBIBTEX_PROCESSING_IDSUGGESTIONS_H

#include <Entry>

#ifdef HAVE_KF5
#include "kbibtexprocessing_export.h"
#endif // HAVE_KF5

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROCESSING_EXPORT IdSuggestions
{
public:
    enum CaseChange {ccNoChange = 0, ccToUpper = 1, ccToLower = 2, ccToCamelCase = 3};

    struct IdSuggestionTokenInfo {
        int len;
        int startWord, endWord;
        bool lastWord;
        CaseChange caseChange;
        QString inBetween;
    };

    IdSuggestions();
    IdSuggestions(const IdSuggestions &) = delete;
    IdSuggestions &operator= (const IdSuggestions &other) = delete;
    ~IdSuggestions();

    QString formatId(const Entry &entry, const QString &formatStr) const;
    QString defaultFormatId(const Entry &entry) const;
    bool hasDefaultFormat() const;

    /**
      * Apply the default formatting string to the entry.
      * If no default formatting string is set, the entry
      * will stay untouched and the function return false.
      * If the formatting string is set, the entry's id
      * will be changed accordingly and the function returns true.
      *
      * @param entry entry where the id has to be set
      * @return true if the id was set, false otherwise
      */
    bool applyDefaultFormatId(Entry &entry) const;

    QStringList formatIdList(const Entry &entry) const;

    QStringList formatStrToHuman(const QString &formatStr) const;

    static QString formatAuthorRange(int minValue, int maxValue, bool lastAuthor);

protected:
    struct IdSuggestionTokenInfo evalToken(const QString &token) const;

private:
    class IdSuggestionsPrivate;
    IdSuggestionsPrivate *d;
};

#endif // KBIBTEX_PROCESSING_IDSUGGESTIONS_H
