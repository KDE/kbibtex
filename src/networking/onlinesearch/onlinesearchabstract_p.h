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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHABSTRACT_P_H
#define KBIBTEX_NETWORKING_ONLINESEARCHABSTRACT_P_H

#ifdef HAVE_QTWIDGETS
#include "onlinesearchabstract.h"

#include <QWidget>

#include <KSharedConfig>

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class OnlineSearchAbstract::Form::Private
{
public:
    explicit Private();

    KSharedConfigPtr config;

    static QStringList authorLastNames(const Entry &entry);
    static QString guessFreeText(const Entry &entry);
};

#endif // HAVE_QTWIDGETS

#endif // KBIBTEX_NETWORKING_ONLINESEARCHABSTRACT_P_H
