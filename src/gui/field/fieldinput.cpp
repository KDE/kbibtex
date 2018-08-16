/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
            : p(parent), colorWidget(nullptr), starRatingWidget(nullptr), fieldLineEdit(nullptr), fieldListEdit(nullptr), fieldInputType(KBibTeX::SingleLine), preferredTypeFlag(KBibTeX::tfSource), bibtexFile(nullptr), element(nullptr) {
        /// nothing
    }

    ~FieldInputPrivate() {
        if (colorWidget != nullptr) delete colorWidget;
        else if (starRatingWidget != nullptr) delete starRatingWidget;
        else if (fieldLineEdit != nullptr) delete fieldLineEdit;
        else if (fieldListEdit != nullptr) delete fieldListEdit;
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
            connect(sm, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), p, &FieldInput::setMonth);
            QMenu *monthMenu = new QMenu(monthSelector);
            for (int i = 1; i <= 12; ++i) {
                QAction *monthAction = monthMenu->addAction(QDate::longMonthName(i, QDate::StandaloneFormat), sm, SLOT(map()));
                sm->setMapping(monthAction, i);
            }
            monthSelector->setMenu(monthMenu);
        }
        break;
        case KBibTeX::Edition: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
            QPushButton *editionSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("clock")), QString());
            editionSelector->setToolTip(i18n("Select a predefined edition"));
            fieldLineEdit->prependWidget(editionSelector);

            QSignalMapper *sm = new QSignalMapper(editionSelector);
            connect(sm, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), p, &FieldInput::setEdition);
            QMenu *editionMenu = new QMenu(editionSelector);
            static const QStringList ordinals{i18n("1st"), i18n("2nd"), i18n("3rd"), i18n("4th"), i18n("5th"), i18n("6th"), i18n("7th"), i18n("8th"), i18n("9th"), i18n("10th"), i18n("11th"), i18n("12th"), i18n("13th"), i18n("14th"), i18n("15th"), i18n("16th")};
            for (int i = 0; i < ordinals.length(); ++i) {
                QAction *editionAction = editionMenu->addAction(ordinals[i], sm, SLOT(map()));
                sm->setMapping(editionAction, i + 1);
            }
            editionSelector->setMenu(editionMenu);
        }
        break;
        case KBibTeX::CrossRef: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            layout->addWidget(fieldLineEdit);
            QPushButton *referenceSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("flag-green")), QStringLiteral("")); ///< find better icon
            referenceSelector->setToolTip(i18n("Select an existing entry"));
            fieldLineEdit->prependWidget(referenceSelector);
            connect(referenceSelector, &QPushButton::clicked, p, &FieldInput::selectCrossRef);
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
        if (fieldLineEdit != nullptr)
            fieldLineEdit->setText(QStringLiteral(""));
        else if (fieldListEdit != nullptr)
            fieldListEdit->clear();
        else if (colorWidget != nullptr)
            colorWidget->clear();
        else if (starRatingWidget != nullptr)
            starRatingWidget->unsetValue();
        enableModifiedSignal();
    }

    bool reset(const Value &value) {
        /// if signals are not deactivated, the "modified" signal would be emitted when
        /// resetting the widget's value
        disableModifiedSignal();

        bool result = false;
        if (fieldLineEdit != nullptr)
            result = fieldLineEdit->reset(value);
        else if (fieldListEdit != nullptr)
            result = fieldListEdit->reset(value);
        else if (colorWidget != nullptr)
            result = colorWidget->reset(value);
        else if (starRatingWidget != nullptr)
            result = starRatingWidget->reset(value);

        enableModifiedSignal();
        return result;
    }

    bool apply(Value &value) const {
        bool result = false;
        if (fieldLineEdit != nullptr)
            result = fieldLineEdit->apply(value);
        else if (fieldListEdit != nullptr)
            result = fieldListEdit->apply(value);
        else if (colorWidget != nullptr)
            result = colorWidget->apply(value);
        else if (starRatingWidget != nullptr)
            result = starRatingWidget->apply(value);
        return result;
    }

    void setReadOnly(bool isReadOnly) {
        if (fieldLineEdit != nullptr)
            fieldLineEdit->setReadOnly(isReadOnly);
        else if (fieldListEdit != nullptr)
            fieldListEdit->setReadOnly(isReadOnly);
        else if (colorWidget != nullptr)
            colorWidget->setReadOnly(isReadOnly);
        else if (starRatingWidget != nullptr)
            starRatingWidget->setReadOnly(isReadOnly);
    }

    void setFile(const File *file) {
        bibtexFile = file;
        if (fieldLineEdit != nullptr)
            fieldLineEdit->setFile(file);
        if (fieldListEdit != nullptr)
            fieldListEdit->setFile(file);
    }

    void setElement(const Element *element) {
        this->element = element;
        if (fieldLineEdit != nullptr)
            fieldLineEdit->setElement(element);
        if (fieldListEdit != nullptr)
            fieldListEdit->setElement(element);
    }

    void setFieldKey(const QString &fieldKey) {
        if (fieldLineEdit != nullptr)
            fieldLineEdit->setFieldKey(fieldKey);
        if (fieldListEdit != nullptr)
            fieldListEdit->setFieldKey(fieldKey);
    }

    void setCompletionItems(const QStringList &items) {
        if (fieldLineEdit != nullptr)
            fieldLineEdit->setCompletionItems(items);
        if (fieldListEdit != nullptr)
            fieldListEdit->setCompletionItems(items);
    }

    bool selectCrossRef() {
        Q_ASSERT_X(fieldLineEdit != nullptr, "void FieldInput::FieldInputPrivate::selectCrossRef()", "fieldLineEdit is invalid");
        if (bibtexFile == nullptr) return false;

        /// create a standard input dialog with a list of all keys (ids of entries)
        bool ok = false;
        QStringList list = bibtexFile->allKeys(File::etEntry);
        list.sort();

        /// remove own id
        const Entry *entry = dynamic_cast<const Entry *>(element);
        if (entry != nullptr) list.removeOne(entry->id());

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
        if (fieldLineEdit != nullptr)
            connect(fieldLineEdit, &FieldLineEdit::textChanged, p, &FieldInput::modified);
        if (fieldListEdit != nullptr)
            connect(fieldListEdit, &FieldListEdit::modified, p, &FieldInput::modified);
        if (colorWidget != nullptr)
            connect(colorWidget, &ColorLabelWidget::modified, p, &FieldInput::modified);
        if (starRatingWidget != nullptr)
            connect(starRatingWidget, &StarRatingFieldInput::modified, p, &FieldInput::modified);
    }

    void disableModifiedSignal() {
        if (fieldLineEdit != nullptr)
            disconnect(fieldLineEdit, &FieldLineEdit::textChanged, p, &FieldInput::modified);
        if (fieldListEdit != nullptr)
            disconnect(fieldListEdit, &FieldListEdit::modified, p, &FieldInput::modified);
        if (colorWidget != nullptr)
            disconnect(colorWidget, &ColorLabelWidget::modified, p, &FieldInput::modified);
        if (starRatingWidget != nullptr)
            disconnect(starRatingWidget, &StarRatingFieldInput::modified, p, &FieldInput::modified);
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
    if (d->fieldLineEdit != nullptr)
        return d->fieldLineEdit->buddy();
    // TODO fieldListEdit
    else if (d->colorWidget != nullptr)
        return d->colorWidget;
    else if (d->starRatingWidget != nullptr)
        return d->starRatingWidget;
    return nullptr;
}

void FieldInput::setMonth(int month)
{
    Value value;
    value.append(QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[month - 1])));
    reset(value);
    emit modified();
}

void FieldInput::setEdition(int edition)
{
    Value value;
    value.append(QSharedPointer<MacroKey>(new MacroKey(QString::number(edition))));
    reset(value);
    emit modified();
}

void FieldInput::selectCrossRef()
{
    if (d->selectCrossRef())
        emit modified();
}
