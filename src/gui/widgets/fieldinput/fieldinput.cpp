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
#include <QApplication>
#include <QMenu>
#include <QDate>
#include <QSignalMapper>

#include <KDebug>
#include <KPushButton>

#include <fieldlineedit.h>
#include <fieldlistedit.h>
#include "fieldinput.h"

class FieldInput::FieldInputPrivate
{
private:
    FieldInput *p;
    FieldLineEdit *fieldLineEdit;
    FieldListEdit *fieldListEdit;

public:
    KBibTeX::FieldInputType fieldInputType;
    KBibTeX::TypeFlags typeFlags;

    FieldInputPrivate(FieldInput *parent)
            : p(parent), fieldLineEdit(NULL), fieldListEdit(NULL) {
        // TODO
    }

    void createGUI() {
        p->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        QVBoxLayout *layout = new QVBoxLayout(p);
        layout->setMargin(0);

        switch (fieldInputType) {
        case KBibTeX::MultiLine:
            fieldLineEdit = new FieldLineEdit(typeFlags, true, p);
            layout->addWidget(fieldLineEdit);
            connect(fieldLineEdit, SIGNAL(editingFinished()), p, SIGNAL(modified()));
            break;
        case KBibTeX::List:
            fieldListEdit = new FieldListEdit(p);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::Month: {
            fieldLineEdit = new FieldLineEdit(typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
            connect(fieldLineEdit, SIGNAL(editingFinished()), p, SIGNAL(modified()));
            KPushButton *monthSelector = new KPushButton(KIcon("view-calendar-month"), "");
            monthSelector->setStyleSheet(QLatin1String("padding: 0px; margin-left:2px; margin-right:2px; text-align: left; background-color: ") + QPalette().color(QPalette::Base).name() + QLatin1String("; border-style: none;"));
            fieldLineEdit->prependWidget(monthSelector);

            QSignalMapper *sm = new QSignalMapper(monthSelector);
            connect(sm, SIGNAL(mapped(int)), p, SLOT(setMonth(int)));
            QMenu *monthMenu = new QMenu(monthSelector);
            for (int i = 1; i <= 12; ++i) {
                QAction *monthAction = monthMenu->addAction(QDate::longMonthName(i, QDate::StandaloneFormat), sm, SLOT(map()));
                sm->setMapping(monthAction, i);
            }
            monthSelector->setMenu(monthMenu);
        }
        break;
        default:
            fieldLineEdit = new FieldLineEdit(typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
            connect(fieldLineEdit, SIGNAL(editingFinished()), p, SIGNAL(modified()));
        }
    }

    void clear() {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setText("");
        else if (fieldListEdit != NULL)
            fieldListEdit->clear();
    }

    void setValue(const Value& value) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setValue(value);
        else if (fieldListEdit != NULL)
            fieldListEdit->setValue(value);
    }

    void applyTo(Value& value) const {
        if (fieldLineEdit != NULL)
            fieldLineEdit->applyTo(value);
        else if (fieldListEdit != NULL)
            fieldListEdit->applyTo(value);
    }

    void setReadOnly(bool isReadOnly) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setReadOnly(isReadOnly);
        else if (fieldListEdit != NULL)
            fieldListEdit->setReadOnly(isReadOnly);
    }
};

FieldInput::FieldInput(KBibTeX::FieldInputType fieldInputType, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : QWidget(parent), d(new FieldInputPrivate(this))
{
    d->fieldInputType = fieldInputType;
    d->typeFlags = typeFlags;
    d->createGUI();
}

void FieldInput::clear()
{
    d->clear();
}

void FieldInput::setValue(const Value& value)
{
    d->setValue(value);
}

void FieldInput::applyTo(Value& value) const
{
    d->applyTo(value);
}

void FieldInput::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

void FieldInput::setMonth(int month)
{
    MacroKey *macro = new MacroKey(KBibTeX::MonthsTriple[month-1]);
    Value value;
    value.append(macro);
    setValue(value);
}
