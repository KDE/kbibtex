/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "settingsgeneralwidget.h"

#include <QFormLayout>

#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>

#include "guihelper.h"
#include "value.h"
#include "preferences.h"

class SettingsGeneralWidget::SettingsGeneralWidgetPrivate
{
private:
    SettingsGeneralWidget *p;

    KComboBox *comboBoxPersonNameFormatting;
    const Person dummyPerson;
    QString restartRequiredMsg;

    KSharedConfigPtr config;
    const QString configGroupName;

public:

    SettingsGeneralWidgetPrivate(SettingsGeneralWidget *parent)
            : p(parent), dummyPerson(Person(i18n("John"), i18n("Doe"), i18n("Jr."))), restartRequiredMsg(i18n("Changing this option requires a restart to take effect.")), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("General")) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QString personNameFormatting = configGroup.readEntry(Preferences::keyPersonNameFormatting, Preferences::defaultPersonNameFormatting);
        int row = GUIHelper::selectValue(comboBoxPersonNameFormatting->model(), Person::transcribePersonName(&dummyPerson, personNameFormatting));
        comboBoxPersonNameFormatting->setCurrentIndex(row);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(Preferences::keyPersonNameFormatting, comboBoxPersonNameFormatting->itemData(comboBoxPersonNameFormatting->currentIndex()));
        config->sync();
    }

    void resetToDefaults() {
        int row = GUIHelper::selectValue(comboBoxPersonNameFormatting->model(), Person::transcribePersonName(&dummyPerson, Preferences::defaultPersonNameFormatting));
        comboBoxPersonNameFormatting->setCurrentIndex(row);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxPersonNameFormatting = new KComboBox(false, p);
        layout->addRow(i18n("Person Names Formatting:"), comboBoxPersonNameFormatting);
        const QStringList formattingOptions = QStringList() << Preferences::personNameFormatFirstLast << Preferences::personNameFormatLastFirst;
        foreach(const QString &formattingOption, formattingOptions) {
            comboBoxPersonNameFormatting->addItem(Person::transcribePersonName(&dummyPerson, formattingOption), formattingOption);
        }
        comboBoxPersonNameFormatting->setToolTip(restartRequiredMsg);
        connect(comboBoxPersonNameFormatting, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));
    }
};


SettingsGeneralWidget::SettingsGeneralWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsGeneralWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsGeneralWidget::~SettingsGeneralWidget()
{
    delete d;
}

QString SettingsGeneralWidget::label() const
{
    return i18n("General");
}

KIcon SettingsGeneralWidget::icon() const
{
    return KIcon("kbibtex");
}

void SettingsGeneralWidget::loadState()
{
    d->loadState();
}

void SettingsGeneralWidget::saveState()
{
    d->saveState();
}

void SettingsGeneralWidget::resetToDefaults()
{
    d->resetToDefaults();
}
