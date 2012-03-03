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

#ifndef KBIBTEX_PROC_IDSUGGESTIONS_H
#define KBIBTEX_PROC_IDSUGGESTIONS_H

#include "kbibtexproc_export.h"

#include "entry.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROC_EXPORT IdSuggestions
{
public:
    enum Authors {aAll, aOnlyFirst, aNotFirst};

    struct IdSuggestionTokenInfo {
        unsigned int len;
        bool toLower;
        bool toUpper;
        QString inBetween;
    };

    static const QString keyFormatStringList;
    static const QString keyDefaultFormatString;
    static const QStringList defaultFormatStringList;
    static const QString defaultDefaultFormatString;
    static const QString configGroupName;

    IdSuggestions();
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

protected:

    struct IdSuggestionTokenInfo evalToken(const QString &token) const;

private:
    class IdSuggestionsPrivate;
    IdSuggestionsPrivate *d;
};

#endif // KBIBTEX_PROC_IDSUGGESTIONS_H
