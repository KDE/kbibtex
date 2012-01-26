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

#include <QFormLayout>
#include <QCheckBox>
#include <QBoxLayout>

#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>

#include <elementwidgets.h>
#include "bibtexfilemodel.h"
#include "settingsuserinterfacewidget.h"

class SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate
{
private:
    SettingsUserInterfaceWidget *p;

    QCheckBox *checkBoxShowComments;
    QCheckBox *checkBoxShowMacros;
    QCheckBox *checkBoxLabelsAboveWidget;

    KSharedConfigPtr config;
    static const QString configGroupName;

public:
    SettingsUserInterfaceWidgetPrivate(SettingsUserInterfaceWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        checkBoxShowComments->setChecked(configGroup.readEntry(BibTeXFileModel::keyShowComments, BibTeXFileModel::defaultShowComments));
        checkBoxShowMacros->setChecked(configGroup.readEntry(BibTeXFileModel::keyShowMacros, BibTeXFileModel::defaultShowMacros));
        checkBoxLabelsAboveWidget->setChecked((Qt::Orientation)(configGroup.readEntry(ElementWidget::keyElementWidgetLayout, (int)ElementWidget::defaultElementWidgetLayout)) == Qt::Vertical);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(BibTeXFileModel::keyShowComments, checkBoxShowComments->isChecked());
        configGroup.writeEntry(BibTeXFileModel::keyShowMacros, checkBoxShowMacros->isChecked());
        configGroup.writeEntry(ElementWidget::keyElementWidgetLayout, (int)(checkBoxLabelsAboveWidget->isChecked() ? Qt::Vertical : Qt::Horizontal));
        config->sync();
    }

    void resetToDefaults() {
        checkBoxShowComments->setChecked(BibTeXFileModel::defaultShowComments);
        checkBoxShowMacros->setChecked(BibTeXFileModel::defaultShowMacros);
        checkBoxLabelsAboveWidget->setChecked(ElementWidget::defaultElementWidgetLayout == Qt::Vertical);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        checkBoxShowComments = new QCheckBox(p);
        layout->addRow(i18n("Show Comments:"), checkBoxShowComments);
        connect(checkBoxShowComments, SIGNAL(toggled(bool)), p, SIGNAL(changed()));
        checkBoxShowMacros = new QCheckBox(p);
        layout->addRow(i18n("Show Macros:"), checkBoxShowMacros);
        connect(checkBoxShowMacros, SIGNAL(toggled(bool)), p, SIGNAL(changed()));
        checkBoxLabelsAboveWidget = new QCheckBox(i18n("Labels above edit widgets"), p);
        layout->addRow(i18n("Entry Editor:"), checkBoxLabelsAboveWidget);
        connect(checkBoxLabelsAboveWidget, SIGNAL(toggled(bool)), p, SIGNAL(changed()));
    }
};

const QString SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate::configGroupName = QLatin1String("User Interface");


SettingsUserInterfaceWidget::SettingsUserInterfaceWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsUserInterfaceWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsUserInterfaceWidget::~SettingsUserInterfaceWidget()
{
    delete d;
}

void SettingsUserInterfaceWidget::loadState()
{
    d->loadState();
}

void SettingsUserInterfaceWidget::saveState()
{
    d->saveState();
}

void SettingsUserInterfaceWidget::resetToDefaults()
{
    d->resetToDefaults();
}
