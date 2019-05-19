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

#ifndef KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H
#define KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H

#include <NotificationHub>
#include <preferences/SettingsAbstractWidget>

#include "kbibtexgui_export.h"

class QSignalMapper;

class KActionMenu;

class FileView;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT SettingsColorLabelWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsColorLabelWidget(QWidget *parent);
    ~SettingsColorLabelWidget() override;

    QString label() const override;
    QIcon icon() const override;

public slots:
    void loadState() override;
    void saveState() override;
    void resetToDefaults() override;

private slots:
    void addColor();
    void removeColor();
    void updateRemoveButtonStatus();

private:
    class Private;
    Private *d;
};

/**
@author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT ColorLabelContextMenu : public QObject, private NotificationListener
{
    Q_OBJECT

public:
    explicit ColorLabelContextMenu(FileView *widget);
    ~ColorLabelContextMenu() override;

    KActionMenu *menuAction();
    void setEnabled(bool);

    void notificationEvent(int eventId) override;

private slots:
    void colorActivated(const QString &colorString);

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H
