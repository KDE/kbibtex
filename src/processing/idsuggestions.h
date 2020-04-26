/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
    enum class CaseChange {None, ToUpper, ToLower, ToCamelCase};

    struct IdSuggestionTokenInfo {
        int len;
        int startWord, endWord;
        bool lastWord;
        CaseChange caseChange;
        QString inBetween;
    };

    static QString formatId(const Entry &entry, const QString &formatStr);
    static QString defaultFormatId(const Entry &entry);
    static bool hasDefaultFormat();

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
    static bool applyDefaultFormatId(Entry &entry);

    static QStringList formatIdList(const Entry &entry);

    static QStringList formatStrToHuman(const QString &formatStr);

    static QString formatAuthorRange(int minValue, int maxValue, bool lastAuthor);

    static struct IdSuggestionTokenInfo evalToken(const QString &token);

private:
    class IdSuggestionsPrivate;
};

#endif // KBIBTEX_PROCESSING_IDSUGGESTIONS_H
