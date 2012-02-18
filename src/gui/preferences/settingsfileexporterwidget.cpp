/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>
#include <KLineEdit>
#include <KPushButton>
#include <KFileDialog>
#include <KUrlRequester>

#include <fileexporter.h>
#include <clipboard.h>
#include "lyx.h"
#include "settingsfileexporterwidget.h"

class SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate
{
private:
    SettingsFileExporterWidget *p;

    KComboBox *comboBoxCopyReferenceCmd;
    static const QString citeCmdToLabel;

    KSharedConfigPtr config;
    const QString configGroupNameGeneral;

public:
    QCheckBox *checkboxUseAutomaticLyXPipeDetection;
    KUrlRequester *lineeditLyXPipePath;
    QString lastUserInput;

    SettingsFileExporterWidgetPrivate(SettingsFileExporterWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
            configGroupNameGeneral(QLatin1String("General")) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupNameGeneral);
        const QString copyReferenceCommand = configGroup.readEntry(Clipboard::keyCopyReferenceCommand, Clipboard::defaultCopyReferenceCommand);
        p->selectValue(comboBoxCopyReferenceCmd, copyReferenceCommand.isEmpty() ? QString("") : copyReferenceCommand, Qt::UserRole);

        KConfigGroup configGroupLyX(config, LyX::configGroupName);
        checkboxUseAutomaticLyXPipeDetection->setChecked(configGroupLyX.readEntry(LyX::keyUseAutomaticLyXPipeDetection, LyX::defaultUseAutomaticLyXPipeDetection));
        lastUserInput = configGroupLyX.readEntry(LyX::keyLyXPipePath, LyX::defaultLyXPipePath);
        p->automaticLyXDetectionToggled(checkboxUseAutomaticLyXPipeDetection->isChecked());
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupNameGeneral);
        const QString copyReferenceCommand = comboBoxCopyReferenceCmd->itemData(comboBoxCopyReferenceCmd->currentIndex()).toString();
        configGroup.writeEntry(Clipboard::keyCopyReferenceCommand, copyReferenceCommand);

        KConfigGroup configGroupLyX(config, LyX::configGroupName);
        configGroupLyX.writeEntry(LyX::keyUseAutomaticLyXPipeDetection, checkboxUseAutomaticLyXPipeDetection->isChecked());
        configGroupLyX.writeEntry(LyX::keyLyXPipePath, checkboxUseAutomaticLyXPipeDetection->isChecked() ? lastUserInput : lineeditLyXPipePath->text());

        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxCopyReferenceCmd, QString(""), Qt::UserRole);
        checkboxUseAutomaticLyXPipeDetection->setChecked(LyX::defaultUseAutomaticLyXPipeDetection);
        QString pipe = LyX::guessLyXPipeLocation();
        if (pipe.isEmpty()) pipe = LyX::defaultLyXPipePath;
        lineeditLyXPipePath->setText(pipe);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

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

        checkboxUseAutomaticLyXPipeDetection = new QCheckBox(QLatin1String(""), p);
        layout->addRow(i18n("Detect LyX pipe automatically:"), checkboxUseAutomaticLyXPipeDetection);
        connect(checkboxUseAutomaticLyXPipeDetection, SIGNAL(toggled(bool)), p, SIGNAL(changed()));
        connect(checkboxUseAutomaticLyXPipeDetection, SIGNAL(toggled(bool)), p, SLOT(automaticLyXDetectionToggled(bool)));

        lineeditLyXPipePath = new KUrlRequester(p);
        layout->addRow(i18n("Manually specified LyX pipe:"), lineeditLyXPipePath);
        connect(lineeditLyXPipePath->lineEdit(), SIGNAL(textEdited(QString)), p, SIGNAL(changed()));
        lineeditLyXPipePath->setMinimumWidth(lineeditLyXPipePath->fontMetrics().width(QChar('W')) * 20);
        lineeditLyXPipePath->setFilter(QLatin1String("inode/fifo"));
        lineeditLyXPipePath->setMode(KFile::ExistingOnly | KFile::LocalOnly);
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

void SettingsFileExporterWidget::automaticLyXDetectionToggled(bool isChecked)
{
    d->lineeditLyXPipePath->setEnabled(!isChecked);
    if (isChecked) {
        d->lastUserInput = d->lineeditLyXPipePath->text();
        d->lineeditLyXPipePath->setText(LyX::guessLyXPipeLocation());
    } else
        d->lineeditLyXPipePath->setText(d->lastUserInput);
}
