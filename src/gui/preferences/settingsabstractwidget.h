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

#ifndef KBIBTEX_GUI_SETTINGSABSTRACTWIDGET_H
#define KBIBTEX_GUI_SETTINGSABSTRACTWIDGET_H

#include <QAbstractItemModel>
#include <QWidget>
#include <QIcon>

#include "kbibtexgui_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT SettingsAbstractWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsAbstractWidget(QWidget *parent);
    // virtual ~SettingsAbstractWidget() { /* nothing */ };

    virtual int eventId() const;
    virtual QString label() const = 0;
    virtual QIcon icon() const = 0;

signals:
    void changed();

public slots:
    virtual void loadState() = 0;
    /**
     * Save the state of this settings widget into the configuration settings,
     * usually using the Preferences class.
     * @return true if saved settings differed from previous values, false otherwise or under error conditions
     */
    virtual bool saveState() = 0;
    virtual void resetToDefaults() = 0;
};

#endif // KBIBTEX_GUI_SETTINGSABSTRACTWIDGET_H
