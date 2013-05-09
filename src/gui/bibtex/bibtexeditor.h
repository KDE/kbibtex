/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include "kbibtexgui_export.h"

#include "filterbar.h"
#include "bibtexfileview.h"
#include "element.h"

class ValueListModel;
class ElementEditor;
class ElementEditorDialog;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXEditor : public BibTeXFileView
{
    Q_OBJECT
public:
    BibTeXEditor(const QString &name, QWidget *parent);

    const QList<QSharedPointer<Element> > &selectedElements() const;
    const QSharedPointer<Element> currentElement() const;
    QSharedPointer<Element> currentElement();
    QSharedPointer<Element> elementAt(const QModelIndex &index);

    void setReadOnly(bool isReadOnly = true);
    bool isReadOnly() const;

    ValueListModel *valueListModel(const QString &field);

    void setFilterBar(FilterBar *filterBar);

signals:
    void selectedElementsChanged();
    void currentElementChanged(QSharedPointer<Element>, const File *);
    void elementExecuted(QSharedPointer<Element>);
    void editorMouseEvent(QMouseEvent *);
    void editorDragEnterEvent(QDragEnterEvent *);
    void editorDragMoveEvent(QDragMoveEvent *);
    void editorDropEvent(QDropEvent *);
    void modified();

public slots:
    void viewCurrentElement();
    void viewElement(const QSharedPointer<Element>);
    void editCurrentElement();
    bool editElement(QSharedPointer<Element>);
    void setSelectedElements(QList<QSharedPointer<Element> > &);
    void setSelectedElement(QSharedPointer<Element>);
    void selectionDelete();
    void externalModification();
    void setFilterBarFilter(SortFilterBibTeXFileModel::FilterQuery);

protected:
    bool m_isReadOnly;

    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void mouseMoveEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

protected slots:
    void itemActivated(const QModelIndex &index);

private:
    enum DialogType { DialogTypeView, DialogTypeEdit };

    QSharedPointer<Element> m_current;
    QList<QSharedPointer<Element> > m_selection;
    FilterBar *m_filterBar;
    QWidget *m_lastEditorPage;

    ElementEditorDialog *m_elementEditorDialog;
    ElementEditor *m_elementEditor;

    void prepareEditorDialog(DialogType dialogType);
};


#endif // KBIBTEX_GUI_BIBTEXEDITOR_H
