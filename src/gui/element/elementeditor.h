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

#ifndef KBIBTEX_GUI_DIALOGS_ELEMENTSEDITOR_H
#define KBIBTEX_GUI_DIALOGS_ELEMENTSEDITOR_H

#include <kbibtexgui_export.h>

#include <QWidget>

class Element;
class File;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT ElementEditor : public QWidget
{
    Q_OBJECT
public:
    ElementEditor(const Element *element, const File *file, QWidget *parent);
    ElementEditor(Element *element, const File *file, QWidget *parent);
    ~ElementEditor();

    void setReadOnly(bool isReadOnly = true);
    bool elementChanged();
    bool elementUnapplied();

    int currentTab();
    void setCurrentTab(int tabIndex);

signals:
    void modified(bool);

public slots:
    void apply();
    void reset();

private slots:
    void tabChanged();
    void checkBibTeX();
    void childModified(bool);

private:
    class ElementEditorPrivate;
    ElementEditorPrivate *d;
};

#endif // KBIBTEX_GUI_DIALOGS_ELEMENTSEDITOR_H
