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
#include <KLocale>
#include <KPushButton>
#include <KInputDialog>

#include <file.h>
#include <entry.h>
#include <fieldlineedit.h>
#include <fieldlistedit.h>
#include <colorlabelwidget.h>
#include "fieldinput.h"

class FieldInput::FieldInputPrivate
{
private:
    FieldInput *p;
    FieldLineEdit *fieldLineEdit;
    FieldListEdit *fieldListEdit;
    ColorLabelWidget *colorWidget;

public:
    KBibTeX::FieldInputType fieldInputType;
    KBibTeX::TypeFlags typeFlags;
    KBibTeX::TypeFlag preferredTypeFlag;
    const File *bibtexFile;
    const Element *element;

    FieldInputPrivate(FieldInput *parent)
            : p(parent), fieldLineEdit(NULL), fieldListEdit(NULL), colorWidget(NULL), bibtexFile(NULL), element(NULL) {
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
        case KBibTeX::CrossRef: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
            KPushButton *referenceSelector = new KPushButton(KIcon("flag-gree"), ""); ///< find better icon
            referenceSelector->setToolTip(i18n("Select an existing entry"));
            fieldLineEdit->prependWidget(referenceSelector);
            connect(referenceSelector, SIGNAL(clicked()), p, SLOT(selectCrossRef()));
        }
        break;
        case KBibTeX::Color: {
            colorWidget = new ColorLabelWidget(p);
            layout->addWidget(colorWidget, 0);
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
            result = colorWidget->reset(value);
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
            result = colorWidget->apply(value);
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
        bibtexFile = file;
        if (fieldLineEdit != NULL)
            fieldLineEdit->setFile(file);
        if (fieldListEdit != NULL)
            fieldListEdit->setFile(file);
    }

    void setElement(const Element *element) {
        this->element = element;
        if (fieldLineEdit != NULL)
            fieldLineEdit->setElement(element);
        if (fieldListEdit != NULL)
            fieldListEdit->setElement(element);
    }

    void setCompletionItems(const QStringList &items) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setCompletionItems(items);
        if (fieldListEdit != NULL)
            fieldListEdit->setCompletionItems(items);
    }

    void selectCrossRef() {
        Q_ASSERT(fieldLineEdit != NULL);
        if (bibtexFile == NULL) return;

        /// create a standard input dialog with a list of all keys (ids of entries)
        bool ok = false;
        QStringList list = bibtexFile->allKeys(File::etEntry);
        list.sort();

        /// remove own id
        const Entry *entry = dynamic_cast<const Entry*>(element);
        if (entry != NULL) list.removeOne(entry->id());

        QString crossRef = KInputDialog::getItem(i18n("Select Cross Reference"), i18n("Select the cross reference to another entry:"), list, 0, false, &ok, p);

        if (ok && !crossRef.isEmpty()) {
            /// insert selected cross reference into edit widget
            VerbatimText *verbatimText = new VerbatimText(crossRef);
            Value value;
            value.append(verbatimText);
            reset(value);
        }
    }


    void enableModifiedSignal() {
        if (fieldLineEdit != NULL)
            connect(fieldLineEdit, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));
        if (fieldListEdit != NULL)
            connect(fieldListEdit, SIGNAL(modified()), p, SIGNAL(modified()));
        if (colorWidget != NULL) {
            connect(colorWidget, SIGNAL(modified()), p, SIGNAL(modified()));
        }
        // TODO
    }

    void disableModifiedSignal() {
        if (fieldLineEdit != NULL)
            disconnect(fieldLineEdit, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));
        if (fieldListEdit != NULL)
            disconnect(fieldListEdit, SIGNAL(modified()), p, SIGNAL(modified()));
        if (colorWidget != NULL) {
            disconnect(colorWidget, SIGNAL(modified()), p, SIGNAL(modified()));
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

void FieldInput::setElement(const Element *element)
{
    d->setElement(element);
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

void FieldInput::selectCrossRef()
{
    d->selectCrossRef();
}
