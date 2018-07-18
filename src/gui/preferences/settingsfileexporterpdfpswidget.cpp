/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "settingsfileexporterpdfpswidget.h"

#include <QFormLayout>
#include <KLineEdit>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>
#include <KLocalizedString>

#include "guihelper.h"
#include "fileexportertoolchain.h"

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
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("FileExporterPDFPS")), configGroupNameGeneral(QStringLiteral("General")) {
        paperSizeLabelToName.insert(i18n("A4"), QStringLiteral("a4"));
        paperSizeLabelToName.insert(i18n("Letter"), QStringLiteral("letter"));
        paperSizeLabelToName.insert(i18n("Legal"), QStringLiteral("legal"));

        setupGUI();
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
        int row = GUIHelper::selectValue(comboBoxPaperSize->model(), FileExporter::defaultPaperSize);
        comboBoxPaperSize->setCurrentIndex(row);
        row = GUIHelper::selectValue(comboBoxBabelLanguage->model(), FileExporterToolchain::defaultBabelLanguage);
        comboBoxBabelLanguage->setCurrentIndex(row);
        row = GUIHelper::selectValue(comboBoxBibliographyStyle->model(), FileExporterToolchain::defaultBibliographyStyle);
        comboBoxBibliographyStyle->setCurrentIndex(row);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxPaperSize = new KComboBox(false, p);
        comboBoxPaperSize->setObjectName(QStringLiteral("comboBoxPaperSize"));
        layout->addRow(i18n("Paper Size:"), comboBoxPaperSize);
        QStringList paperSizeLabelToNameKeys = paperSizeLabelToName.keys();
        paperSizeLabelToNameKeys.sort();
        for (const QString &labelText : const_cast<const QStringList &>(paperSizeLabelToNameKeys)) {
            comboBoxPaperSize->addItem(labelText, paperSizeLabelToName[labelText]);
        }
        connect(comboBoxPaperSize, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsAbstractWidget::changed);

        comboBoxBabelLanguage = new KComboBox(true, p);
        comboBoxBabelLanguage->setObjectName(QStringLiteral("comboBoxBabelLanguage"));
        layout->addRow(i18n("Language for 'babel':"), comboBoxBabelLanguage);
        comboBoxBabelLanguage->addItem(QStringLiteral("english"));
        comboBoxBabelLanguage->addItem(QStringLiteral("ngerman"));
        comboBoxBabelLanguage->addItem(QStringLiteral("swedish"));
        connect(comboBoxBabelLanguage->lineEdit(), &QLineEdit::textChanged, p, &SettingsFileExporterPDFPSWidget::changed);

        comboBoxBibliographyStyle = new KComboBox(true, p);
        comboBoxBibliographyStyle->setObjectName(QStringLiteral("comboBoxBibliographyStyle"));
        layout->addRow(i18n("Bibliography style:"), comboBoxBibliographyStyle);
        static const QStringList styles {QString(QStringLiteral("abbrv")), QString(QStringLiteral("alpha")), QString(QStringLiteral("plain")), QString(QStringLiteral("agsm")), QString(QStringLiteral("dcu")), QString(QStringLiteral("jmr")), QString(QStringLiteral("jphysicsB")), QString(QStringLiteral("kluwer")), QString(QStringLiteral("nederlands"))};
        for (const QString &style : styles) {
            comboBoxBibliographyStyle->addItem(style);
        }
        connect(comboBoxBibliographyStyle->lineEdit(), &QLineEdit::textChanged, p, &SettingsFileExporterPDFPSWidget::changed);
    }
};

SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterPDFPSWidgetPrivate(this))
{
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

QIcon SettingsFileExporterPDFPSWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("application-pdf"));
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
