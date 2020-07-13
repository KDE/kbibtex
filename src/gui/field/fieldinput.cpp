/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QSpinBox>
#include <QPushButton>
#include <QInputDialog>
#include <QSignalBlocker>

#include <KLocalizedString>

#include <File>
#include <Entry>
#include <FileExporterBibTeX>
#include "fieldlineedit.h"
#include "fieldlistedit.h"
#include "colorlabelwidget.h"
#include "widgets/starrating.h"

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
            : p(parent), colorWidget(nullptr), starRatingWidget(nullptr), fieldLineEdit(nullptr), fieldListEdit(nullptr), fieldInputType(KBibTeX::FieldInputType::SingleLine), preferredTypeFlag(KBibTeX::TypeFlag::Source), bibtexFile(nullptr), element(nullptr) {
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
        case KBibTeX::FieldInputType::MultiLine:
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, true, p);
            connect(fieldLineEdit, &FieldLineEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldLineEdit);
            break;
        case KBibTeX::FieldInputType::List:
            fieldListEdit = new FieldListEdit(preferredTypeFlag, typeFlags, p);
            connect(fieldListEdit, &FieldListEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::FieldInputType::Month: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            connect(fieldLineEdit, &FieldLineEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldLineEdit);
            QPushButton *monthSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("view-calendar-month")), QString());
            monthSelector->setToolTip(i18n("Select a predefined month"));
            fieldLineEdit->prependWidget(monthSelector);

            QMenu *monthMenu = new QMenu(monthSelector);
            for (int i = 1; i <= 12; ++i) {
                QAction *monthAction = monthMenu->addAction(QLocale::system().standaloneMonthName(i));
                connect(monthAction, &QAction::triggered, p, [this, i]() {
                    setMonth(i);
                });
            }
            monthSelector->setMenu(monthMenu);
        }
        break;
        case KBibTeX::FieldInputType::Edition: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            connect(fieldLineEdit, &FieldLineEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldLineEdit);
            QPushButton *editionSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("clock")), QString());
            editionSelector->setToolTip(i18n("Select a predefined edition"));
            fieldLineEdit->prependWidget(editionSelector);

            QMenu *editionMenu = new QMenu(editionSelector);
            static const QStringList ordinals{i18n("1st"), i18n("2nd"), i18n("3rd"), i18n("4th"), i18n("5th"), i18n("6th"), i18n("7th"), i18n("8th"), i18n("9th"), i18n("10th"), i18n("11th"), i18n("12th"), i18n("13th"), i18n("14th"), i18n("15th"), i18n("16th")};
            for (int i = 0; i < ordinals.length(); ++i) {
                QAction *editionAction = editionMenu->addAction(ordinals[i]);
                connect(editionAction, &QAction::triggered, p, [this, i]() {
                    setEdition(i + 1);
                });
            }
            editionSelector->setMenu(editionMenu);
        }
        break;
        case KBibTeX::FieldInputType::CrossRef: {
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            connect(fieldLineEdit, &FieldLineEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldLineEdit);
            QPushButton *referenceSelector = new QPushButton(QIcon::fromTheme(QStringLiteral("flag-green")), QString()); ///< find better icon
            referenceSelector->setToolTip(i18n("Select an existing entry"));
            fieldLineEdit->prependWidget(referenceSelector);
            connect(referenceSelector, &QPushButton::clicked, p, &FieldInput::selectCrossRef);
        }
        break;
        case KBibTeX::FieldInputType::Color: {
            colorWidget = new ColorLabelWidget(p);
            connect(colorWidget, &ColorLabelWidget::modified, p, &FieldInput::modified);
            layout->addWidget(colorWidget, 0);
        }
        break;
        case KBibTeX::FieldInputType::StarRating: {
            starRatingWidget = new StarRatingFieldInput(8 /* = #stars */, p);
            connect(starRatingWidget, &StarRatingFieldInput::modified, p, &FieldInput::modified);
            layout->addWidget(starRatingWidget, 0);
        }
        break;
        case KBibTeX::FieldInputType::PersonList:
            fieldListEdit = new PersonListEdit(preferredTypeFlag, typeFlags, p);
            connect(fieldListEdit, &PersonListEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::FieldInputType::UrlList:
            fieldListEdit = new UrlListEdit(p);
            connect(fieldListEdit, &FieldListEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldListEdit);
            break;
        case KBibTeX::FieldInputType::KeywordList:
            fieldListEdit = new KeywordListEdit(p);
            connect(fieldListEdit, &FieldListEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldListEdit);
            break;
        default:
            fieldLineEdit = new FieldLineEdit(preferredTypeFlag, typeFlags, false, p);
            connect(fieldLineEdit, &FieldLineEdit::modified, p, &FieldInput::modified);
            layout->addWidget(fieldLineEdit);
        }
    }

    void clear() {
        if (fieldLineEdit != nullptr) {
            const QSignalBlocker blocker(fieldLineEdit);
            fieldLineEdit->clear();
        } else if (fieldListEdit != nullptr) {
            const QSignalBlocker blocker(fieldListEdit);
            fieldListEdit->clear();
        } else if (colorWidget != nullptr) {
            const QSignalBlocker blocker(colorWidget);
            colorWidget->clear();
        } else if (starRatingWidget != nullptr) {
            const QSignalBlocker blocker(starRatingWidget);
            starRatingWidget->unsetValue();
        }
    }

    bool reset(const Value &value) {
        bool result = false;
        if (fieldLineEdit != nullptr) {
            const QSignalBlocker blocker(fieldLineEdit);
            result = fieldLineEdit->reset(value);
        } else if (fieldListEdit != nullptr) {
            const QSignalBlocker blocker(fieldListEdit);
            result = fieldListEdit->reset(value);
        } else if (colorWidget != nullptr) {
            const QSignalBlocker blocker(colorWidget);
            result = colorWidget->reset(value);
        } else if (starRatingWidget != nullptr) {
            const QSignalBlocker blocker(starRatingWidget);
            result = starRatingWidget->reset(value);
        }

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

    bool validate(QWidget **widgetWithIssue, QString &message) const {
        if (fieldLineEdit != nullptr)
            return fieldLineEdit->validate(widgetWithIssue, message);
        else if (fieldListEdit != nullptr)
            return fieldListEdit->validate(widgetWithIssue, message);
        else if (colorWidget != nullptr)
            return colorWidget->validate(widgetWithIssue, message);
        else if (starRatingWidget != nullptr)
            return starRatingWidget->validate(widgetWithIssue, message);
        return false;
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
        QStringList list = bibtexFile->allKeys(File::ElementType::Entry);
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

    void setMonth(int month)
    {
        Value value;
        value.append(QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[month - 1])));
        reset(value);
        /// Instead of an 'emit' ...
        QMetaObject::invokeMethod(p, "modified", Qt::DirectConnection, QGenericReturnArgument());
    }

    void setEdition(int edition)
    {
        const QString editionString = FileExporterBibTeX::editionNumberToString(edition);
        if (!editionString.isEmpty()) {
            Value value;
            value.append(QSharedPointer<PlainText>(new PlainText(editionString)));
            reset(value);
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(p, "modified", Qt::DirectConnection, QGenericReturnArgument());
        }
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

bool FieldInput::validate(QWidget **widgetWithIssue, QString &message) const
{
    return d->validate(widgetWithIssue, message);
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

void FieldInput::selectCrossRef()
{
    if (d->selectCrossRef())
        emit modified();
}
