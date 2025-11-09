/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_ELEMENTEDITOR_P_H
#define KBIBTEX_GUI_ELEMENTEDITOR_P_H

#include <KSharedConfig>

#include <Entry>
#include <Comment>
#include <Macro>
#include <Preamble>
#include <Element>

#include "elementeditor.h"

class QPushButton;

class ElementWidget;
class ReferenceWidget;
class HidingTabWidget;
class SourceWidget;
class FilesWidget;

class ElementEditor::ElementEditorPrivate : public ElementEditor::ApplyElementInterface
{
private:
    const File *file;

    // Whenever an ElementEditor is reset to a given Element, this Element will be
    // remembered in one of the following four variables: internalEntry, internalMacro,
    // internalPreamble, and internalComment and henceforth call 'current internal element'
    // At any point in time, at most one of the four refers to a valid Element instance,
    // but during initialization, all four may be Null
    // Those four variables are used to apply the editor's state to the original Element,
    // or to restore the original state if the user requests to revert any intermediate
    // changes made in the editor
    QSharedPointer<Entry> internalEntry;
    QSharedPointer<Macro> internalMacro;
    QSharedPointer<Preamble> internalPreamble;
    QSharedPointer<Comment> internalComment;

    ElementEditor *p;
    ElementWidget *previousWidget;
    ReferenceWidget *referenceWidget;
    QPushButton *buttonCheckWithBibTeX;

    /// Settings management through a push button with menu
    KSharedConfigPtr config;
    QPushButton *buttonOptions;
    QAction *actionForceShowAllWidgets, *actionLimitKeyboardTabStops;

public:
    typedef QVector<ElementWidget *> WidgetList;
    QSharedPointer<Element> element;
    HidingTabWidget *tab;
    WidgetList widgets;
    SourceWidget *sourceWidget;
    FilesWidget *filesWidget;
    bool elementChanged, elementUnapplied;

    ElementEditorPrivate(bool scrollable, ElementEditor *parent);
    ~ElementEditorPrivate() override;
    void clearWidgets();
    void setElement(QSharedPointer<Element> element, const File *file);
    void addTabWidgets();
    void createGUI(bool scrollable);
    void updateTabVisibility();
    /**
     * If this element editor makes use of a reference widget
     * (e.g. where entry type and entry id/macro key can be edited),
     * then return the current value of the entry id/macro key
     * editing widget.
     * Otherwise, return an empty string.
     *
     * @return Current value of entry id/macro key if any, otherwise empty string
     */
    QString currentId() const;

    void setCurrentId(const QString &newId);
    /**
    * Return the current File object set for this element editor.
    * May be NULL if nothing has been set or if it has been cleared.
    *
    * @return Current File object, may be nullptr
    */
    const File *currentFile() const;

    void apply();
    void apply(QSharedPointer<Element> element) override;

    bool validate(QWidget **widgetWithIssue, QString &message) const override;

    void reset();
    void reset(QSharedPointer<const Element> element);
    void setReadOnly(bool isReadOnly);
    void updateReqOptWidgets();
    void limitKeyboardTabStops();
    void switchTo(QWidget *futureTab);
    void checkBibTeX();
    void setModified(bool newIsModified);
    void referenceWidgetSetEntryIdByDefault();
};

#endif // KBIBTEX_GUI_ELEMENTEDITOR_P_H
