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

#include "guihelper.h"

#include <QPainter>
#include <QPixmap>

#include <KIconLoader>

#include "logging_gui.h"

int GUIHelper::selectValue(QAbstractItemModel *model, const QString &value, int role)
{
    int row = 0;
    QModelIndex index;
    const QString lowerValue = value.toLower();
    while (row < model->rowCount() && (index = model->index(row, 0, QModelIndex())) != QModelIndex()) {
        QString line = model->data(index, role).toString();
        if (line.toLower() == lowerValue)
            return row;
        ++row;
    }

    qCWarning(LOG_KBIBTEX_GUI) << "Could not find matching row in model for value " << value << " in role" << role;

    return -1;
}

int GUIHelper::selectValue(QAbstractItemModel *model, const int value, int role)
{
    int row = 0;
    QModelIndex index;
    while (row < model->rowCount() && (index = model->index(row, 0, QModelIndex())) != QModelIndex()) {
        const int lineValue = model->data(index, role).toInt();
        if (lineValue == value)
            return row;
        ++row;
    }

    qCWarning(LOG_KBIBTEX_GUI) << "Could not find matching row in model for value " << value << " in role" << role;

    return -1;
}
