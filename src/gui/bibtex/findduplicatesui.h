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

#ifndef KBIBTEX_GUI_FINDDUPLICATES_H
#define KBIBTEX_GUI_FINDDUPLICATES_H

#include "kbibtexio_export.h"

#include <QObject>
#include <QTreeView>

namespace KParts
{
class Part;
}
class KXMLGUIClient;

class BibTeXEditor;


/*
class RadioButtonTreeView :public QTreeView{
    Q_OBJECT

public:
    RadioButtonTreeView(QWidget *parent)
        :QTreeView(parent){
        connect(this,SIGNAL(clicked(QModelIndex)),this,SLOT(indexActivated(QModelIndex)));
     }

private slots:
    void indexActivated(const QModelIndex & index){
        model()->setData(index,QVariant::fromValue(true), Qt::UserRole + 102);
    }
};
*/


class KBIBTEXIO_EXPORT FindDuplicatesUI : public QObject
{
    Q_OBJECT

public:
    FindDuplicatesUI(KParts::Part *part, BibTeXEditor *bibTeXEditor);
    // TODO

private slots:
    void slotFindDuplicates();

private:
    class FindDuplicatesUIPrivate;
    FindDuplicatesUIPrivate *d;
};

#endif // KBIBTEX_GUI_FINDDUPLICATES_H
