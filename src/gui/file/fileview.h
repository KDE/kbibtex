/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_FILEVIEW_H
#define KBIBTEX_GUI_FILEVIEW_H

#include <QWidget>

#include <widgets/FilterBar>
#include <file/BasicFileView>
#include <Element>

#include "kbibtexgui_export.h"

class QAbstractButton;
class QDialogButtonBox;

class ValueListModel;
class Clipboard;
class ElementEditor;
class ElementEditorDialog;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT FileView : public BasicFileView
{
    Q_OBJECT
public:
    FileView(const QString &name, QWidget *parent);

    const QList<QSharedPointer<Element> > &selectedElements() const;
    const QSharedPointer<Element> currentElement() const;
    QSharedPointer<Element> currentElement();
    QSharedPointer<Element> elementAt(const QModelIndex &index);

    void setReadOnly(bool isReadOnly = true);
    bool isReadOnly() const;

    ValueListModel *valueListModel(const QString &field);

    void setFilterBar(FilterBar *filterBar);
    void setClipboard(Clipboard *clipboard);

signals:
    void selectedElementsChanged();
    void currentElementChanged(QSharedPointer<Element>, const File *);
    void elementExecuted(QSharedPointer<Element>);
    void modified(bool);

public slots:
    void viewCurrentElement();
    void viewElement(const QSharedPointer<Element>);
    void editCurrentElement();
    bool editElement(QSharedPointer<Element>);
    void setSelectedElement(QSharedPointer<Element>);
    void selectionDelete();
    void externalModification();
    void setFilterBarFilter(const SortFilterFileModel::FilterQuery &);

protected:
    bool m_isReadOnly;

    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

protected slots:
    void itemActivated(const QModelIndex &index);
    void dialogButtonClicked(QAbstractButton *);

private:
    enum class DialogType { View, Edit };

    QSharedPointer<Element> m_current;
    QList<QSharedPointer<Element> > m_selection;
    FilterBar *m_filterBar;
    Clipboard *m_clipboard;
    QWidget *m_lastEditorPage;

    ElementEditorDialog *m_elementEditorDialog;
    ElementEditor *m_elementEditor;
    QDialogButtonBox *m_dbb;

    void prepareEditorDialog(DialogType dialogType);
};


#endif // KBIBTEX_GUI_FILEVIEW_H
