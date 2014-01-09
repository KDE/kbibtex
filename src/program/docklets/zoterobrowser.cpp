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
#include <QListView>
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
#include "zotero/tags.h"
#include "zotero/tagmodel.h"
#include "zotero/api.h"
#include "zotero/oauthwizard.h"

class ZoteroBrowser::Private
{
private:
    ZoteroBrowser *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString configEntryNumericUserIds, configEntryApiKeys;

public:
    Zotero::Items *items;
    Zotero::Tags *tags;
    Zotero::TagModel *tagModel;
    Zotero::Collection *collection;
    Zotero::CollectionModel *collectionModel;
    Zotero::API *api;

    SearchResults *searchResults;

    QTabWidget *tabWidget;
    QTreeView *collectionBrowser;
    QListView *tagBrowser;
    KComboBox *comboBoxNumericUserId;
    KComboBox *comboBoxApiKey;
    KPushButton *buttonLoadBibliography;

    QCursor nonBusyCursor;

    Private(SearchResults *sr, ZoteroBrowser *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), items(NULL), tags(NULL), tagModel(NULL), collection(NULL), collectionModel(NULL), api(NULL), searchResults(sr), nonBusyCursor(p->cursor()) {
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

        p->updateButtons();
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
    d->tabWidget->addTab(container, KIcon("preferences-web-browser-identification"), i18n("Credentials"));
    QBoxLayout *containerLayout = new QVBoxLayout(container);
    QFormLayout *containerForm = new QFormLayout();
    containerLayout->addLayout(containerForm, 1);

    d->comboBoxNumericUserId = new KComboBox(container);
    d->comboBoxNumericUserId->setEditable(true);
    QSizePolicy sizePolicy = d->comboBoxNumericUserId->sizePolicy();
    sizePolicy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    d->comboBoxNumericUserId->setSizePolicy(sizePolicy);
    dynamic_cast<KLineEdit *>(d->comboBoxNumericUserId->lineEdit())->setClearButtonShown(true);
    containerForm->addRow(i18n("Numeric user id:"), d->comboBoxNumericUserId);

    d->comboBoxApiKey = new KComboBox(container);
    d->comboBoxApiKey->setEditable(true);
    d->comboBoxApiKey->setSizePolicy(sizePolicy);
    dynamic_cast<KLineEdit *>(d->comboBoxApiKey->lineEdit())->setClearButtonShown(true);
    containerForm->addRow(i18n("API key:"), d->comboBoxApiKey);

    QBoxLayout *containerButtonLayout = new QHBoxLayout();
    containerLayout->addLayout(containerButtonLayout, 0);
    containerButtonLayout->addStretch(1);
    d->buttonLoadBibliography = new KPushButton(KIcon("download"), i18n("Load bibliography"), container);
    containerButtonLayout->addWidget(d->buttonLoadBibliography, 0);
    connect(d->buttonLoadBibliography, SIGNAL(clicked()), this, SLOT(applyCredentials()));
    connect(d->comboBoxNumericUserId->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));
    connect(d->comboBoxApiKey->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));

    containerLayout->addStretch(10);

    containerButtonLayout = new QHBoxLayout();
    containerLayout->addLayout(containerButtonLayout, 0);
    KPushButton *buttonGetOAuthCredentials = new KPushButton(KIcon("preferences-web-browser-identification"), i18n("Get Credentials"), container);
    containerButtonLayout->addWidget(buttonGetOAuthCredentials, 0);
    connect(buttonGetOAuthCredentials, SIGNAL(clicked()), this, SLOT(getOAuthCredentials()));
    containerButtonLayout->addStretch(1);

    /// Collection browser
    d->collectionBrowser = new QTreeView(d->tabWidget);
    d->tabWidget->addTab(d->collectionBrowser, KIcon("folder-yellow"), i18n("Collections"));
    d->collectionBrowser->setHeaderHidden(true);
    d->collectionBrowser->setExpandsOnDoubleClick(false);
    connect(d->collectionBrowser, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(collectionDoubleClicked(QModelIndex)));

    /// Tag browser
    d->tagBrowser = new QListView(d->tabWidget);
    d->tabWidget->addTab(d->tagBrowser, KIcon("mail-tagged"), i18n("Tags"));
    connect(d->tagBrowser, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(tagDoubleClicked(QModelIndex)));
}

void ZoteroBrowser::modelReset()
{
    if (!d->collection->busy() && !d->tags->busy()) {
        setCursor(d->nonBusyCursor);
        setEnabled(true);
    } else {
        setCursor(Qt::WaitCursor);
        setEnabled(false);
    }

    if (!d->collection->busy() && !d->collection->initialized())
        KMessageBox::sorry(this, i18n("KBibTeX failed to retrieve the specified collection from Zotero. Please check that the provided user id and API key are valid."), i18n("Failed to retrieve collection"));
    if (!d->tags->busy() && !d->tags->initialized())
        KMessageBox::sorry(this, i18n("KBibTeX failed to retrieve the specified tags from Zotero. Please check that the provided user id and API key are valid."), i18n("Failed to retrieve tags"));
}

void ZoteroBrowser::collectionDoubleClicked(const QModelIndex &index)
{
    setCursor(Qt::WaitCursor);
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (slot reenableList)

    const QString collectionId = index.data(Zotero::CollectionModel::CollectionIdRole).toString();
    d->searchResults->clear();
    d->items->retrieveItemsByCollection(collectionId);
}

void ZoteroBrowser::tagDoubleClicked(const QModelIndex &index)
{
    setCursor(Qt::WaitCursor);
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (slot reenableList)

    const QString tag = index.data(Zotero::TagModel::TagRole).toString();
    d->searchResults->clear();
    d->items->retrieveItemsByTag(tag);
}

void ZoteroBrowser::showItem(QSharedPointer<Element> e)
{
    d->searchResults->insertElement(e);
}

void ZoteroBrowser::reenableList()
{
    setCursor(d->nonBusyCursor);
    setEnabled(true);
}

void ZoteroBrowser::updateButtons()
{
    d->buttonLoadBibliography->setEnabled(!d->comboBoxNumericUserId->currentText().isEmpty() && !d->comboBoxApiKey->currentText().isEmpty());
}

void ZoteroBrowser::applyCredentials()
{
    bool ok = false;
    const int id = d->comboBoxNumericUserId->currentText().toInt(&ok);
    if (ok) {
        setCursor(Qt::WaitCursor);
        setEnabled(false);

        d->api->deleteLater();
        d->collection->deleteLater();
        d->items->deleteLater();
        d->tags->deleteLater();
        d->collectionModel->deleteLater();
        d->tagModel->deleteLater();

        d->addTextToLists();
        d->saveState();

        d->api = new Zotero::API(Zotero::API::UserRequest, id, d->comboBoxApiKey->currentText(), this);
        d->items = new Zotero::Items(d->api, this);
        d->tags = new Zotero::Tags(d->api, this);
        d->tagModel = new Zotero::TagModel(d->tags, this);
        d->tagBrowser->setModel(d->tagModel);
        d->collection = new Zotero::Collection(d->api, this);
        d->collectionModel = new Zotero::CollectionModel(d->collection, this);
        d->collectionBrowser->setModel(d->collectionModel);

        connect(d->collectionModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->tagModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->items, SIGNAL(foundElement(QSharedPointer<Element>)), this, SLOT(showItem(QSharedPointer<Element>)));
        connect(d->items, SIGNAL(stoppedSearch(int)), this, SLOT(reenableList()));
        connect(d->tags, SIGNAL(finishedLoading()), this, SLOT(reenableList()));

        d->tabWidget->setCurrentIndex(1);
    } else
        KMessageBox::information(this, i18n("Value '%1' is not a valid numeric identifier of a user or a group.", d->comboBoxNumericUserId->currentText()), i18n("Invalid numeric identifier"));
}

void ZoteroBrowser::getOAuthCredentials()
{
    Zotero::OAuthWizard wizard(this);
    if (wizard.exec() && !wizard.apiKey().isEmpty() && wizard.userId() >= 0) {
        d->comboBoxApiKey->setEditText(wizard.apiKey());
        d->comboBoxNumericUserId->setEditText(QString::number(wizard.userId()));
    }
}
