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

#include <KPushButton>
#include <KLocale>

#include <elementeditor.h>
#include <mdiwidget.h>
#include <entry.h>
#include "elementform.h"

class ElementForm::ElementFormPrivate
{
private:
    ElementForm *p;
    QGridLayout *layout;
    Entry emptyElement;

public:
    ElementEditor *elementEditor;
    MDIWidget *mdiWidget;
    KPushButton *buttonApply, *buttonReset;

    ElementFormPrivate(ElementForm *parent)
            : p(parent), elementEditor(NULL) {
        layout = new QGridLayout(p);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 0);

        buttonApply = new KPushButton(KIcon("apply"), i18n("Apply"), p);
        layout->addWidget(buttonApply, 1, 1, 1, 1);

        buttonReset = new KPushButton(KIcon("reset"), i18n("Reset"), p);
        layout->addWidget(buttonReset, 1, 2, 1, 1);

        loadElement(NULL);

        connect(buttonApply, SIGNAL(clicked()), p, SIGNAL(elementModified()));
    }

    void loadElement(Element *element) {
        if (elementEditor != NULL)
            delete elementEditor;
        elementEditor = element == NULL ? new ElementEditor(&emptyElement, p) : new ElementEditor(element, p);
        layout->addWidget(elementEditor, 0, 0, 1, 3);
        elementEditor->setEnabled(element != NULL);
        elementEditor->layout()->setMargin(0);
        connect(elementEditor, SIGNAL(modified()), p, SLOT(modified()));

        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        connect(buttonApply, SIGNAL(clicked()), elementEditor, SLOT(apply()));
        connect(buttonReset, SIGNAL(clicked()), elementEditor, SLOT(reset()));
        connect(buttonApply, SIGNAL(clicked()), p, SLOT(modificationCleared()));
        connect(buttonReset, SIGNAL(clicked()), p, SLOT(modificationCleared()));
    }
};

ElementForm::ElementForm(MDIWidget *mdiWidget, QWidget *parent)
        : QWidget(parent), d(new ElementFormPrivate(this))
{
    d->mdiWidget = mdiWidget;
}

void ElementForm::setElement(Element* element, const File *)
{
    d->loadElement(element);
}

void ElementForm::modified()
{
    d->buttonApply->setEnabled(true);
    d->buttonReset->setEnabled(true);
}

void ElementForm::modificationCleared()
{
    d->buttonApply->setEnabled(false);
    d->buttonReset->setEnabled(false);
}
