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

#ifndef KBIBTEX_GUI_FILTERBAR_H
#define KBIBTEX_GUI_FILTERBAR_H

#include <QWidget>

#include <file/SortFilterFileModel>

#include "kbibtexgui_export.h"

/**
@author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT FilterBar : public QWidget
{
    Q_OBJECT
public:
    explicit FilterBar(QWidget *parent);
    ~FilterBar() override;

    SortFilterFileModel::FilterQuery filter();

    void setPlaceholderText(const QString &msg);

public slots:
    /**
     * Set the filter criteria to be both shown in this filter bar
     * and applied to the list of elements.
     * @param fq query data structure to be used
     */
    void setFilter(const SortFilterFileModel::FilterQuery &fq);

signals:
    void filterChanged(const SortFilterFileModel::FilterQuery &);

private:
    class FilterBarPrivate;
    FilterBarPrivate *d;

private slots:
    void comboboxStatusChanged();
    void resetState();
    void userPressedEnter();
    void publishFilter();
    void buttonHeight();
};

#endif // KBIBTEX_GUI_FILTERBAR_H
