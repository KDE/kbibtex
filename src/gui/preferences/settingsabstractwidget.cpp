/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QAbstractItemModel>

#include <KComboBox>

#include "settingsabstractwidget.h"

SettingsAbstractWidget::SettingsAbstractWidget(QWidget *parent)
        : QWidget(parent)
{
    // nothing
}

void SettingsAbstractWidget::selectValue(KComboBox *comboBox, const QString &value, int role)
{
    QAbstractItemModel *model = comboBox->model();
    int row = 0;
    QModelIndex index;
    const QString lowerValue = value.toLower();
    while ((index = model->index(row, 0, QModelIndex())) != QModelIndex()) {
        QString line = model->data(index, role).toString();
        if (line.toLower() == lowerValue) {
            comboBox->setCurrentIndex(row);
            break;
        }
        ++row;
    }
}

