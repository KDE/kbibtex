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

#include <QDebug>

#include "element.h"
#include "macro.h"
#include "entry.h"
#include "preamble.h"
#include "comment.h"

Element::Element()
{
    static int idCounter = 0;
    uniqueId = ++idCounter;
}

QDebug operator<<(QDebug dbg, const Element &element) {
    if (Macro::isMacro(element))
        return operator<<(dbg, *dynamic_cast<const Macro *>(&element));
    else if (Entry::isEntry(element))
        return operator<<(dbg, *dynamic_cast<const Entry *>(&element));
    else if (Preamble::isPreamble(element))
        return operator<<(dbg, *dynamic_cast<const Preamble *>(&element));
    else if (Comment::isComment(element))
        return operator<<(dbg, *dynamic_cast<const Comment *>(&element));
    else {
        dbg.nospace() << "Element object of unsupported type";
        return dbg;
    }
}
