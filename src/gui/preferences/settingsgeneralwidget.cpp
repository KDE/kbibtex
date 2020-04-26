/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "settingsgeneralwidget.h"

#include <QFormLayout>
#include <QComboBox>

#include <KLocalizedString>

#include "preferences.h"
#include "value.h"
#include "guihelper.h"

class SettingsGeneralWidget::SettingsGeneralWidgetPrivate
{
private:
    SettingsGeneralWidget *p;

    QComboBox *comboBoxBibliographySystem;
    QComboBox *comboBoxPersonNameFormatting;
    const Person dummyPerson;

public:

    SettingsGeneralWidgetPrivate(SettingsGeneralWidget *parent)
            : p(parent), dummyPerson(Person(i18n("John"), i18n("Doe"), i18n("Jr."))) {
        setupGUI();
    }

    void loadState() {
        comboBoxBibliographySystem->setCurrentIndex(comboBoxBibliographySystem->findData(QVariant::fromValue<int>(static_cast<int>(Preferences::instance().bibliographySystem()))));

        int row = GUIHelper::selectValue(comboBoxPersonNameFormatting->model(), Person::transcribePersonName(&dummyPerson, Preferences::instance().personNameFormat()));
        comboBoxPersonNameFormatting->setCurrentIndex(row);
    }

    bool saveState() {
        bool settingsGotChanged = false;
        settingsGotChanged |= Preferences::instance().setBibliographySystem(static_cast<Preferences::BibliographySystem>(comboBoxBibliographySystem->currentData().toInt()));
        settingsGotChanged |= Preferences::instance().setPersonNameFormat(comboBoxPersonNameFormatting->itemData(comboBoxPersonNameFormatting->currentIndex()).toString());
        return settingsGotChanged;
    }

    void resetToDefaults() {
        comboBoxBibliographySystem->setCurrentIndex(static_cast<int>(Preferences::defaultBibliographySystem));

        int row = GUIHelper::selectValue(comboBoxPersonNameFormatting->model(), Person::transcribePersonName(&dummyPerson, Preferences::defaultPersonNameFormat));
        comboBoxPersonNameFormatting->setCurrentIndex(row);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxBibliographySystem = new QComboBox(p);
        comboBoxBibliographySystem->setObjectName(QStringLiteral("comboBoxBibliographySystem"));

        for (QVector<QPair<Preferences::BibliographySystem, QString>>::ConstIterator it = Preferences::availableBibliographySystems.constBegin(); it != Preferences::availableBibliographySystems.constEnd(); ++it)
            comboBoxBibliographySystem->addItem(it->second, QVariant::fromValue<int>(static_cast<int>(it->first)));
        layout->addRow(i18n("Bibliography System:"), comboBoxBibliographySystem);
        connect(comboBoxBibliographySystem, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsGeneralWidget::changed);

        comboBoxPersonNameFormatting = new QComboBox(p);
        layout->addRow(i18n("Person Names Formatting:"), comboBoxPersonNameFormatting);
        static const QStringList formattingOptions {Preferences::personNameFormatFirstLast, Preferences::personNameFormatLastFirst};
        for (const QString &formattingOption : formattingOptions)
            comboBoxPersonNameFormatting->addItem(Person::transcribePersonName(&dummyPerson, formattingOption), formattingOption);
        connect(comboBoxPersonNameFormatting, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsGeneralWidget::changed);
    }
};


SettingsGeneralWidget::SettingsGeneralWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsGeneralWidgetPrivate(this))
{
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

QIcon SettingsGeneralWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("kbibtex"));
}

void SettingsGeneralWidget::loadState()
{
    d->loadState();
}

bool SettingsGeneralWidget::saveState()
{
    return d->saveState();
}

void SettingsGeneralWidget::resetToDefaults()
{
    d->resetToDefaults();
}
