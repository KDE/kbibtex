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
#include <QLineEdit>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>
#include <KLocale>

#include "fileexportertoolchain.h"
#include "settingsfileexporterpdfpswidget.h"

class SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidgetPrivate
{
private:
    SettingsFileExporterPDFPSWidget *p;

    KComboBox *comboBoxBabelLanguage;
    KComboBox *comboBoxBibliographyStyle;

    KSharedConfigPtr config;
    static const QString configGroupName;

public:

    SettingsFileExporterPDFPSWidgetPrivate(SettingsFileExporterPDFPSWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QString babelLanguage = configGroup.readEntry(FileExporterToolchain::keyBabelLanguage, FileExporterToolchain::defaultBabelLanguage);
        p->selectValue(comboBoxBabelLanguage, babelLanguage);
        QString bibliographyStyle = configGroup.readEntry(FileExporterToolchain::keyBibliographyStyle, FileExporterToolchain::defaultBibliographyStyle);
        p->selectValue(comboBoxBibliographyStyle, bibliographyStyle);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(FileExporterToolchain::keyBabelLanguage, comboBoxBabelLanguage->lineEdit()->text());
        configGroup.writeEntry(FileExporterToolchain::keyBibliographyStyle, comboBoxBibliographyStyle->lineEdit()->text());
        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxBabelLanguage, FileExporterToolchain::defaultBabelLanguage);
        p->selectValue(comboBoxBibliographyStyle, FileExporterToolchain::defaultBibliographyStyle);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxBabelLanguage = new KComboBox(true, p);
        comboBoxBabelLanguage->setObjectName("comboBoxBabelLanguage");
        layout->addRow(i18n("Language for 'babel':"), comboBoxBabelLanguage);
        comboBoxBabelLanguage->addItem(QLatin1String("english"));
        comboBoxBabelLanguage->addItem(QLatin1String("ngerman"));
        comboBoxBabelLanguage->addItem(QLatin1String("swedish"));
        connect(comboBoxBabelLanguage->lineEdit(), SIGNAL(textChanged(QString)), p, SIGNAL(changed()));

        comboBoxBibliographyStyle = new KComboBox(true, p);
        comboBoxBibliographyStyle->setObjectName("comboBoxBibliographyStyle");
        layout->addRow(i18n("Bibliography style:"), comboBoxBibliographyStyle);
        comboBoxBibliographyStyle->addItem(QLatin1String("abbrv"));
        comboBoxBibliographyStyle->addItem(QLatin1String("alpha"));
        comboBoxBibliographyStyle->addItem(QLatin1String("plain"));
        comboBoxBibliographyStyle->addItem(QLatin1String("dcu"));
        connect(comboBoxBibliographyStyle->lineEdit(), SIGNAL(textChanged(QString)), p, SIGNAL(changed()));
    }
};

const QString SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidgetPrivate::configGroupName = QLatin1String("FileExporterPDFPS");

SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterPDFPSWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

void SettingsFileExporterPDFPSWidget::loadState()
{
    d->loadState();
}

void SettingsFileExporterPDFPSWidget::saveState()
{
    d->saveState();
}

void SettingsFileExporterPDFPSWidget::resetToDefaults()
{
    d->resetToDefaults();
}
