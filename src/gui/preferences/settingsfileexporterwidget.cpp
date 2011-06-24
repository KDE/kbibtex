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

#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>

#include <fileexporter.h>
#include "settingsfileexporterwidget.h"

class SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate
{
private:
    SettingsFileExporterWidget *p;

    KComboBox *comboBoxPaperSize;
    QMap<QString, QString> paperSizeLabelToName;

    KSharedConfigPtr config;
    static const QString configGroupName;

public:

    SettingsFileExporterWidgetPrivate(SettingsFileExporterWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        paperSizeLabelToName.insert(i18n("A4"), QLatin1String("a4"));
        paperSizeLabelToName.insert(i18n("Letter"), QLatin1String("letter"));
        paperSizeLabelToName.insert(i18n("Legal"), QLatin1String("legal"));
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QString paperSizeName = configGroup.readEntry(FileExporter::keyPaperSize, FileExporter::defaultPaperSize);
        p->selectValue(comboBoxPaperSize, paperSizeLabelToName[paperSizeName]);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        QString paperSizeName = paperSizeLabelToName.key(comboBoxPaperSize->currentText());
        configGroup.writeEntry(FileExporter::keyPaperSize, paperSizeName);
        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxPaperSize, paperSizeLabelToName[FileExporter::defaultPaperSize]);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxPaperSize = new KComboBox(false, p);
        layout->addRow(i18n("Paper Size:"), comboBoxPaperSize);
        QStringList paperSizeLabelToNameKeys = paperSizeLabelToName.keys();
        paperSizeLabelToNameKeys.sort();
        foreach(QString labelText, paperSizeLabelToNameKeys) {
            comboBoxPaperSize->addItem(labelText, paperSizeLabelToName[labelText]);
        }
    }
};

const QString SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate::configGroupName = QLatin1String("General");


SettingsFileExporterWidget::SettingsFileExporterWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

void SettingsFileExporterWidget::loadState()
{
    d->loadState();
}

void SettingsFileExporterWidget::saveState()
{
    d->saveState();
}

void SettingsFileExporterWidget::resetToDefaults()
{
    d->resetToDefaults();
}
