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
#ifndef KBIBTEX_GUI_FILTERBAR_H
#define KBIBTEX_GUI_FILTERBAR_H

#include <kbibtexgui_export.h>

#include <QWidget>

#include "bibtexfilemodel.h"

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FilterBar : public QWidget
{
    Q_OBJECT
public:
    FilterBar(QWidget *parent);
    ~FilterBar();

    SortFilterBibTeXFileModel::FilterQuery filter();

public slots:
    void clearFilter();
    void setFilter(SortFilterBibTeXFileModel::FilterQuery);

signals:
    void filterChanged(SortFilterBibTeXFileModel::FilterQuery);

private:
    class FilterBarPrivate;
    FilterBarPrivate *d;

private slots:
    void lineeditTextChanged();
    void comboboxStatusChanged();
    void lineeditReturnPressed();
    void timerTriggered();
};

#endif // KBIBTEX_GUI_FILTERBAR_H
