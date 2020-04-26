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

#include "settingsfileexporterpdfpswidget.h"

#include <QFormLayout>
#include <QPageSize>
#include <QLineEdit>
#include <QComboBox>

#include <KLocalizedString>

#include <Preferences>
#include <FileExporterToolchain>
#include "guihelper.h"

class SettingsFileExporterPDFPSWidget::SettingsFileExporterPDFPSWidgetPrivate
{
private:
    SettingsFileExporterPDFPSWidget *p;

    QComboBox *comboBoxPaperSize;

    QComboBox *comboBoxBabelLanguage;
    QComboBox *comboBoxBibliographyStyle;

public:

    SettingsFileExporterPDFPSWidgetPrivate(SettingsFileExporterPDFPSWidget *parent)
            : p(parent)
    {
        setupGUI();
    }

    void loadState() {
        int row = qMax(0, GUIHelper::selectValue(comboBoxPaperSize->model(), static_cast<int>(Preferences::instance().pageSize()), Qt::UserRole));
        comboBoxPaperSize->setCurrentIndex(row);

        const QString babelLanguage = Preferences::instance().laTeXBabelLanguage();
        row = qMax(0, GUIHelper::selectValue(comboBoxBabelLanguage->model(), babelLanguage));
        comboBoxBabelLanguage->setCurrentIndex(row);
        const QString bibliographyStyle = Preferences::instance().bibTeXBibliographyStyle();
        row = qMax(0, GUIHelper::selectValue(comboBoxBibliographyStyle->model(), bibliographyStyle));
        comboBoxBibliographyStyle->setCurrentIndex(row);
    }

    bool saveState() {
        bool settingsGotChanged = false;
        settingsGotChanged |= Preferences::instance().setPageSize(static_cast<QPageSize::PageSizeId>(comboBoxPaperSize->currentData().toInt()));
        settingsGotChanged |= Preferences::instance().setLaTeXBabelLanguage(comboBoxBabelLanguage->lineEdit()->text());
        settingsGotChanged |= Preferences::instance().setBibTeXBibliographyStyle(comboBoxBibliographyStyle->lineEdit()->text());
        return settingsGotChanged;
    }

    void resetToDefaults() {
        int row = qMax(0, GUIHelper::selectValue(comboBoxPaperSize->model(), static_cast<int>(Preferences::defaultPageSize), Qt::UserRole));
        comboBoxPaperSize->setCurrentIndex(row);
        row = qMax(0, GUIHelper::selectValue(comboBoxBabelLanguage->model(), Preferences::defaultLaTeXBabelLanguage));
        comboBoxBabelLanguage->setCurrentIndex(row);
        row = qMax(0, GUIHelper::selectValue(comboBoxBibliographyStyle->model(), Preferences::defaultBibTeXBibliographyStyle));
        comboBoxBibliographyStyle->setCurrentIndex(row);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxPaperSize = new QComboBox(p);
        comboBoxPaperSize->setObjectName(QStringLiteral("comboBoxPaperSize"));
        layout->addRow(i18n("Paper Size:"), comboBoxPaperSize);
        for (const auto &dbItem : Preferences::availablePageSizes)
            comboBoxPaperSize->addItem(QPageSize::name(dbItem.first), dbItem.second);
        connect(comboBoxPaperSize, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsAbstractWidget::changed);

        comboBoxBabelLanguage = new QComboBox(p);
        comboBoxBabelLanguage->setObjectName(QStringLiteral("comboBoxBabelLanguage"));
        comboBoxBabelLanguage->setEditable(true);
        layout->addRow(i18n("Language for 'babel':"), comboBoxBabelLanguage);
        comboBoxBabelLanguage->addItem(QStringLiteral("english"));
        comboBoxBabelLanguage->addItem(QStringLiteral("ngerman"));
        comboBoxBabelLanguage->addItem(QStringLiteral("swedish"));
        connect(comboBoxBabelLanguage->lineEdit(), &QLineEdit::textChanged, p, &SettingsFileExporterPDFPSWidget::changed);

        comboBoxBibliographyStyle = new QComboBox(p);
        comboBoxBibliographyStyle->setObjectName(QStringLiteral("comboBoxBibliographyStyle"));
        comboBoxBibliographyStyle->setEditable(true);
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

bool SettingsFileExporterPDFPSWidget::saveState()
{
    return d->saveState();
}

void SettingsFileExporterPDFPSWidget::resetToDefaults()
{
    d->resetToDefaults();
}
