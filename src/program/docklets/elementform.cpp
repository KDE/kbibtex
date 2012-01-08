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

#include <QLayout>
#include <QDockWidget>
#include <QLabel>

#include <KPushButton>
#include <KLocale>
#include <KIconLoader>

#include <elementeditor.h>
#include <mdiwidget.h>
#include <entry.h>
#include "elementform.h"

class ElementForm::ElementFormPrivate
{
private:
    ElementForm *p;
    QGridLayout *layout;
    QSharedPointer<Element> emptyElement;
    QSharedPointer<Element> element;
    const File *file;

public:
    ElementEditor *elementEditor;
    MDIWidget *mdiWidget;
    KPushButton *buttonApply, *buttonReset;
    QWidget *widgetUnmodifiedChanges;

    ElementFormPrivate(ElementForm *parent)
            : p(parent), element(NULL), file(NULL), elementEditor(NULL) {
        layout = new QGridLayout(p);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 0);

        /// Create a special widget that shows a small icon and a text
        /// stating that there are unsaved changes. It will be shown
        /// simultaneously when the Apply and Reset buttons are enabled.
        widgetUnmodifiedChanges = new QWidget(p);
        layout->addWidget(widgetUnmodifiedChanges, 1, 0, 1, 1);
        QBoxLayout *layoutUnmodifiedChanges = new QHBoxLayout(widgetUnmodifiedChanges);
        layoutUnmodifiedChanges->addStretch(100);
        QLabel *label = new QLabel(widgetUnmodifiedChanges);
        label->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        label->setPixmap(KIconLoader::global()->loadIcon("dialog-information", KIconLoader::Dialog, KIconLoader::SizeSmall));
        layoutUnmodifiedChanges->addWidget(label);
        label = new QLabel(i18n("There are unsaved changes. Please press either 'Apply' or 'Reset'."), widgetUnmodifiedChanges);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layoutUnmodifiedChanges->addWidget(label);

        buttonApply = new KPushButton(KIcon("dialog-ok-apply"), i18n("Apply"), p);
        layout->addWidget(buttonApply, 1, 1, 1, 1);

        buttonReset = new KPushButton(KIcon("edit-undo"), i18n("Reset"), p);
        layout->addWidget(buttonReset, 1, 2, 1, 1);

        emptyElement = QSharedPointer<Element>(new Entry());
        loadElement(QSharedPointer<Element>(), NULL);

        connect(buttonApply, SIGNAL(clicked()), p, SIGNAL(elementModified()));
    }

    void refreshElement() {
        loadElement(element, file);
    }

    void loadElement(QSharedPointer<Element> element, const File *file) {
        /// store both element and file for later refresh
        this->element = element;
        this->file = file;

        /// skip whole process of loading an element if not visible
        if (isVisible())
            p->setEnabled(true);
        else {
            p->setEnabled(false);
            return;
        }

        /// recreate and reset element editor
        int tabIndex = 0;
        if (elementEditor != NULL) {
            tabIndex = elementEditor->currentTab();
            delete elementEditor;
        }
        // FIXME why do we need emptyElement?
        elementEditor = element.isNull() ? new ElementEditor(emptyElement, file, p) :
                        new ElementEditor(element, file, p);
        layout->addWidget(elementEditor, 0, 0, 1, 3);
        elementEditor->setEnabled(!element.isNull());
        elementEditor->setCurrentTab(tabIndex);
        elementEditor->layout()->setMargin(0);
        connect(elementEditor, SIGNAL(modified(bool)), p, SLOT(modified()));

        /// make apply and reset buttons aware of new element editor
        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        widgetUnmodifiedChanges->setVisible(false);
        connect(buttonApply, SIGNAL(clicked()), p, SLOT(apply()));
        connect(buttonReset, SIGNAL(clicked()), p, SLOT(reset()));
    }

    bool isVisible() {
        /// get dock where this widget is inside
        /// static cast is save as constructor requires parent to be QDockWidget
        QDockWidget *pp = static_cast<QDockWidget*>(p->parent());
        return pp != NULL && !pp->isHidden();
    }

    void apply() {
        elementEditor->apply();
        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        widgetUnmodifiedChanges->setVisible(false);
    }

    void reset() {
        elementEditor->reset();
        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        widgetUnmodifiedChanges->setVisible(false);
    }
};

ElementForm::ElementForm(MDIWidget *mdiWidget, QDockWidget *parent)
        : QWidget(parent), d(new ElementFormPrivate(this))
{
    connect(parent, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    d->mdiWidget = mdiWidget;
}

void ElementForm::setElement(QSharedPointer<Element> element, const File *file)
{
    d->loadElement(element, file);
}

void ElementForm::modified()
{
    d->buttonApply->setEnabled(true);
    d->buttonReset->setEnabled(true);
    d->widgetUnmodifiedChanges->setVisible(true);
}

void ElementForm::apply()
{
    d->apply();
}

void ElementForm::reset()
{
    d->reset();
}

void ElementForm::visibilityChanged(bool)
{
    d->refreshElement();
}
