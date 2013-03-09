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

class DisallowEmptyStringListModel : public QStringListModel
{
public:
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        if (role == Qt::EditRole && value.canConvert<QString>() && value.toString().isEmpty())
            return false; /// do not accept values that are empty
        else
            return QStringListModel::setData(index, value, role);
    }
};

class SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidgetPrivate
{
private:
    SettingsGlobalKeywordsWidget *p;

    KSharedConfigPtr config;
    const QString configGroupName;

public:
    QListView *listViewKeywords;
    DisallowEmptyStringListModel stringListModel;
    KPushButton *buttonRemove;
    static int keywordCounter;

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

        KPushButton *buttonAdd = new KPushButton(KIcon("list-add"), i18n("Add"), p);
        layout->addWidget(buttonAdd, 0, 1, 1, 1);
        connect(buttonAdd, SIGNAL(clicked()), p, SLOT(addKeyword()));

        buttonRemove = new KPushButton(KIcon("list-remove"), i18n("Remove"), p);
        layout->addWidget(buttonRemove, 1, 1, 1, 1);
        buttonRemove->setEnabled(false);
        connect(buttonRemove, SIGNAL(clicked()), p, SLOT(removeKeyword()));
    }
};

int SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidgetPrivate::keywordCounter = 0;


SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsGlobalKeywordsWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsGlobalKeywordsWidget::~SettingsGlobalKeywordsWidget()
{
    delete d;
}

QString SettingsGlobalKeywordsWidget::label() const
{
    return i18n("Keywords");
}

KIcon SettingsGlobalKeywordsWidget::icon() const
{
    return KIcon("checkbox"); // TODO find better icon
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

void SettingsGlobalKeywordsWidget::addKeyword()
{
    if (d->stringListModel.insertRow(d->stringListModel.rowCount(), QModelIndex())) {
        QModelIndex index = d->stringListModel.index(d->stringListModel.rowCount() - 1, 0);
        d->stringListModel.setData(index, i18n("NewKeyword%1", d->keywordCounter++));
        d->listViewKeywords->setCurrentIndex(index);
        d->listViewKeywords->edit(index);
        d->buttonRemove->setEnabled(true);
    }
}

void SettingsGlobalKeywordsWidget::removeKeyword()
{
    QModelIndex currIndex = d->listViewKeywords->currentIndex();
    if (currIndex == QModelIndex())
        currIndex = d->listViewKeywords->selectionModel()->selectedIndexes().first();
    d->stringListModel.removeRow(currIndex.row());
    d->buttonRemove->setEnabled(d->listViewKeywords->currentIndex() != QModelIndex());
}

void SettingsGlobalKeywordsWidget::enableRemoveButton()
{
    d->buttonRemove->setEnabled(d->listViewKeywords->currentIndex() != QModelIndex());
}
