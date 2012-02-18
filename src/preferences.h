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

#ifndef KBIBTEX_PREFERENCES_H
#define KBIBTEX_PREFERENCES_H

#include <QString>
#include <QStringList>

#include <KLocale>

namespace Preferences
{
static const QString groupColor = QLatin1String("Color Labels");
static const QString keyColorCodes = QLatin1String("colorCodes");
static const QStringList defaultColorCodes = QStringList() << QLatin1String("#cc3300") << QLatin1String("#0033ff") << QLatin1String("#009966");
static const QString keyColorLabels = QLatin1String("colorLabels");
static const QStringList defaultcolorLabels = QStringList() << I18N_NOOP("Important") << I18N_NOOP("Unread") << I18N_NOOP("Read");
}

#endif // KBIBTEX_PREFERENCES_H
