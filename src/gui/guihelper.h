/*****************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#ifndef KBIBTEX_GUI_GUIHELPER_H
#define KBIBTEX_GUI_GUIHELPER_H

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
    /**
     * Given a model containing strings in the first column attached
     * to the root node, search all those strings for equivalence to
     * a given string. The test for equivalence is case-insensitive
     * and the role how this string is retrieved from the model can
     * be chosen.
     * If the string was found, the row number (0 to rowCount()-1) is
     * returned. If the string was not found or any other error
     * condition occurred, a negative value is returned.
     *
     * @param model model containing strings to test for case-insensitive equivalence
     * @param value value to search for in the model
     * @param role role used to retrieve string data from the model
     * @return row of value's occurrence or a negative value if not found or any other error
     */
    static int selectValue(QAbstractItemModel *model, const QString &value, int role = Qt::DisplayRole);
    static int selectValue(QAbstractItemModel *model, const int value, int role = Qt::DisplayRole);
};

#endif // KBIBTEX_GUI_GUIHELPER_H
