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

#include <QTabWidget>
#include <QLayout>

#include <entry.h>
#include <comment.h>
#include <macro.h>
#include <preamble.h>
#include <element.h>
#include "elementwidgets.h"
#include "elementeditor.h"

class ElementEditor::ElementEditorPrivate
{
private:
    QList<ElementWidget*> widgets;
    Element *element;
    Entry *internalEntry;
    Macro *internalMacro;
    Preamble *internalPreamble;
    Comment *internalComment;
    ElementEditor *p;
    ElementWidget *previousWidget, *referenceWidget, *sourceWidget;

public:

    bool isModified;
    QTabWidget *tab;

    ElementEditorPrivate(Element *m, ElementEditor *parent)
            : element(m), p(parent), previousWidget(NULL) {
        isModified = false;
        createGUI();
        reset(m);
    }

    void createGUI() {
        widgets.clear();
        EntryLayout *el = EntryLayout::self();

        QVBoxLayout *layout = new QVBoxLayout(p);

        if (ReferenceWidget::canEdit(element)) {
            referenceWidget = new ReferenceWidget(p);
            connect(referenceWidget, SIGNAL(modified()), p, SIGNAL(modified()));
            layout->addWidget(referenceWidget);
            widgets << referenceWidget;
        } else
            referenceWidget = NULL;

        tab = new QTabWidget(p);
        layout->addWidget(tab);

        if (EntryConfiguredWidget::canEdit(element))
            for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit) {
                EntryTabLayout etl = *elit;
                ElementWidget *widget = new EntryConfiguredWidget(etl, tab);
                connect(widget, SIGNAL(modified()), p, SIGNAL(modified()));
                tab->addTab(widget, widget->icon(), widget->label());
                widgets << widget;
            }

        if (PreambleWidget::canEdit(element)) {
            ElementWidget *widget = new PreambleWidget(tab);
            connect(widget, SIGNAL(modified()), p, SIGNAL(modified()));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (MacroWidget::canEdit(element)) {
            ElementWidget *widget = new MacroWidget(tab);
            connect(widget, SIGNAL(modified()), p, SIGNAL(modified()));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (OtherFieldsWidget::canEdit(element)) {
            ElementWidget *widget = new OtherFieldsWidget(tab);
            connect(widget, SIGNAL(modified()), p, SIGNAL(modified()));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (SourceWidget::canEdit(element)) {
            sourceWidget = new SourceWidget(tab);
            connect(sourceWidget, SIGNAL(modified()), p, SIGNAL(modified()));
            tab->addTab(sourceWidget, sourceWidget->icon(), sourceWidget->label());
            widgets << sourceWidget;
        }

        previousWidget = dynamic_cast<ElementWidget*>(tab->widget(0));
    }

    void apply() {
        apply(element);
    }

    void apply(Element *element) {
        referenceWidget->apply(element);
        const ElementWidget *widget = dynamic_cast<const ElementWidget*>(tab->currentWidget());
        widget->apply(element);
    }

    void reset() {
        reset(element);
    }

    void reset(const Element *element) {
        for (QList<ElementWidget*>::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->reset(element);

        internalEntry = NULL;
        internalMacro = NULL;
        internalComment = NULL;
        internalPreamble = NULL;
        const Entry *e = dynamic_cast<const Entry*>(element);
        if (e != NULL) {
            internalEntry = new Entry(*e);
        } else {
            const Macro *m = dynamic_cast<const Macro*>(element);
            if (m != NULL)
                internalMacro = new Macro(*m);
            else {
                const Comment *c = dynamic_cast<const Comment*>(element);
                if (c != NULL)
                    internalComment = new Comment(*c);
                else {
                    const Preamble *p = dynamic_cast<const Preamble*>(element);
                    if (p != NULL)
                        internalPreamble = new Preamble(*p);
                    else
                        Q_ASSERT_X(element == NULL, "ElementEditor::ElementEditorPrivate::reset(const Element *element)", "element is not NULL but could not be cast on a valid Element sub-class");
                }
            }
        }
    }

    void setReadOnly(bool isReadOnly) {
        for (QList<ElementWidget*>::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setReadOnly(isReadOnly);
    }

    void switchTo(QWidget *newTab) {
        bool isSourceWidget = newTab == sourceWidget;
        ElementWidget *newWidget = dynamic_cast<ElementWidget*>(newTab);
        if (previousWidget != NULL && newWidget != NULL) {
            Element *temp;
            if (internalEntry != NULL)
                temp = internalEntry;
            else if (internalMacro != NULL)
                temp = internalMacro;
            else if (internalComment != NULL)
                temp = internalComment;
            else if (internalPreamble != NULL)
                temp = internalPreamble;

            previousWidget->apply(temp);
            if (isSourceWidget) referenceWidget->apply(temp);
            newWidget->reset(temp);
            if (dynamic_cast<SourceWidget*>(previousWidget) != NULL) referenceWidget->reset(temp);
        }
        previousWidget = newWidget;

        for (QList<ElementWidget*>::Iterator it = widgets.begin();it != widgets.end();++it)
            (*it)->setEnabled(!isSourceWidget || *it == newTab);
    }
};

ElementEditor::ElementEditor(Element *element, QWidget *parent)
        : QWidget(parent), d(new ElementEditorPrivate(element, this))
{
    connect(d->tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
}

ElementEditor::ElementEditor(const Element *element, QWidget *parent)
        : QWidget(parent)
{
    Element *m = NULL;
    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL)
        m = new Entry(*entry);
    else {
        const Macro *macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL)
            m = new Macro(*macro);
        else {
            const Preamble *preamble = dynamic_cast<const Preamble*>(element);
            if (preamble != NULL)
                m = new Preamble(*preamble);
            else {
                const Comment *comment = dynamic_cast<const Comment*>(element);
                if (comment != NULL)
                    m = new Comment(*comment);
                else
                    Q_ASSERT_X(element == NULL, "ElementEditor::ElementEditor(const Element *element, QWidget *parent)", "element is not NULL but could not be cast on a valid Element sub-class");
            }
        }
    }

    d = new ElementEditorPrivate(m, this);
    setReadOnly(true);
}

void ElementEditor::apply()
{
    d->apply();
}

void ElementEditor::reset()
{
    d->reset();
    d->isModified = false;
}

void ElementEditor::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

void ElementEditor::tabChanged()
{
    d->switchTo(d->tab->currentWidget());
}
