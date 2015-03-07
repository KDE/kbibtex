/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <QCheckBox>

#include <KPushButton>
#include <KLocale>
#include <KIconLoader>
#include <KMessageBox>
#include <KConfigGroup>

#include <elementeditor.h>
#include <mdiwidget.h>
#include <entry.h>
#include "elementform.h"

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
    KPushButton *buttonApply, *buttonReset;
    QWidget *widgetUnmodifiedChanges;
    bool gotModified;
    QSharedPointer<Element> element;

    KSharedConfigPtr config;
    /// Group name in configuration file for all settings for this form
    static const QString configGroupName;
    /// Key to store/retrieve setting whether changes in form should be automatically applied to element or not
    static const QString configKeyAutoApply;

    ElementFormPrivate(ElementForm *parent)
            : p(parent), file(NULL), gotModified(false), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        KConfigGroup configGroup(config, configGroupName);

        layout = new QGridLayout(p);
        layout->setColumnStretch(0, 10);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 0);
        layout->setColumnStretch(3, 0);

        elementEditor = new ElementEditor(p);
        layout->addWidget(elementEditor, 0, 0, 1, 4);
        elementEditor->setEnabled(false);
        elementEditor->layout()->setMargin(0);
        connect(elementEditor, SIGNAL(modified(bool)), p, SLOT(modified(bool)));

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
        label->setPixmap(KIconLoader::global()->loadIcon("dialog-information", KIconLoader::Dialog, KIconLoader::SizeSmall));
        layoutUnmodifiedChanges->addWidget(label);
        label = new QLabel(i18n("There are unsaved changes. Please press either 'Apply' or 'Reset'."), widgetUnmodifiedChanges);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layoutUnmodifiedChanges->addWidget(label);

        buttonApply = new KPushButton(KIcon("dialog-ok-apply"), i18n("Apply"), p);
        layout->addWidget(buttonApply, 1, 2, 1, 1);

        buttonReset = new KPushButton(KIcon("edit-undo"), i18n("Reset"), p);
        layout->addWidget(buttonReset, 1, 3, 1, 1);

        connect(buttonApply, SIGNAL(clicked()), p, SIGNAL(elementModified()));
        connect(checkBoxAutoApply, SIGNAL(toggled(bool)), p, SLOT(autoApplyToggled(bool)));
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
        connect(buttonApply, SIGNAL(clicked()), p, SLOT(apply()));
        connect(buttonReset, SIGNAL(clicked()), p, SLOT(reset()));
    }

    bool isVisible() {
        /// get dock where this widget is inside
        /// static cast is save as constructor requires parent to be QDockWidget
        QDockWidget *pp = static_cast<QDockWidget *>(p->parent());
        return pp != NULL && !pp->isHidden();
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

const QString ElementForm::ElementFormPrivate::configGroupName = QLatin1String("ElementForm");
const QString ElementForm::ElementFormPrivate::configKeyAutoApply = QLatin1String("AutoApply");

ElementForm::ElementForm(MDIWidget *mdiWidget, QDockWidget *parent)
        : QWidget(parent), d(new ElementFormPrivate(this))
{
    connect(parent, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    d->mdiWidget = mdiWidget;
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
    if (d->gotModified && element != d->element && KMessageBox::questionYesNo(this, i18n("The current element got modified.\nApply or discard changes?"), i18n("Element modified"), KGuiItem(i18n("Apply changes"), KIcon("dialog-ok-apply")), KGuiItem(i18n("Discard changes"), KIcon("edit-undo"))) == KMessageBox::Yes) {
        d->apply();
    }
    if (element != d->element) {
        /// Ignore loading the same element again
        d->loadElement(element, file);
    }
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
        apply();
        /// Notify rest of program (esp. main list) about changes
        emit elementModified();
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
            /// Working on a real element, so apply changes
            apply();
            emit elementModified();
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
