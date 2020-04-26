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

#ifndef KBIBTEX_GUI_SETTINGSGENERALWIDGET_H
#define KBIBTEX_GUI_SETTINGSGENERALWIDGET_H

#include <preferences/SettingsAbstractWidget>

#include "kbibtexgui_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT SettingsGeneralWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsGeneralWidget(QWidget *parent);
    ~SettingsGeneralWidget() override;

    QString label() const override;
    QIcon icon() const override;

public slots:
    void loadState() override;
    bool saveState() override;
    void resetToDefaults() override;

private:
    class SettingsGeneralWidgetPrivate;
    SettingsGeneralWidgetPrivate *d;
};


#endif // KBIBTEX_GUI_SETTINGSGENERALWIDGET_H
