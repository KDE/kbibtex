/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "settingsuserinterfacewidget.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QBoxLayout>

#include <KLocalizedString>

#include <Preferences>
#include "element/elementwidgets.h"
#include "guihelper.h"

class SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate
{
private:
    SettingsUserInterfaceWidget *p;

    QComboBox *comboBoxElementDoubleClickAction;

public:
    SettingsUserInterfaceWidgetPrivate(SettingsUserInterfaceWidget *parent)
            : p(parent) {
        setupGUI();
    }

    void loadState() {
        const int row = qMax(0, GUIHelper::selectValue(comboBoxElementDoubleClickAction->model(), static_cast<int>(Preferences::instance().fileViewDoubleClickAction()), Qt::UserRole));
        comboBoxElementDoubleClickAction->setCurrentIndex(row);
    }

    void saveState() {
        Preferences::instance().setFileViewDoubleClickAction(static_cast<Preferences::FileViewDoubleClickAction>(comboBoxElementDoubleClickAction->currentData().toInt()));
    }

    void resetToDefaults() {
        const int row = qMax(0, GUIHelper::selectValue(comboBoxElementDoubleClickAction->model(), static_cast<int>(Preferences::defaultFileViewDoubleClickAction), Qt::UserRole));
        comboBoxElementDoubleClickAction->setCurrentIndex(row);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxElementDoubleClickAction = new QComboBox(p);
        comboBoxElementDoubleClickAction->setObjectName(QStringLiteral("comboBoxElementDoubleClickAction"));
        for (QVector<QPair<Preferences::FileViewDoubleClickAction, QString>>::ConstIterator it = Preferences::availableFileViewDoubleClickActions.constBegin(); it != Preferences::availableFileViewDoubleClickActions.constEnd(); ++it)
            comboBoxElementDoubleClickAction->addItem(it->second, static_cast<int>(it->first));
        layout->addRow(i18n("When double-clicking an element:"), comboBoxElementDoubleClickAction);
        connect(comboBoxElementDoubleClickAction, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsUserInterfaceWidget::changed);
    }
};


SettingsUserInterfaceWidget::SettingsUserInterfaceWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsUserInterfaceWidgetPrivate(this))
{
    d->loadState();
}

QString SettingsUserInterfaceWidget::label() const
{
    return i18n("User Interface");
}

QIcon SettingsUserInterfaceWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("user-identity"));
}

SettingsUserInterfaceWidget::~SettingsUserInterfaceWidget()
{
    delete d;
}

void SettingsUserInterfaceWidget::loadState()
{
    d->loadState();
}

void SettingsUserInterfaceWidget::saveState()
{
    d->saveState();
}

void SettingsUserInterfaceWidget::resetToDefaults()
{
    d->resetToDefaults();
}
