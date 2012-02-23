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
#ifndef KBIBTEX_GUI_BIBTEXFILEVIEW_H
#define KBIBTEX_GUI_BIBTEXFILEVIEW_H

#include <QTreeView>

#include "kbibtexgui_export.h"

class BibTeXFileModel;
class QSortFilterProxyModel;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXFileView : public QTreeView
{
    Q_OBJECT
public:
    BibTeXFileView(const QString &name, QWidget *parent = 0);
    ~BibTeXFileView();

    virtual void setModel(QAbstractItemModel *model);
    BibTeXFileModel *bibTeXModel();
    QSortFilterProxyModel *sortFilterProxyModel();

protected:
    virtual void keyPressEvent(QKeyEvent *event);

protected slots:
    void columnMoved();
    void columnResized(int column, int oldSize, int newSize);

private:
    class BibTeXFileViewPrivate;
    BibTeXFileViewPrivate *d;

private slots:
    void headerActionToggled();
    void headerResetToDefaults();
    void headerAdjustColumnWidths();
    void sort(int, Qt::SortOrder);
};


#endif // KBIBTEX_GUI_BIBTEXFILEVIEW_H
