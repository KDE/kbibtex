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

#include <QLayout>
#include <QResizeEvent>
#include <QSignalMapper>

#include <KMessageBox>
#include <KLocale>
#include <KPushButton>

#include <file.h>
#include <entry.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <fieldlineedit.h>
#include "fieldlistedit.h"

class FieldListEdit::FieldListEditPrivate
{
private:
    FieldListEdit *p;
    const int innerSpacing;
    QSignalMapper *smRemove, *smGoUp, *smGoDown;
    QVBoxLayout *layout;
    KBibTeX::TypeFlags typeFlags;

public:
    QList<FieldLineEdit*> lineEditList;
    QWidget *container;
    KPushButton *addButton;

    FieldListEditPrivate(KBibTeX::TypeFlags tf, FieldListEdit *parent)
            : p(parent), innerSpacing(4), typeFlags(tf) {
        smRemove = new QSignalMapper(parent);
        smGoUp = new QSignalMapper(parent);
        smGoDown = new QSignalMapper(parent);
        setupGUI();
    }

    void setupGUI() {
        p->setBackgroundRole(QPalette::Base);

        container = new QWidget();
        container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        p->setWidget(container);
        layout = new QVBoxLayout(container);
        layout->setMargin(0);
        layout->setSpacing(innerSpacing);

        addButton = new KPushButton(KIcon("list-add"), i18n("Add"), container);
        layout->addWidget(addButton);
        connect(addButton, SIGNAL(clicked()), p, SLOT(lineAdd()));

        connect(smRemove, SIGNAL(mapped(QWidget*)), p, SLOT(lineRemove(QWidget*)));
        connect(smGoDown, SIGNAL(mapped(QWidget*)), p, SLOT(lineGoDown(QWidget*)));
        connect(smGoUp, SIGNAL(mapped(QWidget*)), p, SLOT(lineGoUp(QWidget*)));

        p->ensureWidgetVisible(container);
        // TODO
    }

    int recommendedHeight() {
        int heightHint = 0;

        for (QList<FieldLineEdit*>::ConstIterator it = lineEditList.constBegin();it != lineEditList.constEnd(); ++it)
            heightHint += (*it)->sizeHint().height();

        heightHint += lineEditList.count() * innerSpacing;
        heightHint += addButton->sizeHint().height();

        return heightHint;
    }

    FieldLineEdit *addFieldLineEdit() {
        FieldLineEdit *le = new FieldLineEdit(typeFlags, false, container);
        layout->insertWidget(layout->count() - 1, le);
        lineEditList.append(le);

        KPushButton *remove = new KPushButton(KIcon("list-remove"), QLatin1String(""), le);
        le->appendWidget(remove);
        connect(remove, SIGNAL(clicked()), smRemove, SLOT(map()));
        smRemove->setMapping(remove, le);

        KPushButton *goDown = new KPushButton(KIcon("go-down"), QLatin1String(""), le);
        le->appendWidget(goDown);
        connect(goDown, SIGNAL(clicked()), smGoDown, SLOT(map()));
        smGoDown->setMapping(goDown, le);

        KPushButton *goUp = new KPushButton(KIcon("go-up"), QLatin1String(""), le);
        le->appendWidget(goUp);
        connect(goUp, SIGNAL(clicked()), smGoUp, SLOT(map()));
        smGoUp->setMapping(goUp, le);

        return le;
    }

    void removeAllFieldLineEdits() {
        while (!lineEditList.isEmpty()) {
            FieldLineEdit *fieldLineEdit = *lineEditList.begin();
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeFirst();
        }
    }

    void removeFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        lineEditList.removeOne(fieldLineEdit);
        layout->removeWidget(fieldLineEdit);
        delete fieldLineEdit;
    }

    void goDownFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx < lineEditList.count() - 1) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx + 1, fieldLineEdit);
            layout->insertWidget(idx + 1, fieldLineEdit);
        }
    }

    void goUpFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx > 0) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx - 1, fieldLineEdit);
            layout->insertWidget(idx - 1, fieldLineEdit);
        }
    }
};

FieldListEdit::FieldListEdit(KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : QScrollArea(parent), d(new FieldListEditPrivate(typeFlags, this))
{
// TODO
}

void FieldListEdit::setValue(const Value& value)
{
    d->removeAllFieldLineEdits();
    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        Value v;
        v.append(*it);
        FieldLineEdit *fieldLineEdit = d->addFieldLineEdit();
        fieldLineEdit->setValue(v);
    }
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
}

void FieldListEdit::applyTo(Value& value) const
{
    value.clear();

    for (QList<FieldLineEdit*>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it) {
        Value v;
        (*it)->applyTo(v);
        for (Value::ConstIterator itv = v.constBegin(); itv != v.constEnd(); ++itv)
            value.append(*itv);
    }
}

void FieldListEdit::clear()
{
    d->removeAllFieldLineEdits();
}

void FieldListEdit::setReadOnly(bool isReadOnly)
{
    for (QList<FieldLineEdit*>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it)
        (*it)->setReadOnly(isReadOnly);
    d->addButton->setEnabled(!isReadOnly);
}

/*
void FieldListEdit::reset()
{
    loadValue(m_originalValue);
}
*/

void FieldListEdit::resizeEvent(QResizeEvent *event)
{

    QSize size(event->size().width(), d->recommendedHeight());
    d->container->resize(size);
}

void FieldListEdit::lineAdd()
{
    d->addFieldLineEdit();
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
}

void FieldListEdit::lineRemove(QWidget * widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(widget);
    d->removeFieldLineEdit(fieldLineEdit);
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
}

void FieldListEdit::lineGoDown(QWidget * widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(widget);
    d->goDownFieldLineEdit(fieldLineEdit);
}

void FieldListEdit::lineGoUp(QWidget * widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit*>(widget);
    d->goUpFieldLineEdit(fieldLineEdit);

}
