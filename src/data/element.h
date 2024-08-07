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

#ifndef KBIBTEX_DATA_ELEMENT_H
#define KBIBTEX_DATA_ELEMENT_H

#include <File>

#ifdef HAVE_KF
#include "kbibtexdata_export.h"
#endif // HAVE_KF

/**
 * Base class for bibliographic elements in a BibTeX file.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT Element
{
public:
    Element();
    virtual ~Element() {
        /* nothing */
    }

    bool operator<(const Element &other) const;

private:
    int uniqueId;
};

KBIBTEXDATA_EXPORT QDebug operator<<(QDebug dbg, const Element &element);

#endif // KBIBTEX_DATA_ELEMENT_H
