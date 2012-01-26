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
#include <KLineEdit>
#include <KPushButton>
#include <KFileDialog>

#include <fileexporter.h>
#include <lyx.h>
#include <clipboard.h>
#include "settingsfileexporterwidget.h"

class SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate
{
private:
    SettingsFileExporterWidget *p;

    KComboBox *comboBoxPaperSize;
    QMap<QString, QString> paperSizeLabelToName;

    KComboBox *comboBoxCopyReferenceCmd;
    static const QString citeCmdToLabel;

    KSharedConfigPtr config;
    const QString configGroupNameGeneral, configGroupNameLyX;

public:
    KLineEdit *lineEditLyXServerPipeName;

    SettingsFileExporterWidgetPrivate(SettingsFileExporterWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
            configGroupNameGeneral(QLatin1String("General")), configGroupNameLyX(QLatin1String("LyX")) {
        paperSizeLabelToName.insert(i18n("A4"), QLatin1String("a4"));
        paperSizeLabelToName.insert(i18n("Letter"), QLatin1String("letter"));
        paperSizeLabelToName.insert(i18n("Legal"), QLatin1String("legal"));
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupNameGeneral);
        const QString paperSizeName = configGroup.readEntry(FileExporter::keyPaperSize, FileExporter::defaultPaperSize);
        p->selectValue(comboBoxPaperSize, paperSizeLabelToName.key(paperSizeName));

        const QString copyReferenceCommand = configGroup.readEntry(Clipboard::keyCopyReferenceCommand, Clipboard::defaultCopyReferenceCommand);
        p->selectValue(comboBoxCopyReferenceCmd, copyReferenceCommand.isEmpty() ? QString("") : copyReferenceCommand, Qt::UserRole);

        configGroup = KConfigGroup(config, configGroupNameLyX);
        lineEditLyXServerPipeName->setText(configGroup.readEntry(LyX::keyLyXServerPipeName, LyX::defaultLyXServerPipeName));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupNameGeneral);
        const QString paperSizeName = paperSizeLabelToName.value(comboBoxPaperSize->currentText(), FileExporter::defaultPaperSize);
        configGroup.writeEntry(FileExporter::keyPaperSize, paperSizeName);

        const QString copyReferenceCommand = comboBoxCopyReferenceCmd->itemData(comboBoxCopyReferenceCmd->currentIndex()).toString();
        configGroup.writeEntry(Clipboard::keyCopyReferenceCommand, copyReferenceCommand);

        configGroup = KConfigGroup(config, configGroupNameLyX);
        configGroup.writeEntry(LyX::keyLyXServerPipeName, lineEditLyXServerPipeName->text());
        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxPaperSize, paperSizeLabelToName[FileExporter::defaultPaperSize]);
        p->selectValue(comboBoxCopyReferenceCmd, QString(""), Qt::UserRole);
        lineEditLyXServerPipeName->setText(LyX::defaultLyXServerPipeName);
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

        comboBoxCopyReferenceCmd = new KComboBox(false, p);
        comboBoxCopyReferenceCmd->setObjectName("comboBoxCopyReferenceCmd");
        layout->addRow(i18n("Command for \"Copy Reference\":"), comboBoxCopyReferenceCmd);
        ItalicTextItemModel *itim = new ItalicTextItemModel();
        itim->addItem(i18n("No command"), QString(""));
        const QStringList citeCommands = QStringList() << QLatin1String("cite") << QLatin1String("citealt") << QLatin1String("citeauthor") << QLatin1String("citeauthor*") << QLatin1String("citeyear") << QLatin1String("citeyearpar") << QLatin1String("shortcite") << QLatin1String("citet") << QLatin1String("citet*") << QLatin1String("citep") << QLatin1String("citep*"); // TODO more
        foreach(const QString &citeCommand, citeCommands) {
            itim->addItem(citeCmdToLabel.arg(citeCommand), citeCommand);
        }
        comboBoxCopyReferenceCmd->setModel(itim);
        connect(comboBoxCopyReferenceCmd, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

        QBoxLayout *boxLayout = new QHBoxLayout();
        boxLayout->setMargin(0);
        lineEditLyXServerPipeName = new KLineEdit(p);
        lineEditLyXServerPipeName->setReadOnly(true);
        boxLayout->addWidget(lineEditLyXServerPipeName);
        KPushButton *buttonBrowseForPipeName = new KPushButton(KIcon("network-connect"), "", p);
        boxLayout->addWidget(buttonBrowseForPipeName);
        layout->addRow(i18n("LyX Server Pipe"), boxLayout);
        connect(buttonBrowseForPipeName, SIGNAL(clicked()), p, SLOT(selectPipeName()));
        connect(lineEditLyXServerPipeName, SIGNAL(textChanged(QString)), p, SIGNAL(changed()));
    }
};

const QString SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate::citeCmdToLabel = QLatin1String("\\%1{") + QChar(0x2026) + QChar('}');

SettingsFileExporterWidget::SettingsFileExporterWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsFileExporterWidget::~SettingsFileExporterWidget()
{
    delete d;
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

void SettingsFileExporterWidget::selectPipeName()
{
    QString pipeName = KFileDialog::getOpenFileName(QDir::homePath(), QLatin1String("inode/fifo"), this, i18n("Select LyX Server Pipe"));
    if (!pipeName.isEmpty())
        d->lineEditLyXServerPipeName->setText(pipeName);
}
