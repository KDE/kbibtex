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
#ifndef KBIBTEX_GUI_MENULINEEDIT_H
#define KBIBTEX_GUI_MENULINEEDIT_H

#include <QFrame>

#include "kbibtexgui_export.h"

class QMenu;
class QIcon;

/**
@author Thomas Fischer
 */
class MenuLineEdit : public QFrame
{
    Q_OBJECT

public:
    /**
     * @brief MenuLineConfigurationChangedEvent
     * Event id for use with @c NotificationHub.
     * All MenuLineWidgets register to this event.
     */
    static const int MenuLineConfigurationChangedEvent;
    /**
     * @brief keyLimitKeyboardTabStops
     * Configuration key in group "UserInterface" to access
     * the setting for limited keyboard tab stops.
     */
    static const QString keyLimitKeyboardTabStops;

    MenuLineEdit(bool isMultiLine, QWidget *parent);
    ~MenuLineEdit() override;

    void setMenu(QMenu *menu);
    virtual void setReadOnly(bool);
    QString text() const;
    void setText(const QString &);
    void setIcon(const QIcon &icon);
    void setFont(const QFont &font);
    void setButtonToolTip(const QString &);
    void setChildAcceptDrops(bool acceptDrops);

    QWidget *buddy();
    void prependWidget(QWidget *widget);
    void appendWidget(QWidget *widget);
    void setInnerWidgetsTransparency(bool makeInnerWidgetsTransparent);

    bool isModified() const;
    void setCompletionItems(const QStringList &items);

protected:
    void focusInEvent(QFocusEvent *event) override;

signals:
    void textChanged(const QString &);

private slots:
    void slotTextChanged();
    void slotVerticallyStretchButtons();

private:
    class MenuLineEditPrivate;
    MenuLineEditPrivate *const d;
};


#endif // KBIBTEX_GUI_MENULINEEDIT_H
