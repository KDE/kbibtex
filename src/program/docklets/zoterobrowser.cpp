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
#include <KLineEdit>
#include <KPushButton>

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

public:
    Zotero::Items *items;
    Zotero::Collection *collection;
    Zotero::CollectionModel *model;
    Zotero::API *api;

    SearchResults *searchResults;

    QTabWidget *tabWidget;
    QTreeView *collectionBrowser;
    KLineEdit *lineEditNumericUserId;
    KLineEdit *lineEditApiKey;

    Private(SearchResults *sr, ZoteroBrowser *parent)
            : p(parent), searchResults(sr) {
        /// nothing
    }
};

ZoteroBrowser::ZoteroBrowser(SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), d(new ZoteroBrowser::Private(searchResults, this))
{
    setupGUI();
    setEnabled(false);
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
    d->lineEditNumericUserId = new KLineEdit(container);
    d->lineEditNumericUserId->setClearButtonShown(true);
    d->lineEditNumericUserId->setText(QLatin1String("475425"));
    containerForm->addRow(i18n("Numeric user id:"), d->lineEditNumericUserId);
    d->lineEditApiKey = new KLineEdit(container);
    d->lineEditApiKey->setClearButtonShown(true);
    containerForm->addRow(i18n("API key:"), d->lineEditApiKey);
    QBoxLayout *containerButtonLayout = new QHBoxLayout();
    containerLayout->addLayout(containerButtonLayout, 0);
    KPushButton *buttonApplyCredentials = new KPushButton(i18n("Apply"), container);
    containerButtonLayout->addStretch(1);
    containerButtonLayout->addWidget(buttonApplyCredentials, 0);
    connect(buttonApplyCredentials, SIGNAL(clicked()), this, SLOT(applyCredentials()));

    /// Collection browser
    d->collectionBrowser = new QTreeView(d->tabWidget);
    d->tabWidget->addTab(d->collectionBrowser, i18n("Collections"));
    d->tabWidget->setCurrentWidget(d->collectionBrowser);
    d->api = new Zotero::API(Zotero::API::UserRequest, 475425, QString(), this);
    d->items = new Zotero::Items(d->api, this);
    d->collection = new Zotero::Collection(d->api, this);
    d->model = new Zotero::CollectionModel(d->collection, this);
    d->collectionBrowser->setModel(d->model);
    d->collectionBrowser->setHeaderHidden(true);
    d->collectionBrowser->setExpandsOnDoubleClick(false);

    connect(d->collectionBrowser, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(collectionDoubleClicked(QModelIndex)));
    connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
    connect(d->items, SIGNAL(foundElement(QSharedPointer<Element>)), this, SLOT(showItem(QSharedPointer<Element>)));
    connect(d->items, SIGNAL(stoppedSearch(int)), this, SLOT(reenableList()));
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
    d->api->deleteLater();
    d->collection->deleteLater();
    d->items->deleteLater();
    d->model->deleteLater();

    const int id = d->lineEditNumericUserId->text().toInt();
    d->api = new Zotero::API(Zotero::API::UserRequest, id, d->lineEditApiKey->text(), this);
    d->items = new Zotero::Items(d->api, this);
    d->collection = new Zotero::Collection(d->api, this);
    d->model = new Zotero::CollectionModel(d->collection, this);
    d->collectionBrowser->setModel(d->model);

    connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
    connect(d->items, SIGNAL(foundElement(QSharedPointer<Element>)), this, SLOT(showItem(QSharedPointer<Element>)));
    connect(d->items, SIGNAL(stoppedSearch(int)), this, SLOT(reenableList()));

    setEnabled(false);
    d->tabWidget->setCurrentIndex(1);
}
