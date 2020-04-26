/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2017 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_FILEDELEGATE_H
#define KBIBTEX_GUI_FILEDELEGATE_H

#include <QStyledItemDelegate>

#include "kbibtexgui_export.h"

class KBIBTEXGUI_EXPORT FileDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit FileDelegate(QWidget *parent = nullptr)
            : QStyledItemDelegate(parent) {
        /* nothing */
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // KBIBTEX_GUI_FILEDELEGATE_H
