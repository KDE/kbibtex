/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "zoterobrowser.h"

#include <QTreeView>
#include <QTabWidget>
#include <QLayout>
#include <QFormLayout>
#include <QAbstractItemModel>

#include <KLocale>
#include <KComboBox>
#include <KLineEdit>
#include <KPushButton>
#include <KConfigGroup>
#include <KMessageBox>

#include "element.h"
#include "searchresults.h"
#include "zotero/collectionmodel.h"
#include "zotero/collection.h"
#include "zotero/items.h"
#include "zotero/api.h"

class ZoteroBrowser::Private
{
private:
    ZoteroBrowser *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString configEntryNumericUserIds, configEntryApiKeys;

public:
    Zotero::Items *items;
    Zotero::Collection *collection;
    Zotero::CollectionModel *model;
    Zotero::API *api;

    SearchResults *searchResults;

    QTabWidget *tabWidget;
    QTreeView *collectionBrowser;
    KComboBox *comboBoxNumericUserId;
    KComboBox *comboBoxApiKey;

    Private(SearchResults *sr, ZoteroBrowser *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), items(NULL), collection(NULL), model(NULL), api(NULL), searchResults(sr) {
        /// nothing
    }


    void loadState() {
        KConfigGroup configGroup(config, configGroupName);

        const QStringList numericUserIds = configGroup.readEntry(configEntryNumericUserIds, QStringList());
        comboBoxNumericUserId->clear();
        comboBoxNumericUserId->addItems(numericUserIds);
        if (!numericUserIds.isEmpty()) comboBoxNumericUserId->setCurrentIndex(0);

        const QStringList apiKeys = configGroup.readEntry(configEntryApiKeys, QStringList());
        comboBoxApiKey->clear();
        comboBoxApiKey->addItems(apiKeys);
        if (!apiKeys.isEmpty()) comboBoxApiKey->setCurrentIndex(0);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);

        QStringList buffer;
        for (int i = 0; i < comboBoxNumericUserId->count(); ++i)
            buffer.append(comboBoxNumericUserId->itemText(i));
        configGroup.writeEntry(configEntryNumericUserIds, buffer);

        buffer.clear();
        for (int i = 0; i < comboBoxApiKey->count(); ++i)
            buffer.append(comboBoxApiKey->itemText(i));
        configGroup.writeEntry(configEntryApiKeys, buffer);

        config->sync();
    }

    void addTextToLists() {
        QString buffer = comboBoxNumericUserId->currentText();
        for (int i = 0; i < comboBoxNumericUserId->count(); ++i)
            if (comboBoxNumericUserId->itemText(i) == buffer) {
                comboBoxNumericUserId->removeItem(i);
                break;
            }
        comboBoxNumericUserId->insertItem(0, buffer);

        buffer = comboBoxApiKey->currentText();
        for (int i = 0; i < comboBoxApiKey->count(); ++i)
            if (comboBoxApiKey->itemText(i) == buffer) {
                comboBoxApiKey->removeItem(i);
                break;
            }
        comboBoxApiKey->insertItem(0, buffer);
    }
};

const QString ZoteroBrowser::Private::configGroupName = QLatin1String("ZoteroBrowser");
const QString ZoteroBrowser::Private::configEntryNumericUserIds = QLatin1String("NumericUserIds");
const QString ZoteroBrowser::Private::configEntryApiKeys = QLatin1String("ApiKeys");

ZoteroBrowser::ZoteroBrowser(SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), d(new ZoteroBrowser::Private(searchResults, this))
{
    setupGUI();
    d->loadState();
    applyCredentials();
}

ZoteroBrowser::~ZoteroBrowser()
{
    // TODO
}

void ZoteroBrowser::setupGUI()
{
    QBoxLayout *layout = new QVBoxLayout(this);
    d->tabWidget = new QTabWidget(this);
    layout->addWidget(d->tabWidget);

    /// Credentials
    QWidget *container = new QWidget(d->tabWidget);
    d->tabWidget->addTab(container, i18n("Credentials"));
    QBoxLayout *containerLayout = new QVBoxLayout(container);
    QFormLayout *containerForm = new QFormLayout();
    containerLayout->addLayout(containerForm, 1);
    d->comboBoxNumericUserId = new KComboBox(container);
    d->comboBoxNumericUserId->setEditable(true);
    dynamic_cast<KLineEdit *>(d->comboBoxNumericUserId->lineEdit())->setClearButtonShown(true);
    containerForm->addRow(i18n("Numeric user id:"), d->comboBoxNumericUserId);
    d->comboBoxApiKey = new KComboBox(container);
    d->comboBoxApiKey->setEditable(true);
    dynamic_cast<KLineEdit *>(d->comboBoxApiKey->lineEdit())->setClearButtonShown(true);
    containerForm->addRow(i18n("API key:"), d->comboBoxApiKey);
    QBoxLayout *containerButtonLayout = new QHBoxLayout();
    containerLayout->addLayout(containerButtonLayout, 0);
    KPushButton *buttonApplyCredentials = new KPushButton(i18n("Apply"), container);
    containerButtonLayout->addStretch(1);
    containerButtonLayout->addWidget(buttonApplyCredentials, 0);
    connect(buttonApplyCredentials, SIGNAL(clicked()), this, SLOT(applyCredentials()));
    QSizePolicy sizePolicy =  d->comboBoxApiKey->sizePolicy();
    sizePolicy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    d->comboBoxApiKey->setSizePolicy(sizePolicy);
    d->comboBoxNumericUserId->setSizePolicy(sizePolicy);

    /// Collection browser
    d->collectionBrowser = new QTreeView(d->tabWidget);
    d->tabWidget->addTab(d->collectionBrowser, i18n("Collections"));
    d->tabWidget->setCurrentWidget(d->collectionBrowser);
    d->collectionBrowser->setModel(d->model);
    d->collectionBrowser->setHeaderHidden(true);
    d->collectionBrowser->setExpandsOnDoubleClick(false);
    connect(d->collectionBrowser, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(collectionDoubleClicked(QModelIndex)));
}

void ZoteroBrowser::modelReset()
{
    setEnabled(true);
}

void ZoteroBrowser::collectionDoubleClicked(const QModelIndex &index)
{
    const QString collectionId = index.data(Zotero::CollectionModel::CollectionIdRole).toString();
    d->searchResults->clear();
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (signal stoppedSearch)
    d->items->retrieveItems(collectionId);
}

void ZoteroBrowser::showItem(QSharedPointer<Element> e)
{
    d->searchResults->insertElement(e);
}

void ZoteroBrowser::reenableList()
{
    setEnabled(true);
}

void ZoteroBrowser::applyCredentials()
{
    bool ok = false;
    const int id = d->comboBoxNumericUserId->currentText().toInt(&ok);
    if (ok) {
        d->api->deleteLater();
        d->collection->deleteLater();
        d->items->deleteLater();
        d->model->deleteLater();

        d->addTextToLists();
        d->saveState();

        d->api = new Zotero::API(Zotero::API::UserRequest, id, d->comboBoxApiKey->currentText(), this);
        d->items = new Zotero::Items(d->api, this);
        d->collection = new Zotero::Collection(d->api, this);
        d->model = new Zotero::CollectionModel(d->collection, this);
        d->collectionBrowser->setModel(d->model);

        connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->items, SIGNAL(foundElement(QSharedPointer<Element>)), this, SLOT(showItem(QSharedPointer<Element>)));
        connect(d->items, SIGNAL(stoppedSearch(int)), this, SLOT(reenableList()));

        setEnabled(false);
        d->tabWidget->setCurrentIndex(1);
    } else
        KMessageBox::information(this, i18n("Value '%1' is not a valid numeric identifier of a user or a group.", d->comboBoxNumericUserId->currentText()), i18n("Invalid numeric identifier"));
}
