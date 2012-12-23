/*****************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#ifndef GUI_HELPER_H
#define GUI_HELPER_H

#include <QAbstractItemModel>
#include <QPoint>
#include <QSize>

#include "kbibtexgui_export.h"

class QPainter;

/**
 * @author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT GUIHelper
{
public:
    static int selectValue(QAbstractItemModel *model, const QString &value, int role = Qt::DisplayRole);
    static void paintStars(QPainter *painter, int numActiveStars, int numTotalStars, const QSize &maxSize, const QPoint &pos);
};


#endif // GUI_HELPER_H
