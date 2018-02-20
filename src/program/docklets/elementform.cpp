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

#include "elementform.h"

#include <QLayout>
#include <QDockWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#include <KLocalizedString>
#include <KIconLoader>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KMessageBox>

#include "entry.h"
#include "elementeditor.h"
#include "mdiwidget.h"

class ElementForm::ElementFormPrivate
{
private:
    ElementForm *p;
    QGridLayout *layout;
    const File *file;

public:
    ElementEditor *elementEditor;
    MDIWidget *mdiWidget;
    QCheckBox *checkBoxAutoApply;
    QPushButton *buttonApply, *buttonReset;
    QWidget *widgetUnmodifiedChanges;
    bool gotModified;
    QSharedPointer<Element> element;

    KSharedConfigPtr config;
    /// Group name in configuration file for all settings for this form
    static const QString configGroupName;
    /// Key to store/retrieve setting whether changes in form should be automatically applied to element or not
    static const QString configKeyAutoApply;

    ElementFormPrivate(MDIWidget *_mdiWidget, ElementForm *parent)
            : p(parent), file(nullptr), mdiWidget(_mdiWidget), gotModified(false), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))) {
        KConfigGroup configGroup(config, configGroupName);

        layout = new QGridLayout(p);
        layout->setColumnStretch(0, 10);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 0);
        layout->setColumnStretch(3, 0);

        elementEditor = new ElementEditor(true, p);
        layout->addWidget(elementEditor, 0, 0, 1, 4);
        elementEditor->setEnabled(false);
        elementEditor->layout()->setMargin(0);
        connect(elementEditor, &ElementEditor::modified, p, &ElementForm::modified);

        /// Checkbox enabling/disabling setting to automatically apply changes in form to element
        checkBoxAutoApply = new QCheckBox(i18n("Automatically apply changes"), p);
        checkBoxAutoApply->setChecked(configGroup.readEntry(configKeyAutoApply, false));
        layout->addWidget(checkBoxAutoApply, 1, 0, 1, 1);

        /// Create a special widget that shows a small icon and a text
        /// stating that there are unsaved changes. It will be shown
        /// simultaneously when the Apply and Reset buttons are enabled.
        // TODO nearly identical code as in SearchResultsPrivate constructor, create common class
        widgetUnmodifiedChanges = new QWidget(p);
        layout->addWidget(widgetUnmodifiedChanges, 1, 1, 1, 1);
        QBoxLayout *layoutUnmodifiedChanges = new QHBoxLayout(widgetUnmodifiedChanges);
        layoutUnmodifiedChanges->addSpacing(32);
        QLabel *label = new QLabel(widgetUnmodifiedChanges);
        label->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        label->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("dialog-information"), KIconLoader::Dialog, KIconLoader::SizeSmall));
        layoutUnmodifiedChanges->addWidget(label);
        label = new QLabel(i18n("There are unsaved changes. Please press either 'Apply' or 'Reset'."), widgetUnmodifiedChanges);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layoutUnmodifiedChanges->addWidget(label);

        buttonApply = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")), i18n("Apply"), p);
        layout->addWidget(buttonApply, 1, 2, 1, 1);

        buttonReset = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-undo")), i18n("Reset"), p);
        layout->addWidget(buttonReset, 1, 3, 1, 1);

        connect(checkBoxAutoApply, &QCheckBox::toggled, p, &ElementForm::autoApplyToggled);
        connect(buttonApply, &QPushButton::clicked, p, &ElementForm::validateAndOnlyThenApply);
        connect(buttonReset, &QPushButton::clicked, p, &ElementForm::reset);
    }

    ~ElementFormPrivate() {
        delete elementEditor;
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

        elementEditor->setElement(element, file);
        elementEditor->setEnabled(!element.isNull());

        /// make apply and reset buttons aware of new element editor
        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        widgetUnmodifiedChanges->setVisible(false);
        gotModified = false;
    }

    bool isVisible() {
        /// get dock where this widget is inside
        /// static cast is save as constructor requires parent to be QDockWidget
        QDockWidget *pp = static_cast<QDockWidget *>(p->parent());
        return pp != nullptr && !pp->isHidden();
    }

    void apply() {
        elementEditor->apply();
        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        gotModified = false;
        widgetUnmodifiedChanges->setVisible(false);
    }

    void reset() {
        elementEditor->reset();
        buttonApply->setEnabled(false);
        buttonReset->setEnabled(false);
        gotModified = false;
        widgetUnmodifiedChanges->setVisible(false);
    }
};

const QString ElementForm::ElementFormPrivate::configGroupName = QStringLiteral("ElementForm");
const QString ElementForm::ElementFormPrivate::configKeyAutoApply = QStringLiteral("AutoApply");

ElementForm::ElementForm(MDIWidget *mdiWidget, QDockWidget *parent)
        : QWidget(parent), d(new ElementFormPrivate(mdiWidget, this))
{
    connect(parent, &QDockWidget::visibilityChanged, this, &ElementForm::visibilityChanged);
}

ElementForm::~ElementForm()
{
    delete d;
}

void ElementForm::setElement(QSharedPointer<Element> element, const File *file)
{
    /// Test if previous element (1) got modified, (2) the new element isn't
    /// the same as the new one, and (3) the user confirms to apply those
    /// changes rather than to discard them -> apply changes in previous element.
    /// FIXME If the previous element got delete from the file and therefore a different
    /// element gets set, changes will be still applied to the element to-be-deleted.
    if (d->gotModified && element != d->element && KMessageBox::questionYesNo(this, i18n("The current element got modified.\nApply or discard changes?"), i18n("Element modified"), KGuiItem(i18n("Apply changes"), QIcon::fromTheme(QStringLiteral("dialog-ok-apply"))), KGuiItem(i18n("Discard changes"), QIcon::fromTheme(QStringLiteral("edit-undo")))) == KMessageBox::Yes) {
        d->apply();
    }
    if (element != d->element) {
        /// Ignore loading the same element again
        d->loadElement(element, file);
    }
}

void ElementForm::refreshElement()
{
    d->refreshElement();
}

/**
 * Fetch the modified signal from the editing widget.
 * @param gotModified true if widget was modified by user, false if modified status was reset by e.g. apply operation
 */
void ElementForm::modified(bool gotModified)
{
    /// Only interested in modifications, not resets of modified status
    if (!gotModified) return;

    if (d->checkBoxAutoApply->isChecked()) {
        /// User wants to automatically apply changes, so do it
        // FIXME validateAndOnlyThenApply();
        apply();
    } else {
        /// No automatic apply, therefore enable buttons where user can
        /// apply or reset changes, plus show warning label about unsaved changes
        d->buttonApply->setEnabled(true);
        d->buttonReset->setEnabled(true);
        d->widgetUnmodifiedChanges->setVisible(true);
        d->gotModified = true;
    }
}

void ElementForm::apply()
{
    d->apply();

    /// Notify rest of program (esp. main list) about changes
    emit elementModified();

}

bool ElementForm::validateAndOnlyThenApply()
{
    const bool isValid = d->elementEditor->validate();
    if (isValid)
        apply();
    return isValid;
}

void ElementForm::reset()
{
    d->reset();
}

void ElementForm::visibilityChanged(bool)
{
    d->refreshElement();
}

/**
 * React on toggles of checkbox for auto-apply.
 * @param isChecked true if checkbox got checked, false if checkbox got unchecked
 */
void ElementForm::autoApplyToggled(bool isChecked)
{
    if (isChecked) {
        /// Got toggled to check state
        if (!d->element.isNull()) {
            validateAndOnlyThenApply();
        } else {
            /// The following settings would happen when calling apply(),
            /// but as no valid element is edited, perform settings here instead
            d->buttonApply->setEnabled(false);
            d->buttonReset->setEnabled(false);
            d->widgetUnmodifiedChanges->setVisible(false);
            d->gotModified = false;
        }
    }

    /// Save changed status of checkbox in configuration settings
    KConfigGroup configGroup(d->config, ElementFormPrivate::configGroupName);
    configGroup.writeEntry(ElementFormPrivate::configKeyAutoApply, d->checkBoxAutoApply->isChecked());
    configGroup.sync();
}
