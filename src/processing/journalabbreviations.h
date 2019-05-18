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

#ifndef KBIBTEX_PROCESSING_JOURNALABBREVIATIONS_H
#define KBIBTEX_PROCESSING_JOURNALABBREVIATIONS_H

#include <QString>

#ifdef HAVE_KF5
#include "kbibtexprocessing_export.h"
#endif // HAVE_KF5

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROCESSING_EXPORT JournalAbbreviations
{
public:
    static const JournalAbbreviations &instance();

    QString toShortName(const QString &longName) const;
    QString toLongName(const QString &shortName) const;

protected:
    explicit JournalAbbreviations();
    ~JournalAbbreviations();

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_PROCESSING_JOURNALABBREVIATIONS_H
