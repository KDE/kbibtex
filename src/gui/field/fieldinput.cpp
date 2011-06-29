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
#include <QPaintEngine>
#include <Q3Painter>

#include <KDebug>
#include <KLocale>
#include <KPushButton>
#include <KColorButton>

#include <fieldlineedit.h>
#include <fieldlistedit.h>
#include "fieldinput.h"

class FieldInput::FieldInputPrivate
{
private:
    FieldInput *p;
    FieldLineEdit *fieldLineEdit;
    FieldListEdit *fieldListEdit;
    KColorButton *colorButton;
    KPushButton *predefColorButton;
    KPushButton *resetColorButton;
    QWidget *colorWidget;
    QMenu *colorMenu;
    QSignalMapper *colorSignalMapper;

public:
    KBibTeX::FieldInputType fieldInputType;
    KBibTeX::TypeFlags typeFlags;
    KBibTeX::TypeFlag preferredTypeFlag;

    FieldInputPrivate(FieldInput *parent)
            : p(parent), fieldLineEdit(NULL), fieldListEdit(NULL), colorButton(NULL), colorWidget(NULL) {
        // TODO
    }

    void createGUI() {
        QHBoxLayout *layout = new QHBoxLayout(p);
        layout->setMargin(0);

        switch (fieldInputType) {
        case KBibTeX::MultiLine:
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, true, p);
            layout->addWidget(fieldLineEdit);
            break;
        case KBibTeX::List:
            fieldListEdit = new FieldListEdit(preferredTypeFlag, typeFlags, p);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::Month: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
            KPushButton *monthSelector = new KPushButton(KIcon("view-calendar-month"), "");
            monthSelector->setToolTip(i18n("Select a predefined month"));
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
        case KBibTeX::Color: {
            colorWidget = new QWidget(p);
            QBoxLayout *boxLayout = new QHBoxLayout(colorWidget);
            boxLayout->setMargin(0);
            predefColorButton = new KPushButton(KIcon("color-picker-white"), i18n("Predefined colors"), colorWidget);
            boxLayout->addWidget(predefColorButton, 0);
            colorButton = new KColorButton(colorWidget);
            boxLayout->addWidget(colorButton, 0);
            layout->addWidget(colorWidget, 0);
            resetColorButton = new KPushButton(KIcon("edit-clear-locationbar-rtl"), i18n("Reset"), colorWidget);
            layout->addWidget(resetColorButton, 0, Qt::AlignLeft);
            connect(resetColorButton, SIGNAL(clicked()), p, SLOT(resetColor()));

            colorSignalMapper = new QSignalMapper(predefColorButton);
            connect(colorSignalMapper, SIGNAL(mapped(QString)), p, SLOT(setColor(QString)));
            colorMenu = new QMenu(predefColorButton);
            predefColorButton->setMenu(colorMenu);

            // TODO: Make it configurable

            QAction *action = colorAction(i18n("Important"), "#cc3300");
            colorMenu->addAction(action);
            colorSignalMapper->setMapping(action, "#cc3300");
            connect(action, SIGNAL(triggered()), colorSignalMapper, SLOT(map()));

            action = colorAction(i18n("Read"), "#009966");
            colorMenu->addAction(action);
            colorSignalMapper->setMapping(action, "#009966");
            connect(action, SIGNAL(triggered()), colorSignalMapper, SLOT(map()));
        }
        break;
        case KBibTeX::PersonList:
            fieldListEdit = new PersonListEdit(preferredTypeFlag, typeFlags, p);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::UrlList:
            fieldListEdit = new UrlListEdit(p);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::KeywordList:
            fieldListEdit = new KeywordListEdit(p);
            layout->addWidget(fieldListEdit);
            break;
        default:
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
        }

        enableModifiedSignal();
    }

    QAction *colorAction(const QString&label, const QString &color) {
        int h = predefColorButton->fontMetrics().height() - 4;
        QPixmap pm(h, h);
        QPainter painter(&pm);
        painter.setPen(QColor(color));
        painter.setBrush(QBrush(painter.pen().color()));
        painter.drawRect(0, 0, h, h);
        return new QAction(KIcon(pm), label, p);
    }

    void clear() {
        disableModifiedSignal();
        if (fieldLineEdit != NULL)
            fieldLineEdit->setText("");
        else if (fieldListEdit != NULL)
            fieldListEdit->clear();
        enableModifiedSignal();
    }

    bool reset(const Value& value) {
        /// if signals are not deactivated, the "modified" signal would be emitted when
        /// resetting the widget's value
        disableModifiedSignal();

        bool result = false;
        if (fieldLineEdit != NULL)
            result = fieldLineEdit->reset(value);
        else if (fieldListEdit != NULL)
            result = fieldListEdit->reset(value);
        else if (colorWidget != NULL) {
            disconnect(colorButton, SIGNAL(changed(QColor)), p, SIGNAL(modified()));

            VerbatimText *verbatimText = NULL;
            if (value.count() == 1 && (verbatimText = dynamic_cast<VerbatimText*>(value.first())) != NULL)
                colorButton->setColor(QColor(verbatimText->text()));
            else
                p->resetColor();
            result = true;
        }

        enableModifiedSignal();
        return result;
    }

    bool apply(Value& value) const {
        bool result = false;
        if (fieldLineEdit != NULL)
            result = fieldLineEdit->apply(value);
        else if (fieldListEdit != NULL)
            result = fieldListEdit->apply(value);
        else if (colorWidget != NULL) {
            value.clear();
            const QString colorName = colorButton->color().name();
            if (!(colorButton->color() == QColor(Qt::black)) && colorName != QLatin1String("#000000")) { // FIXME test looks redundant
                VerbatimText *verbatimText = new VerbatimText(colorName);
                value << verbatimText;
            }
            result = true;
        }
        return result;
    }

    void setReadOnly(bool isReadOnly) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setReadOnly(isReadOnly);
        else if (fieldListEdit != NULL)
            fieldListEdit->setReadOnly(isReadOnly);
    }

    void setFile(const File *file) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setFile(file);
        if (fieldListEdit != NULL)
            fieldListEdit->setFile(file);
    }

    void setCompletionItems(const QStringList &items) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setCompletionItems(items);
        if (fieldListEdit != NULL)
            fieldListEdit->setCompletionItems(items);
    }

    void enableModifiedSignal() {
        if (fieldLineEdit != NULL)
            connect(fieldLineEdit, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));
        if (fieldListEdit != NULL)
            connect(fieldListEdit, SIGNAL(modified()), p, SIGNAL(modified()));
        if (colorButton != NULL) {
            connect(resetColorButton, SIGNAL(clicked()), p, SIGNAL(modified()));
            connect(colorButton, SIGNAL(changed(QColor)), p, SIGNAL(modified()));
            connect(colorSignalMapper, SIGNAL(mapped(int)), p, SIGNAL(modified()));
        }
        // TODO
    }

    void disableModifiedSignal() {
        if (fieldLineEdit != NULL)
            disconnect(fieldLineEdit, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));
        if (fieldListEdit != NULL)
            disconnect(fieldListEdit, SIGNAL(modified()), p, SIGNAL(modified()));
        if (colorButton != NULL) {
            disconnect(resetColorButton, SIGNAL(clicked()), p, SIGNAL(modified()));
            disconnect(colorButton, SIGNAL(changed(QColor)), p, SIGNAL(modified()));
            disconnect(colorSignalMapper, SIGNAL(mapped(int)), p, SIGNAL(modified()));
        }
        // TODO
    }
};

FieldInput::FieldInput(KBibTeX::FieldInputType fieldInputType, KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : QWidget(parent), d(new FieldInputPrivate(this))
{
    d->fieldInputType = fieldInputType;
    d->typeFlags = typeFlags;
    d->preferredTypeFlag = preferredTypeFlag;
    d->createGUI();
}

void FieldInput::clear()
{
    d->clear();
}

bool FieldInput::reset(const Value& value)
{
    return d->reset(value);
}

bool FieldInput::apply(Value& value) const
{
    return d->apply(value);
}

void FieldInput::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

void FieldInput::setFile(const File *file)
{
    d->setFile(file);
}

void FieldInput::setCompletionItems(const QStringList &items)
{
    d->setCompletionItems(items);
}

void FieldInput::setMonth(int month)
{
    MacroKey *macro = new MacroKey(KBibTeX::MonthsTriple[month-1]);
    Value value;
    value.append(macro);
    reset(value);
}

void FieldInput::setColor(const QString&color)
{
    VerbatimText *verbatimText = new VerbatimText(color);
    Value value;
    value.append(verbatimText);
    reset(value);
}

void FieldInput::resetColor()
{
    VerbatimText *verbatimText = new VerbatimText(QLatin1String("#000000"));
    Value value;
    value.append(verbatimText);
    reset(value);
}
