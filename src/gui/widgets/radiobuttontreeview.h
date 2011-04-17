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

#ifndef KBIBTEX_GUI_RADIOBUTTONTREEVIEW_H
#define KBIBTEX_GUI_RADIOBUTTONTREEVIEW_H

#include <QTreeView>
#include <QStyledItemDelegate>

class QMouseEvent;
class QKeyEvent;

const int RadioSelectedRole = Qt::UserRole + 102;
const int IsRadioRole = Qt::UserRole + 103;


class RadioButtonItemDelegate : public QStyledItemDelegate
{
public:
    RadioButtonItemDelegate(QObject *p);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


class RadioButtonTreeView : public QTreeView
{
public:
    RadioButtonTreeView(QWidget *parent);

    virtual void reset();

protected:
    void mouseReleaseEvent(QMouseEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    void switchRadioFlag(QModelIndex &index);
};

#endif // KBIBTEX_GUI_RADIOBUTTONTREEVIEW_H
