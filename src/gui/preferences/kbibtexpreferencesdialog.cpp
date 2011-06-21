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
#include "settingsfileexporterbibtexwidget.h"
#include "settingsfileexporterpdfpswidget.h"
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
        KPageWidgetItem *page = p->addPage(settingsWidget, i18n("General"));
        page->setIcon(KIcon("kbibtex"));

        KPageWidgetItem *pageSaving = p->addPage(new QWidget(), i18n("Saving"));
        pageSaving->setIcon(KIcon("document-save"));

        settingsWidget = new SettingsFileExporterBibTeXWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageSaving, settingsWidget, i18n("BibTeX"));
        page->setIcon(KIcon("text-x-bibtex"));

        settingsWidget = new SettingsFileExporterPDFPSWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageSaving, settingsWidget, i18n("PDF and Postscript"));
        page->setIcon(KIcon("application-pdf"));
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
    setButtons(Help | Default | Ok | Apply | Cancel);
    setDefaultButton(Ok);
    setModal(true);
    showButtonSeparator(true);

    connect(this, SIGNAL(applyClicked()), this, SLOT(apply()));
    connect(this, SIGNAL(okClicked()), this, SLOT(ok()));
    connect(this, SIGNAL(defaultClicked()), this, SLOT(resetToDefaults()));

    d->addPages();
}

void KBibTeXPreferencesDialog::apply()
{
    d->saveState();
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
