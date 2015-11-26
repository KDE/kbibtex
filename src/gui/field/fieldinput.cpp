/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "fieldinput.h"

#include <QLayout>
#include <QApplication>
#include <QMenu>
#include <QDate>
#include <QSignalMapper>
#include <QSpinBox>
#include <QPushButton>
#include <QInputDialog>

#include <KLocalizedString>

#include "file.h"
#include "entry.h"
#include "fieldlineedit.h"
#include "fieldlistedit.h"
#include "colorlabelwidget.h"
#include "starrating.h"

class FieldInput::FieldInputPrivate
{
private:
    FieldInput *p;

public:
    ColorLabelWidget *colorWidget;
    StarRatingFieldInput *starRatingWidget;
    FieldLineEdit *fieldLineEdit;
    FieldListEdit *fieldListEdit;
    KBibTeX::FieldInputType fieldInputType;
    KBibTeX::TypeFlags typeFlags;
    KBibTeX::TypeFlag preferredTypeFlag;
    const File *bibtexFile;
    const Element *element;

    FieldInputPrivate(FieldInput *parent)
            : p(parent), colorWidget(NULL), starRatingWidget(NULL), fieldLineEdit(NULL), fieldListEdit(NULL), fieldInputType(KBibTeX::SingleLine), preferredTypeFlag(KBibTeX::tfSource), bibtexFile(NULL), element(NULL) {
        /// nothing
    }

    ~FieldInputPrivate() {
        if (colorWidget != NULL) delete colorWidget;
        else if (starRatingWidget != NULL) delete starRatingWidget;
        else if (fieldLineEdit != NULL) delete fieldLineEdit;
        else if (fieldListEdit != NULL) delete fieldListEdit;
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
            QPushButton *monthSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("view-calendar-month")), QStringLiteral(""));
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
            QPushButton *referenceSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("flag-gree")), QStringLiteral("")); ///< find better icon
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
        case KBibTeX::StarRating: {
            starRatingWidget = new StarRatingFieldInput(8 /* = #stars */, p);
            layout->addWidget(starRatingWidget, 0);
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
            fieldLineEdit->setText(QStringLiteral(""));
        else if (fieldListEdit != NULL)
            fieldListEdit->clear();
        else if (colorWidget != NULL)
            colorWidget->clear();
        else if (starRatingWidget != NULL)
            starRatingWidget->unsetValue();
        enableModifiedSignal();
    }

    bool reset(const Value &value) {
        /// if signals are not deactivated, the "modified" signal would be emitted when
        /// resetting the widget's value
        disableModifiedSignal();

        bool result = false;
        if (fieldLineEdit != NULL)
            result = fieldLineEdit->reset(value);
        else if (fieldListEdit != NULL)
            result = fieldListEdit->reset(value);
        else if (colorWidget != NULL)
            result = colorWidget->reset(value);
        else if (starRatingWidget != NULL)
            result = starRatingWidget->reset(value);

        enableModifiedSignal();
        return result;
    }

    bool apply(Value &value) const {
        bool result = false;
        if (fieldLineEdit != NULL)
            result = fieldLineEdit->apply(value);
        else if (fieldListEdit != NULL)
            result = fieldListEdit->apply(value);
        else if (colorWidget != NULL)
            result = colorWidget->apply(value);
        else if (starRatingWidget != NULL)
            result = starRatingWidget->apply(value);
        return result;
    }

    void setReadOnly(bool isReadOnly) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setReadOnly(isReadOnly);
        else if (fieldListEdit != NULL)
            fieldListEdit->setReadOnly(isReadOnly);
        else if (colorWidget != NULL)
            colorWidget->setReadOnly(isReadOnly);
        else if (starRatingWidget != NULL)
            starRatingWidget->setReadOnly(isReadOnly);
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

    void setFieldKey(const QString &fieldKey) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setFieldKey(fieldKey);
        if (fieldListEdit != NULL)
            fieldListEdit->setFieldKey(fieldKey);
    }

    void setCompletionItems(const QStringList &items) {
        if (fieldLineEdit != NULL)
            fieldLineEdit->setCompletionItems(items);
        if (fieldListEdit != NULL)
            fieldListEdit->setCompletionItems(items);
    }

    bool selectCrossRef() {
        Q_ASSERT_X(fieldLineEdit != NULL, "void FieldInput::FieldInputPrivate::selectCrossRef()", "fieldLineEdit is invalid");
        if (bibtexFile == NULL) return false;

        /// create a standard input dialog with a list of all keys (ids of entries)
        bool ok = false;
        QStringList list = bibtexFile->allKeys(File::etEntry);
        list.sort();

        /// remove own id
        const Entry *entry = dynamic_cast<const Entry *>(element);
        if (entry != NULL) list.removeOne(entry->id());

        QString crossRef = QInputDialog::getItem(p, i18n("Select Cross Reference"), i18n("Select the cross reference to another entry:"), list, 0, false, &ok);

        if (ok && !crossRef.isEmpty()) {
            /// insert selected cross reference into edit widget
            Value value;
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(crossRef)));
            reset(value);
            return true;
        }
        return false;
    }

    void enableModifiedSignal() {
        if (fieldLineEdit != NULL)
            connect(fieldLineEdit, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));
        if (fieldListEdit != NULL)
            connect(fieldListEdit, SIGNAL(modified()), p, SIGNAL(modified()));
        if (colorWidget != NULL)
            connect(colorWidget, SIGNAL(modified()), p, SIGNAL(modified()));
        if (starRatingWidget != NULL)
            connect(starRatingWidget, SIGNAL(modified()), p, SIGNAL(modified()));
    }

    void disableModifiedSignal() {
        if (fieldLineEdit != NULL)
            disconnect(fieldLineEdit, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));
        if (fieldListEdit != NULL)
            disconnect(fieldListEdit, SIGNAL(modified()), p, SIGNAL(modified()));
        if (colorWidget != NULL)
            disconnect(colorWidget, SIGNAL(modified()), p, SIGNAL(modified()));
        if (starRatingWidget != NULL)
            disconnect(starRatingWidget, SIGNAL(modified()), p, SIGNAL(modified()));
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

FieldInput::~FieldInput()
{
    delete d;
}

void FieldInput::clear()
{
    d->clear();
}

bool FieldInput::reset(const Value &value)
{
    return d->reset(value);
}

bool FieldInput::apply(Value &value) const
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

void FieldInput::setFieldKey(const QString &fieldKey)
{
    d->setFieldKey(fieldKey);
}

void FieldInput::setCompletionItems(const QStringList &items)
{
    d->setCompletionItems(items);
}

QWidget *FieldInput::buddy()
{
    if (d->fieldLineEdit != NULL)
        return d->fieldLineEdit->buddy();
    // TODO fieldListEdit
    else if (d->colorWidget != NULL)
        return d->colorWidget;
    else if (d->starRatingWidget != NULL)
        return d->starRatingWidget;
    return NULL;
}

void FieldInput::setMonth(int month)
{
    Value value;
    value.append(QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[month - 1])));
    reset(value);
    emit modified();
}

void FieldInput::selectCrossRef()
{
    if (d->selectCrossRef())
        emit modified();
}
