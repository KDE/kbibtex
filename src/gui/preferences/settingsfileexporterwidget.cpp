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
#include <clipboard.h>
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
    SettingsFileExporterWidgetPrivate(SettingsFileExporterWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
            configGroupNameGeneral(QLatin1String("General")) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupNameGeneral);
        const QString copyReferenceCommand = configGroup.readEntry(Clipboard::keyCopyReferenceCommand, Clipboard::defaultCopyReferenceCommand);
        p->selectValue(comboBoxCopyReferenceCmd, copyReferenceCommand.isEmpty() ? QString("") : copyReferenceCommand, Qt::UserRole);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupNameGeneral);
        const QString copyReferenceCommand = comboBoxCopyReferenceCmd->itemData(comboBoxCopyReferenceCmd->currentIndex()).toString();
        configGroup.writeEntry(Clipboard::keyCopyReferenceCommand, copyReferenceCommand);

        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxCopyReferenceCmd, QString(""), Qt::UserRole);
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
    }
};

const QString SettingsFileExporterWidget::SettingsFileExporterWidgetPrivate::citeCmdToLabel = QLatin1String("\\%1{") + QChar(0x2026) + QChar('}');

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
