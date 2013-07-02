/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include <KComboBox>
#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>

#include "preferences.h"
#include "elementwidgets.h"
#include "bibtexfilemodel.h"
#include "settingsuserinterfacewidget.h"

class SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate
{
private:
    SettingsUserInterfaceWidget *p;

    QCheckBox *checkBoxShowComments;
    QCheckBox *checkBoxShowMacros;
    KComboBox *comboBoxBibliographySystem;
    KComboBox *comboBoxElementDoubleClickAction;

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

        const QStringList styles = configGroup.readEntry("BibTeXStyles", QStringList());
        foreach(QString style, styles) {
            QStringList item = style.split("|");
            QString itemLabel = item.at(0);
            item.removeFirst();
            comboBoxBibliographySystem->addItem(itemLabel, item);
        }
        int styleIndex = comboBoxBibliographySystem->findData(configGroup.readEntry("CurrentStyle", QString()));
        if (styleIndex < 0) styleIndex = 0;
        comboBoxBibliographySystem->setCurrentIndex(styleIndex);

        comboBoxElementDoubleClickAction->setCurrentIndex(configGroup.readEntry(Preferences::keyElementDoubleClickAction, (int)Preferences::defaultElementDoubleClickAction));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(BibTeXFileModel::keyShowComments, checkBoxShowComments->isChecked());
        configGroup.writeEntry(BibTeXFileModel::keyShowMacros, checkBoxShowMacros->isChecked());
        configGroup.writeEntry("CurrentStyle", comboBoxBibliographySystem->itemData(comboBoxBibliographySystem->currentIndex()).toString());
        configGroup.writeEntry(Preferences::keyElementDoubleClickAction, comboBoxElementDoubleClickAction->currentIndex());
        config->sync();
    }

    void resetToDefaults() {
        checkBoxShowComments->setChecked(BibTeXFileModel::defaultShowComments);
        checkBoxShowMacros->setChecked(BibTeXFileModel::defaultShowMacros);
        comboBoxBibliographySystem->setCurrentIndex(0);
        comboBoxElementDoubleClickAction->setCurrentIndex(Preferences::defaultElementDoubleClickAction);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        checkBoxShowComments = new QCheckBox(p);
        layout->addRow(i18n("Show Comments:"), checkBoxShowComments);
        connect(checkBoxShowComments, SIGNAL(toggled(bool)), p, SIGNAL(changed()));

        checkBoxShowMacros = new QCheckBox(p);
        layout->addRow(i18n("Show Macros:"), checkBoxShowMacros);
        connect(checkBoxShowMacros, SIGNAL(toggled(bool)), p, SIGNAL(changed()));

        comboBoxBibliographySystem = new KComboBox(p);
        comboBoxBibliographySystem->setObjectName("comboBoxBibtexStyle");
        layout->addRow(i18n("Bibliography System:"), comboBoxBibliographySystem);
        connect(comboBoxBibliographySystem, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

        comboBoxElementDoubleClickAction = new KComboBox(p);
        comboBoxElementDoubleClickAction->setObjectName("comboBoxElementDoubleClickAction");
        comboBoxElementDoubleClickAction->addItem(i18n("Open Editor")); ///< ActionOpenEditor = 0
        comboBoxElementDoubleClickAction->addItem(i18n("View Document")); ///< ActionViewDocument = 1
        layout->addRow(i18n("When double-clicking an element:"), comboBoxElementDoubleClickAction);
        connect(comboBoxElementDoubleClickAction, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));
    }
};

const QString SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate::configGroupName = QLatin1String("User Interface");


SettingsUserInterfaceWidget::SettingsUserInterfaceWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsUserInterfaceWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

QString SettingsUserInterfaceWidget::label() const
{
    return i18n("User Interface");
}

KIcon SettingsUserInterfaceWidget::icon() const
{
    return KIcon("user-identity");
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
