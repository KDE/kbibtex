/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QSet>

#include <KLocale>
#include <KComboBox>

#include "settingsgeneralwidget.h"
#include "settingsglobalkeywordswidget.h"
#include "settingsfileexporterbibtexwidget.h"
#include "settingsfileexporterpdfpswidget.h"
#include "settingsfileexporterwidget.h"
#include "settingscolorlabelwidget.h"
#include "settingsuserinterfacewidget.h"
#include "kbibtexpreferencesdialog.h"

class KBibTeXPreferencesDialog::KBibTeXPreferencesDialogPrivate
{
private:
    KBibTeXPreferencesDialog *p;
    KComboBox *listOfEncodings;
    QSet<SettingsAbstractWidget*> settingWidgets;

public:
    KBibTeXPreferencesDialogPrivate(KBibTeXPreferencesDialog *parent)
            : p(parent) {
        // nothing
    }

    void addPages() {
        SettingsAbstractWidget *settingsWidget = new SettingsGeneralWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *pageGlobal = p->addPage(settingsWidget, i18n("General"));
        pageGlobal->setIcon(KIcon("kbibtex"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsGlobalKeywordsWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *page = p->addSubPage(pageGlobal, settingsWidget, i18n("Keywords"));
        page->setIcon(KIcon("checkbox")); // TODO find better icon
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsColorLabelWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageGlobal, settingsWidget, i18n("Color & Labels"));
        page->setIcon(KIcon("preferences-desktop-color"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsUserInterfaceWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addPage(settingsWidget, i18n("User Interface"));
        page->setIcon(KIcon("user-identity"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsFileExporterWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *pageSaving = p->addPage(settingsWidget, i18n("Saving and Exporting"));
        pageSaving->setIcon(KIcon("document-save"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsFileExporterBibTeXWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageSaving, settingsWidget, i18n("BibTeX"));
        page->setIcon(KIcon("text-x-bibtex"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsFileExporterPDFPSWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageSaving, settingsWidget, i18n("PDF, Postscript, and RTF"));
        page->setIcon(KIcon("application-pdf"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));
    }

    void loadState() {
        foreach(SettingsAbstractWidget *settingsWidget, settingWidgets) {
            settingsWidget->loadState();
        }
    }

    void saveState() {
        foreach(SettingsAbstractWidget *settingsWidget, settingWidgets) {
            settingsWidget->saveState();
        }
    }

    void resetToDefaults() {
        foreach(SettingsAbstractWidget *settingsWidget, settingWidgets) {
            settingsWidget->resetToDefaults();
        }
    }
};

KBibTeXPreferencesDialog::KBibTeXPreferencesDialog(QWidget *parent, Qt::WFlags flags)
        : KPageDialog(parent, flags), d(new KBibTeXPreferencesDialogPrivate(this))
{
    setFaceType(KPageDialog::Tree);
    setWindowTitle(i18n("Preferences"));
    setButtons(Default | Reset | Ok | Apply | Cancel);
    setDefaultButton(Ok);
    enableButtonApply(false);
    setModal(true);
    showButtonSeparator(true);

    connect(this, SIGNAL(applyClicked()), this, SLOT(apply()));
    connect(this, SIGNAL(okClicked()), this, SLOT(ok()));
    connect(this, SIGNAL(defaultClicked()), this, SLOT(resetToDefaults()));
    connect(this, SIGNAL(resetClicked()), this, SLOT(reset()));

    d->addPages();
}

KBibTeXPreferencesDialog::~KBibTeXPreferencesDialog()
{
    delete d;
}

void KBibTeXPreferencesDialog::apply()
{
    enableButtonApply(false);
    d->saveState();
}

void KBibTeXPreferencesDialog::reset()
{
    enableButtonApply(false);
    d->loadState();
}

void KBibTeXPreferencesDialog::ok()
{
    apply();
    accept();
}

void KBibTeXPreferencesDialog::resetToDefaults()
{
    d->resetToDefaults();
}

void KBibTeXPreferencesDialog::gotChanged()
{
    enableButtonApply(true);
}
