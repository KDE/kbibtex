/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <qplatformdefs.h>

#include <KLocalizedString>
#include <KUrlRequester>
#include <KLineEdit> /// required as KUrlRequester returns it

#include <kio_version.h>
#include <Preferences>
#include <FileExporter>
#include <LyX>
#include "guihelper.h"
#include "italictextitemmodel.h"

class SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate
{
private:
    SettingsFileExporterWidget *p;

    QComboBox *comboBoxCopyReferenceCmd;
    static const QString citeCmdToLabel;

public:
#ifdef QT_LSTAT
    QCheckBox *checkboxUseAutomaticLyXPipeDetection;
#endif // QT_LSTAT
    QComboBox *comboBoxBackupScope;
    QSpinBox *spinboxNumberOfBackups;

    KUrlRequester *lineeditLyXPipePath;
#ifdef QT_LSTAT
    QString lastUserInputLyXPipePath;
#endif // QT_LSTAT

    SettingsFileExporterWidgetPrivate(SettingsFileExporterWidget *parent)
            : p(parent) {
        setupGUI();
    }

    void loadState() {
        int row = GUIHelper::selectValue(comboBoxCopyReferenceCmd->model(), Preferences::instance().copyReferenceCommand(), ItalicTextItemModel::IdentifierRole);
        comboBoxCopyReferenceCmd->setCurrentIndex(row);

        const int index = qMax(0, comboBoxBackupScope->findData(static_cast<int>(Preferences::instance().backupScope())));
        comboBoxBackupScope->setCurrentIndex(index);
        spinboxNumberOfBackups->setValue(qMax(0, qMin(spinboxNumberOfBackups->maximum(), Preferences::instance().numberOfBackups())));

#ifndef QT_LSTAT
        lineeditLyXPipePath->setText(Preferences::instance().lyXPipePath());
#else // QT_LSTAT
        checkboxUseAutomaticLyXPipeDetection->setChecked(Preferences::instance().lyXUseAutomaticPipeDetection());
        lastUserInputLyXPipePath = Preferences::instance().lyXPipePath();
        lineeditLyXPipePath->setText(lastUserInputLyXPipePath);
        p->automaticLyXDetectionToggled(checkboxUseAutomaticLyXPipeDetection->isChecked());
#endif // QT_LSTAT
    }

    bool saveState() {
        bool settingsGotChanged = false;
        settingsGotChanged |= Preferences::instance().setCopyReferenceCommand(comboBoxCopyReferenceCmd->itemData(comboBoxCopyReferenceCmd->currentIndex(), ItalicTextItemModel::IdentifierRole).toString());
        settingsGotChanged |= Preferences::instance().setBackupScope(static_cast<Preferences::BackupScope>(comboBoxBackupScope->itemData(comboBoxBackupScope->currentIndex()).toInt()));
        settingsGotChanged |= Preferences::instance().setNumberOfBackups(spinboxNumberOfBackups->value());

#ifndef QT_LSTAT
        settingsGotChanged |= Preferences::instance().setLyXPipePath(lineeditLyXPipePath->text());
#else // QT_LSTAT
        settingsGotChanged |= Preferences::instance().setLyXUseAutomaticPipeDetection(checkboxUseAutomaticLyXPipeDetection->isChecked());
        settingsGotChanged |= Preferences::instance().setLyXPipePath(checkboxUseAutomaticLyXPipeDetection->isChecked() ? lastUserInputLyXPipePath : lineeditLyXPipePath->text());
#endif // QT_LSTAT

        return settingsGotChanged;
    }

    void resetToDefaults() {
        int row = GUIHelper::selectValue(comboBoxCopyReferenceCmd->model(), QString(), Qt::UserRole);
        comboBoxCopyReferenceCmd->setCurrentIndex(row);

        const int index = qMax(0, comboBoxBackupScope->findData(static_cast<int>(Preferences::defaultBackupScope)));
        comboBoxBackupScope->setCurrentIndex(index);
        spinboxNumberOfBackups->setValue(qMax(0, qMin(spinboxNumberOfBackups->maximum(), Preferences::defaultNumberOfBackups)));

#ifndef QT_LSTAT
        const QString pipe = Preferences::defaultLyXPipePath;
#else // QT_LSTAT
        checkboxUseAutomaticLyXPipeDetection->setChecked(Preferences::defaultLyXUseAutomaticPipeDetection);
        QString pipe = LyX::guessLyXPipeLocation();
        if (pipe.isEmpty()) pipe = Preferences::defaultLyXPipePath;
#endif // QT_LSTAT
        lineeditLyXPipePath->setText(pipe);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxCopyReferenceCmd = new QComboBox(p);
        comboBoxCopyReferenceCmd->setObjectName(QStringLiteral("comboBoxCopyReferenceCmd"));
        layout->addRow(i18n("Command for 'Copy Reference':"), comboBoxCopyReferenceCmd);
        ItalicTextItemModel *itim = new ItalicTextItemModel(comboBoxCopyReferenceCmd);
        itim->addItem(i18n("No command"), QString());
        for (const QString &citeCommand : Preferences::availableCopyReferenceCommands)
            itim->addItem(citeCmdToLabel.arg(citeCommand), citeCommand);
        comboBoxCopyReferenceCmd->setModel(itim);
        connect(comboBoxCopyReferenceCmd, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsFileExporterWidget::changed);

#ifdef QT_LSTAT
        checkboxUseAutomaticLyXPipeDetection = new QCheckBox(QString(), p);
        layout->addRow(i18n("Detect LyX pipe automatically:"), checkboxUseAutomaticLyXPipeDetection);
        connect(checkboxUseAutomaticLyXPipeDetection, &QCheckBox::toggled, p, &SettingsFileExporterWidget::changed);
        connect(checkboxUseAutomaticLyXPipeDetection, &QCheckBox::toggled, p, &SettingsFileExporterWidget::automaticLyXDetectionToggled);
#endif // QT_LSTAT

        lineeditLyXPipePath = new KUrlRequester(p);
        layout->addRow(i18n("Manually specified LyX pipe:"), lineeditLyXPipePath);
        connect(qobject_cast<QLineEdit *>(lineeditLyXPipePath->lineEdit()), &QLineEdit::textEdited, p, &SettingsFileExporterWidget::changed);
#if QT_VERSION >= 0x050b00
        lineeditLyXPipePath->setMinimumWidth(lineeditLyXPipePath->fontMetrics().horizontalAdvance(QLatin1Char('W')) * 20);
#else // QT_VERSION >= 0x050b00
        lineeditLyXPipePath->setMinimumWidth(lineeditLyXPipePath->fontMetrics().width(QLatin1Char('W')) * 20);
#endif // QT_VERSION >= 0x050b00
#if KIO_VERSION < QT_VERSION_CHECK(5, 108, 0)
        lineeditLyXPipePath->setFilter(QStringLiteral("inode/fifo"));
#else // KIO_VERSION < QT_VERSION_CHECK(5, 108, 0) // >= 5.108.0
        lineeditLyXPipePath->setMimeTypeFilters({QStringLiteral("inode/fifo")});
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 108, 0)
        lineeditLyXPipePath->setMode(KFile::ExistingOnly | KFile::LocalOnly);

        comboBoxBackupScope = new QComboBox(p);
        for (const auto &pair : Preferences::availableBackupScopes)
            comboBoxBackupScope->addItem(pair.second, static_cast<int>(pair.first));
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

const QString SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate::citeCmdToLabel = QStringLiteral("\\%1{") + QChar(0x2026) + QStringLiteral("}");

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

bool SettingsFileExporterWidget::saveState()
{
    return d->saveState();
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
    d->spinboxNumberOfBackups->setEnabled(d->comboBoxBackupScope->itemData(d->comboBoxBackupScope->currentIndex()).toInt() != static_cast<int>(Preferences::BackupScope::None));
}
