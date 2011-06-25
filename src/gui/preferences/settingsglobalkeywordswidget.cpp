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

#include <QLayout>
#include <QStringListModel>
#include <QListView>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KPushButton>
#include <KLocale>
#include <KInputDialog>

#include <fieldlistedit.h>
#include "settingsglobalkeywordswidget.h"

class SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidgetPrivate
{
private:
    SettingsGlobalKeywordsWidget *p;

    KSharedConfigPtr config;
    const QString configGroupName;

public:
    QListView *listViewKeywords;
    QStringListModel stringListModel;
    KPushButton *buttonRemove;

    SettingsGlobalKeywordsWidgetPrivate(SettingsGlobalKeywordsWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("Global Keywords")) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QStringList keywordList = configGroup.readEntry(KeywordListEdit::keyGlobalKeywordList, QStringList());
        stringListModel.setStringList(keywordList);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(KeywordListEdit::keyGlobalKeywordList, stringListModel.stringList());
        config->sync();
    }

    void resetToDefaults() {
        /// ignored, you don't want to delete all your keywords ...
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setMargin(0);

        listViewKeywords = new QListView(p);
        layout->addWidget(listViewKeywords, 0, 0, 3, 1);
        listViewKeywords->setModel(&stringListModel);
        connect(listViewKeywords, SIGNAL(pressed(QModelIndex)), p, SLOT(enableRemoveButton()));

        KPushButton *buttonAdd = new KPushButton(KIcon("list-add"), i18n("Add ..."), p);
        layout->addWidget(buttonAdd, 0, 1, 1, 1);
        connect(buttonAdd, SIGNAL(clicked()), p, SLOT(addKeywordDialog()));

        buttonRemove = new KPushButton(KIcon("list-remove"), i18n("Remove"), p);
        layout->addWidget(buttonRemove, 1, 1, 1, 1);
        buttonRemove->setEnabled(false);
        connect(buttonRemove, SIGNAL(clicked()), p, SLOT(removeKeyword()));
    }
};


SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsGlobalKeywordsWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

void SettingsGlobalKeywordsWidget::loadState()
{
    d->loadState();
}

void SettingsGlobalKeywordsWidget::saveState()
{
    d->saveState();
}

void SettingsGlobalKeywordsWidget::resetToDefaults()
{
    d->resetToDefaults();
}

void SettingsGlobalKeywordsWidget::addKeywordDialog()
{
    bool ok = false;
    QString newKeyword = KInputDialog::getText(i18n("New Keyword"), i18n("Enter a new keyword:"), QLatin1String(""), &ok, this);
    if (ok && !d->stringListModel.stringList().contains(newKeyword)) {
        QStringList list = d->stringListModel.stringList();
        list.append(newKeyword);
        list.sort();
        d->stringListModel.setStringList(list);
    }
}

void SettingsGlobalKeywordsWidget::removeKeyword()
{
    QString keyword = d->stringListModel.data(d->listViewKeywords->selectionModel()->selectedIndexes().first(), Qt::DisplayRole).toString();
    QStringList list = d->stringListModel.stringList();
    list.removeOne(keyword);
    d->stringListModel.setStringList(list);
    d->buttonRemove->setEnabled(false);
}

void SettingsGlobalKeywordsWidget::enableRemoveButton()
{
    d->buttonRemove->setEnabled(!d->listViewKeywords->selectionModel()->selectedIndexes().isEmpty());
}
