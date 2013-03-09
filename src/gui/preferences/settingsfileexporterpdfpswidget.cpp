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
#include <QLineEdit>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>
#include <KLocale>

#include "guihelper.h"
#include "fileexportertoolchain.h"
#include "settingsfileexporterpdfpswidget.h"

class SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidgetPrivate
{
private:
    SettingsFileExporterPDFPSWidget *p;

    KComboBox *comboBoxPaperSize;
    QMap<QString, QString> paperSizeLabelToName;

    KComboBox *comboBoxBabelLanguage;
    KComboBox *comboBoxBibliographyStyle;

    KSharedConfigPtr config;
    const QString configGroupName, configGroupNameGeneral ;

public:

    SettingsFileExporterPDFPSWidgetPrivate(SettingsFileExporterPDFPSWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("FileExporterPDFPS")), configGroupNameGeneral(QLatin1String("General")) {
        paperSizeLabelToName.insert(i18n("A4"), QLatin1String("a4"));
        paperSizeLabelToName.insert(i18n("Letter"), QLatin1String("letter"));
        paperSizeLabelToName.insert(i18n("Legal"), QLatin1String("legal"));
    }

    void loadState() {
        KConfigGroup configGroupGeneral(config, configGroupNameGeneral);
        const QString paperSizeName = configGroupGeneral.readEntry(FileExporter::keyPaperSize, FileExporter::defaultPaperSize);
        int row = GUIHelper::selectValue(comboBoxPaperSize->model(), paperSizeLabelToName.key(paperSizeName));
        comboBoxPaperSize->setCurrentIndex(row);

        KConfigGroup configGroup(config, configGroupName);
        QString babelLanguage = configGroup.readEntry(FileExporterToolchain::keyBabelLanguage, FileExporterToolchain::defaultBabelLanguage);
        row = GUIHelper::selectValue(comboBoxBabelLanguage->model(), babelLanguage);
        comboBoxBabelLanguage->setCurrentIndex(row);
        QString bibliographyStyle = configGroup.readEntry(FileExporterToolchain::keyBibliographyStyle, FileExporterToolchain::defaultBibliographyStyle);
        row = GUIHelper::selectValue(comboBoxBibliographyStyle->model(), bibliographyStyle);
        comboBoxBibliographyStyle->setCurrentIndex(row);
    }

    void saveState() {
        KConfigGroup configGroupGeneral(config, configGroupNameGeneral);
        const QString paperSizeName = paperSizeLabelToName.value(comboBoxPaperSize->currentText(), FileExporter::defaultPaperSize);
        configGroupGeneral.writeEntry(FileExporter::keyPaperSize, paperSizeName);

        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(FileExporterToolchain::keyBabelLanguage, comboBoxBabelLanguage->lineEdit()->text());
        configGroup.writeEntry(FileExporterToolchain::keyBibliographyStyle, comboBoxBibliographyStyle->lineEdit()->text());
        config->sync();
    }

    void resetToDefaults() {
        int row = GUIHelper::selectValue(comboBoxPaperSize->model(), paperSizeLabelToName[FileExporter::defaultPaperSize]);
        comboBoxPaperSize->setCurrentIndex(row);
        row = GUIHelper::selectValue(comboBoxBabelLanguage->model(), FileExporterToolchain::defaultBabelLanguage);
        comboBoxBabelLanguage->setCurrentIndex(row);
        row = GUIHelper::selectValue(comboBoxBibliographyStyle->model(), FileExporterToolchain::defaultBibliographyStyle);
        comboBoxBibliographyStyle->setCurrentIndex(row);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxPaperSize = new KComboBox(false, p);
        comboBoxPaperSize->setObjectName("comboBoxPaperSize");
        layout->addRow(i18n("Paper Size:"), comboBoxPaperSize);
        QStringList paperSizeLabelToNameKeys = paperSizeLabelToName.keys();
        paperSizeLabelToNameKeys.sort();
        foreach(QString labelText, paperSizeLabelToNameKeys) {
            comboBoxPaperSize->addItem(labelText, paperSizeLabelToName[labelText]);
        }
        connect(comboBoxPaperSize, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

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
        foreach(const QString &style, QStringList() << QLatin1String("abbrv") << QLatin1String("alpha") << QLatin1String("plain") << QLatin1String("agsm") << QLatin1String("dcu") << QLatin1String("jmr") << QLatin1String("jphysicsB") << QLatin1String("kluwer") << QLatin1String("nederlands")) {
            comboBoxBibliographyStyle->addItem(style);
        }
        connect(comboBoxBibliographyStyle->lineEdit(), SIGNAL(textChanged(QString)), p, SIGNAL(changed()));
    }
};

SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterPDFPSWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsFileExporterPDFPSWidget::~SettingsFileExporterPDFPSWidget()
{
    delete d;
}

QString SettingsFileExporterPDFPSWidget::label() const
{
    return i18n("PDF & Postscript");
}

KIcon SettingsFileExporterPDFPSWidget::icon() const
{
    return KIcon("application-pdf");
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
