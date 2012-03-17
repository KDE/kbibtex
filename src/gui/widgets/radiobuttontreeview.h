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

#ifndef KBIBTEX_GUI_RADIOBUTTONTREEVIEW_H
#define KBIBTEX_GUI_RADIOBUTTONTREEVIEW_H

#include <QTreeView>
#include <QStyledItemDelegate>

class QMouseEvent;
class QKeyEvent;

static const int RadioSelectedRole = Qt::UserRole + 102;
static const int IsRadioRole = Qt::UserRole + 103;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *
 * This class is a refinement of QTreeView, as it adds support
 * for radio buttons for elements in the view.
 * To use this view, set RadioButtonItemDelegate as the item delegate
 * and use a model that respondes to the roles IsRadioRole and
 * RadioSelectedRole. The role IsRadioRole returns a boolean value
 * packed in a QVariant if a QModelIndex should have a radio button,
 * RadioSelectedRole is boolean value as well, determining if a
 * radio button is selected or not.
 * This class will take care that if a QModelIndex receives a mouse
 * click or a space key press, RadioSelectedRole will be set true for
 * this QModelIndex and all sibling indices will be set to false.
 */
class RadioButtonTreeView : public QTreeView
{
public:
    RadioButtonTreeView(QWidget *parent);

protected:
    void mouseReleaseEvent(QMouseEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    void switchRadioFlag(QModelIndex &index);
};

#endif // KBIBTEX_GUI_RADIOBUTTONTREEVIEW_H
