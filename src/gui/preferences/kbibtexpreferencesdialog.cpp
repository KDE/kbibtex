/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "kbibtexpreferencesdialog.h"

#include <QSet>
#include <QFileInfo>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KMessageBox>
#include <KGuiItem>

#include <NotificationHub>
#include "settingsgeneralwidget.h"
#include "settingsglobalkeywordswidget.h"
#include "settingsfileexporterpdfpswidget.h"
#include "settingsfileexporterwidget.h"
#include "settingscolorlabelwidget.h"
#include "settingsuserinterfacewidget.h"
#include "settingsidsuggestionswidget.h"
#include "logging_gui.h"

class KBibTeXPreferencesDialog::KBibTeXPreferencesDialogPrivate
{
private:
    KBibTeXPreferencesDialog *p;
    QSet<SettingsAbstractWidget *> settingWidgets;

public:
    bool notifyOfChanges;

    KBibTeXPreferencesDialogPrivate(KBibTeXPreferencesDialog *parent)
            : p(parent) {
        notifyOfChanges = false;
    }

    void addPages() {
        SettingsAbstractWidget *settingsWidget = new SettingsGeneralWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *pageGlobal = p->addPage(settingsWidget, settingsWidget->label());
        pageGlobal->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);

        settingsWidget = new SettingsGlobalKeywordsWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *page = p->addSubPage(pageGlobal, settingsWidget, settingsWidget->label());
        page->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);

        settingsWidget = new SettingsColorLabelWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageGlobal, settingsWidget, settingsWidget->label());
        page->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);

        settingsWidget = new SettingsIdSuggestionsWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageGlobal, settingsWidget, settingsWidget->label());
        page->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);

        settingsWidget = new SettingsUserInterfaceWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addPage(settingsWidget, settingsWidget->label());
        page->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);

        settingsWidget = new SettingsFileExporterWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *pageSaving = p->addPage(settingsWidget, settingsWidget->label());
        pageSaving->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);

        settingsWidget = new SettingsFileExporterPDFPSWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageSaving, settingsWidget, settingsWidget->label());
        page->setIcon(settingsWidget->icon());
        connect(settingsWidget, &SettingsAbstractWidget::changed, p, &KBibTeXPreferencesDialog::gotChanged);
    }

    void loadState() {
        for (SettingsAbstractWidget *settingsWidget : const_cast<const QSet<SettingsAbstractWidget *> &>(settingWidgets)) {
            settingsWidget->loadState();
        }
    }

    void saveState() {
        for (SettingsAbstractWidget *settingsWidget : const_cast<const QSet<SettingsAbstractWidget *> &>(settingWidgets)) {
            settingsWidget->saveState();
        }
    }

    void restoreDefaults() {
        notifyOfChanges = true;
        switch (KMessageBox::warningYesNoCancel(p, i18n("This will reset the settings to factory defaults. Should this affect only the current page or all settings?"), i18n("Reset to Defaults"), KGuiItem(i18n("All settings"), QStringLiteral("edit-undo")), KGuiItem(i18n("Only current page"), QStringLiteral("document-revert")))) {
        case KMessageBox::Yes: {
            for (SettingsAbstractWidget *settingsWidget : const_cast<const QSet<SettingsAbstractWidget *> &>(settingWidgets)) {
                settingsWidget->resetToDefaults();
            }
            break;
        }
        case KMessageBox::No: {
            SettingsAbstractWidget *widget = qobject_cast<SettingsAbstractWidget *>(p->currentPage()->widget());
            if (widget != nullptr)
                widget->resetToDefaults();
            break;
        }
        case KMessageBox::Cancel:
            break; /// nothing to do here
        default:
            qCWarning(LOG_KBIBTEX_GUI) << "There should be no use for a default case here!";
        }
    }

    void apply()
    {
        p->buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
        saveState();
        notifyOfChanges = true;
    }

    void reset()
    {
        p->buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
        loadState();
    }

    void ok()
    {
        notifyOfChanges = true;
        if (p->buttonBox()->button(QDialogButtonBox::Apply)->isEnabled()) {
            /// Apply settings only if changes have made
            /// (state stored by Apply button being enabled or disabled)
            apply();
        }
        p->accept();
    }
};

KBibTeXPreferencesDialog::KBibTeXPreferencesDialog(QWidget *parent, Qt::WindowFlags flags)
        : KPageDialog(parent, flags), d(new KBibTeXPreferencesDialogPrivate(this))
{
    setFaceType(KPageDialog::Tree);
    setWindowTitle(i18n("Preferences"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    connect(buttonBox, &QDialogButtonBox::clicked, this, &KBibTeXPreferencesDialog::buttonClicked);
    setButtonBox(buttonBox);

    setModal(true);

    d->addPages();
}

KBibTeXPreferencesDialog::~KBibTeXPreferencesDialog()
{
    delete d;
}

void KBibTeXPreferencesDialog::hideEvent(QHideEvent *)
{
    // TODO missing documentation: what triggers 'hideEvent' and why is 'notifyOfChanges' necessary?
    if (d->notifyOfChanges)
        NotificationHub::publishEvent(NotificationHub::EventConfigurationChanged);
}

void KBibTeXPreferencesDialog::buttonClicked(QAbstractButton *button) {
    switch (buttonBox()->standardButton(button)) {
    case QDialogButtonBox::Apply:
        d->apply();
        break;
    case QDialogButtonBox::Ok:
        d->ok();
        break;
    case QDialogButtonBox::RestoreDefaults:
        d->restoreDefaults();
        break;
    case QDialogButtonBox::Reset:
        d->reset();
        break;
    default:
        qCWarning(LOG_KBIBTEX_GUI) << "There should be no use for a default case here!";
    }
}

void KBibTeXPreferencesDialog::gotChanged()
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}
