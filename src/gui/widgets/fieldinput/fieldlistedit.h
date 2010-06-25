/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#ifndef KBIBTEX_GUI_FIELDLISTEDIT_H
#define KBIBTEX_GUI_FIELDLISTEDIT_H

#include <QScrollArea>

#include <value.h>

/**
@author Thomas Fischer
*/
class FieldListEdit : public QScrollArea
{
    Q_OBJECT

public:
    FieldListEdit(KBibTeX::TypeFlags typeFlags = KBibTeX::tfSource, QWidget *parent = NULL);

    void setValue(const Value& value);
    void applyTo(Value& value) const;

    void clear();
    void setReadOnly(bool isReadOnly);

//public slots:
    //  void reset();

protected:
    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void lineAdd();
    void lineRemove(QWidget * widget);
    void lineGoDown(QWidget * widget);
    void lineGoUp(QWidget * widget);

private:
    class FieldListEditPrivate;
    FieldListEditPrivate *d;
};

#endif // KBIBTEX_GUI_FIELDLISTEDIT_H
