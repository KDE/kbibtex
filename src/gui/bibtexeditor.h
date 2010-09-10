/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#ifndef KBIBTEX_GUI_BIBTEXEDITOR_H
#define KBIBTEX_GUI_BIBTEXEDITOR_H

#include <QWidget>

#include <kbibtexgui_export.h>

#include <bibtexfileview.h>
#include <element.h>

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXEditor : public BibTeXFileView
{
    Q_OBJECT
public:
    BibTeXEditor(QWidget *parent);

    const QList<Element*>& selectedElements() const;
    const Element* currentElement() const;
    Element* currentElement();

signals:
    void selectedElementsChanged();
    void currentElementChanged(Element*, const File *);
    void elementExecuted(Element*);
    void keyPressed(QKeyEvent *);

public slots:
    void viewCurrentElement();
    void viewElement(const Element*);
    void editCurrentElement();
    void editElement(Element*);
    void setSelectedElements(QList<Element*>&);
    void setSelectedElement(Element*);

protected:
    void keyPressEvent(QKeyEvent *event);
    void currentChanged(const QModelIndex & current, const QModelIndex & previous);
    void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

protected slots:
    void itemActivated(const QModelIndex & index);

private:
    Element* m_current;
    QList<Element*> m_selection;
};


#endif // KBIBTEX_GUI_BIBTEXEDITOR_H
