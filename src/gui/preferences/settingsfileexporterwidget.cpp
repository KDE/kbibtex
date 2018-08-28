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

#include "settingsfileexporterwidget.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <qplatformdefs.h>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>
#include <KLineEdit>
#include <KUrlRequester>

#include "guihelper.h"
#include "italictextitemmodel.h"
#include "fileexporter.h"
#include "clipboard.h"
#include "preferences.h"
#include "lyx.h"

class SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate
{
private:
    SettingsFileExporterWidget *p;

    KComboBox *comboBoxCopyReferenceCmd;
    static const QString citeCmdToLabel;

    KSharedConfigPtr config;

public:
#ifdef QT_LSTAT
    QCheckBox *checkboxUseAutomaticLyXPipeDetection;
#endif // QT_LSTAT
    KComboBox *comboBoxBackupScope;
    QSpinBox *spinboxNumberOfBackups;

    KUrlRequester *lineeditLyXPipePath;
#ifdef QT_LSTAT
    QString lastUserInputLyXPipePath;
#endif // QT_LSTAT

    SettingsFileExporterWidgetPrivate(SettingsFileExporterWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))) {
        setupGUI();
    }

    void loadState() {
        KConfigGroup configGroup(config, Preferences::groupGeneral);
        const QString copyReferenceCommand = configGroup.readEntry(Clipboard::keyCopyReferenceCommand, Clipboard::defaultCopyReferenceCommand);
        int row = GUIHelper::selectValue(comboBoxCopyReferenceCmd->model(), copyReferenceCommand.isEmpty() ? QString() : copyReferenceCommand, ItalicTextItemModel::IdentifierRole);
        comboBoxCopyReferenceCmd->setCurrentIndex(row);

        const int index = qMax(0, comboBoxBackupScope->findData(configGroup.readEntry(Preferences::keyBackupScope, static_cast<int>(Preferences::defaultBackupScope))));
        comboBoxBackupScope->setCurrentIndex(index);
        spinboxNumberOfBackups->setValue(qMax(0, qMin(spinboxNumberOfBackups->maximum(), configGroup.readEntry(Preferences::keyNumberOfBackups, Preferences::defaultNumberOfBackups))));

        KConfigGroup configGroupLyX(config, LyX::configGroupName);
#ifndef QT_LSTAT
        lineeditLyXPipePath->setText(configGroupLyX.readEntry(LyX::keyLyXPipePath, LyX::defaultLyXPipePath));
#else // QT_LSTAT
        checkboxUseAutomaticLyXPipeDetection->setChecked(configGroupLyX.readEntry(LyX::keyUseAutomaticLyXPipeDetection, LyX::defaultUseAutomaticLyXPipeDetection));
        lastUserInputLyXPipePath = configGroupLyX.readEntry(LyX::keyLyXPipePath, LyX::defaultLyXPipePath);
        p->automaticLyXDetectionToggled(checkboxUseAutomaticLyXPipeDetection->isChecked());
#endif // QT_LSTAT
    }

    void saveState() {
        KConfigGroup configGroup(config, Preferences::groupGeneral);
        const QString copyReferenceCommand = comboBoxCopyReferenceCmd->itemData(comboBoxCopyReferenceCmd->currentIndex(), ItalicTextItemModel::IdentifierRole).toString();
        configGroup.writeEntry(Clipboard::keyCopyReferenceCommand, copyReferenceCommand);

        configGroup.writeEntry(Preferences::keyBackupScope, comboBoxBackupScope->itemData(comboBoxBackupScope->currentIndex()).toInt());
        configGroup.writeEntry(Preferences::keyNumberOfBackups, spinboxNumberOfBackups->value());

        KConfigGroup configGroupLyX(config, LyX::configGroupName);
#ifndef QT_LSTAT
        configGroupLyX.writeEntry(LyX::keyLyXPipePath, lineeditLyXPipePath->text());
#else // QT_LSTAT
        configGroupLyX.writeEntry(LyX::keyUseAutomaticLyXPipeDetection, checkboxUseAutomaticLyXPipeDetection->isChecked());
        configGroupLyX.writeEntry(LyX::keyLyXPipePath, checkboxUseAutomaticLyXPipeDetection->isChecked() ? lastUserInputLyXPipePath : lineeditLyXPipePath->text());
#endif // QT_LSTAT

        config->sync();
    }

    void resetToDefaults() {
        int row = GUIHelper::selectValue(comboBoxCopyReferenceCmd->model(), QString(), Qt::UserRole);
        comboBoxCopyReferenceCmd->setCurrentIndex(row);

        const int index = qMax(0, comboBoxBackupScope->findData(Preferences::defaultBackupScope));
        comboBoxBackupScope->setCurrentIndex(index);
        spinboxNumberOfBackups->setValue(qMax(0, qMin(spinboxNumberOfBackups->maximum(), Preferences::defaultNumberOfBackups)));

#ifndef QT_LSTAT
        const QString pipe = LyX::defaultLyXPipePath;
#else // QT_LSTAT
        checkboxUseAutomaticLyXPipeDetection->setChecked(LyX::defaultUseAutomaticLyXPipeDetection);
        QString pipe = LyX::guessLyXPipeLocation();
        if (pipe.isEmpty()) pipe = LyX::defaultLyXPipePath;
#endif // QT_LSTAT
        lineeditLyXPipePath->setText(pipe);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxCopyReferenceCmd = new KComboBox(false, p);
        comboBoxCopyReferenceCmd->setObjectName(QStringLiteral("comboBoxCopyReferenceCmd"));
        layout->addRow(i18n("Command for 'Copy Reference':"), comboBoxCopyReferenceCmd);
        ItalicTextItemModel *itim = new ItalicTextItemModel();
        itim->addItem(i18n("No command"), QString());
        static const QStringList citeCommands {QStringLiteral("cite"), QStringLiteral("citealt"), QStringLiteral("citeauthor"), QStringLiteral("citeauthor*"), QStringLiteral("citeyear"), QStringLiteral("citeyearpar"), QStringLiteral("shortcite"), QStringLiteral("citet"), QStringLiteral("citet*"), QStringLiteral("citep"), QStringLiteral("citep*")}; // TODO more
        for (const QString &citeCommand : citeCommands) {
            itim->addItem(citeCmdToLabel.arg(citeCommand), citeCommand);
        }
        comboBoxCopyReferenceCmd->setModel(itim);
        connect(comboBoxCopyReferenceCmd, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsFileExporterWidget::changed);

#ifdef QT_LSTAT
        checkboxUseAutomaticLyXPipeDetection = new QCheckBox(QStringLiteral(""), p);
        layout->addRow(i18n("Detect LyX pipe automatically:"), checkboxUseAutomaticLyXPipeDetection);
        connect(checkboxUseAutomaticLyXPipeDetection, &QCheckBox::toggled, p, &SettingsFileExporterWidget::changed);
        connect(checkboxUseAutomaticLyXPipeDetection, &QCheckBox::toggled, p, &SettingsFileExporterWidget::automaticLyXDetectionToggled);
#endif // QT_LSTAT

        lineeditLyXPipePath = new KUrlRequester(p);
        layout->addRow(i18n("Manually specified LyX pipe:"), lineeditLyXPipePath);
        connect(lineeditLyXPipePath->lineEdit(), &KLineEdit::textEdited, p, &SettingsFileExporterWidget::changed);
        lineeditLyXPipePath->setMinimumWidth(lineeditLyXPipePath->fontMetrics().width(QChar('W')) * 20);
        lineeditLyXPipePath->setFilter(QStringLiteral("inode/fifo"));
        lineeditLyXPipePath->setMode(KFile::ExistingOnly | KFile::LocalOnly);

        comboBoxBackupScope = new KComboBox(false, p);
        comboBoxBackupScope->addItem(i18n("No backups"), Preferences::NoBackup);
        comboBoxBackupScope->addItem(i18n("Local files only"), Preferences::LocalOnly);
        comboBoxBackupScope->addItem(i18n("Both local and remote files"), Preferences::BothLocalAndRemote);
        layout->addRow(i18n("Backups when saving:"), comboBoxBackupScope);
        connect(comboBoxBackupScope, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsFileExporterWidget::changed);

        spinboxNumberOfBackups = new QSpinBox(p);
        spinboxNumberOfBackups->setMinimum(1);
        spinboxNumberOfBackups->setMaximum(16);
        layout->addRow(i18n("Number of Backups:"), spinboxNumberOfBackups);
        connect(spinboxNumberOfBackups, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), p, &SettingsFileExporterWidget::changed);

        connect(comboBoxBackupScope, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsFileExporterWidget::updateGUI);
    }
};

const QString SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate::citeCmdToLabel = QStringLiteral("\\%1{") + QChar(0x2026) + QChar('}');

SettingsFileExporterWidget::SettingsFileExporterWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterWidgetPrivate(this))
{
    d->loadState();
}

SettingsFileExporterWidget::~SettingsFileExporterWidget()
{
    delete d;
}

QString SettingsFileExporterWidget::label() const
{
    return i18n("Saving and Exporting");
}

QIcon SettingsFileExporterWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("document-save"));
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

#ifdef QT_LSTAT
void SettingsFileExporterWidget::automaticLyXDetectionToggled(bool isChecked)
{
    d->lineeditLyXPipePath->setEnabled(!isChecked);
    if (isChecked) {
        d->lastUserInputLyXPipePath = d->lineeditLyXPipePath->text();
        d->lineeditLyXPipePath->setText(LyX::guessLyXPipeLocation());
    } else
        d->lineeditLyXPipePath->setText(d->lastUserInputLyXPipePath);
}
#endif // QT_LSTAT

void SettingsFileExporterWidget::updateGUI()
{
    d->spinboxNumberOfBackups->setEnabled(d->comboBoxBackupScope->itemData(d->comboBoxBackupScope->currentIndex()).toInt() != static_cast<int>(Preferences::NoBackup));
}
