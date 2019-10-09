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

#include "settingsglobalkeywordswidget.h"

#include <QLayout>
#include <QStringListModel>
#include <QListView>
#include <QPushButton>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "field/fieldlistedit.h"

class DisallowEmptyStringListModel : public QStringListModel
{
    Q_OBJECT

public:
    explicit DisallowEmptyStringListModel(QObject *parent)
            : QStringListModel(parent)
    {
        /// nothing
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override {
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
    QPushButton *buttonRemove;
    static int keywordCounter;

    SettingsGlobalKeywordsWidgetPrivate(SettingsGlobalKeywordsWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("Global Keywords")), stringListModel(parent) {
        setupGUI();
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QStringList keywordList = configGroup.readEntry(KeywordListEdit::keyGlobalKeywordList, QStringList());
        stringListModel.setStringList(keywordList);
    }

    bool saveState() {
        KConfigGroup configGroup(config, configGroupName);
        const QStringList oldGlobalKeywordList = configGroup.readEntry(KeywordListEdit::keyGlobalKeywordList, QStringList());
        configGroup.writeEntry(KeywordListEdit::keyGlobalKeywordList, stringListModel.stringList());
        config->sync();
        return oldGlobalKeywordList != stringListModel.stringList();
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
        connect(listViewKeywords, &QListView::pressed, p, &SettingsGlobalKeywordsWidget::enableRemoveButton);

        QPushButton *buttonAdd = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add"), p);
        layout->addWidget(buttonAdd, 0, 1, 1, 1);
        connect(buttonAdd, &QPushButton::clicked, p, &SettingsGlobalKeywordsWidget::addKeyword);

        buttonRemove = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove"), p);
        layout->addWidget(buttonRemove, 1, 1, 1, 1);
        buttonRemove->setEnabled(false);
        connect(buttonRemove, &QPushButton::clicked, p, &SettingsGlobalKeywordsWidget::removeKeyword);
    }
};

int SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidgetPrivate::keywordCounter = 0;


SettingsGlobalKeywordsWidget::SettingsGlobalKeywordsWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsGlobalKeywordsWidgetPrivate(this))
{
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

QIcon SettingsGlobalKeywordsWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("checkbox")); // TODO find better icon
}

void SettingsGlobalKeywordsWidget::loadState()
{
    d->loadState();
}

bool SettingsGlobalKeywordsWidget::saveState()
{
    return d->saveState();
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

#include "settingsglobalkeywordswidget.moc"
