/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_GUI_SETTINGSFILEEXPORTERPDFPSWIDGET_H
#define KBIBTEX_GUI_SETTINGSFILEEXPORTERPDFPSWIDGET_H

#include <preferences/SettingsAbstractWidget>

#include "kbibtexgui_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT SettingsFileExporterPDFPSWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsFileExporterPDFPSWidget(QWidget *parent);
    ~SettingsFileExporterPDFPSWidget() override;

    QString label() const override;
    QIcon icon() const override;

public slots:
    void loadState() override;
    void saveState() override;
    void resetToDefaults() override;

private:
    class SettingsFileExporterPDFPSWidgetPrivate;
    SettingsFileExporterPDFPSWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSFILEEXPORTERPDFPSWIDGET_H
