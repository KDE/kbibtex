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

#include "zoterobrowser.h"

#include <QTreeView>
#include <QTabWidget>
#include <QListView>
#include <QLayout>
#include <QFormLayout>
#include <QAbstractItemModel>
#include <QRadioButton>
#include <QPushButton>
#include <QPointer>
#include <QLineEdit>
#include <QComboBox>

#include <KLocalizedString>
#include <KWallet/KWallet>
#include <KMessageBox>

#include <Element>
#include "searchresults.h"
#include "zotero/collectionmodel.h"
#include "zotero/collection.h"
#include "zotero/items.h"
#include "zotero/groups.h"
#include "zotero/tags.h"
#include "zotero/tagmodel.h"
#include "zotero/api.h"
#include "zotero/oauthwizard.h"
#include "logging_program.h"

using KWallet::Wallet;

class ZoteroBrowser::Private
{
private:
    ZoteroBrowser *p;

public:
    Zotero::Items *items;
    Zotero::Groups *groups;
    Zotero::Tags *tags;
    Zotero::TagModel *tagModel;
    Zotero::Collection *collection;
    Zotero::CollectionModel *collectionModel;
    QSharedPointer<Zotero::API> api;
    bool needToApplyCredentials;

    SearchResults *searchResults;

    QTabWidget *tabWidget;
    QTreeView *collectionBrowser;
    QListView *tagBrowser;
    QLineEdit *lineEditNumericUserId;
    QLineEdit *lineEditApiKey;
    QRadioButton *radioPersonalLibrary;
    QRadioButton *radioGroupLibrary;
    bool comboBoxGroupListInitialized;
    QComboBox *comboBoxGroupList;

    QCursor nonBusyCursor;

    Wallet *wallet;
    static const QString walletFolderOAuth, walletEntryKBibTeXZotero, walletKeyZoteroId, walletKeyZoteroApiKey;

    Private(SearchResults *sr, ZoteroBrowser *parent)
            : p(parent), items(nullptr), groups(nullptr), tags(nullptr), tagModel(nullptr), collection(nullptr), collectionModel(nullptr), needToApplyCredentials(true), searchResults(sr), comboBoxGroupListInitialized(false), nonBusyCursor(p->cursor()), wallet(nullptr) {
        setupGUI();
    }

    ~Private() {
        if (wallet != nullptr)
            delete wallet;
        if (items != nullptr) delete items;
        if (groups != nullptr) delete groups;
        if (tags != nullptr) delete tags;
        if (tagModel != nullptr) delete tagModel;
        if (collection != nullptr) delete collection;
        if (collectionModel != nullptr) delete collectionModel;
        api.clear();
    }

    void setupGUI()
    {
        QBoxLayout *layout = new QVBoxLayout(p);
        tabWidget = new QTabWidget(p);
        layout->addWidget(tabWidget);

        QWidget *container = new QWidget(tabWidget);
        tabWidget->addTab(container, QIcon::fromTheme(QStringLiteral("preferences-web-browser-identification")), i18n("Library"));
        connect(tabWidget, &QTabWidget::currentChanged, p, &ZoteroBrowser::tabChanged);
        QBoxLayout *containerLayout = new QVBoxLayout(container);

        /// Personal or Group Library
        QGridLayout *gridLayout = new QGridLayout();
        containerLayout->addLayout(gridLayout);
        gridLayout->setMargin(0);
        gridLayout->setColumnMinimumWidth(0, 16); // TODO determine size of a radio button
        radioPersonalLibrary = new QRadioButton(i18n("Personal library"), container);
        gridLayout->addWidget(radioPersonalLibrary, 0, 0, 1, 2);
        radioGroupLibrary = new QRadioButton(i18n("Group library"), container);
        gridLayout->addWidget(radioGroupLibrary, 1, 0, 1, 2);
        comboBoxGroupList = new QComboBox(container);
        gridLayout->addWidget(comboBoxGroupList, 2, 1, 1, 1);
        QSizePolicy sizePolicy = comboBoxGroupList->sizePolicy();
        sizePolicy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
        comboBoxGroupList->setSizePolicy(sizePolicy);
        radioPersonalLibrary->setChecked(true);
        comboBoxGroupList->setEnabled(false);
        comboBoxGroupList->addItem(i18n("No groups available"));
        connect(radioGroupLibrary, &QRadioButton::toggled, p, &ZoteroBrowser::radioButtonsToggled);
        connect(radioPersonalLibrary, &QRadioButton::toggled, p, &ZoteroBrowser::radioButtonsToggled);
        connect(comboBoxGroupList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, [this]() {
            needToApplyCredentials = true;
        });

        containerLayout->addStretch(10);

        /// Credentials
        QFormLayout *containerForm = new QFormLayout();
        containerLayout->addLayout(containerForm, 1);
        containerForm->setMargin(0);
        lineEditNumericUserId = new QLineEdit(container);
        lineEditNumericUserId->setSizePolicy(sizePolicy);
        lineEditNumericUserId->setReadOnly(true);
        containerForm->addRow(i18n("Numeric user id:"), lineEditNumericUserId);
        connect(lineEditNumericUserId, &QLineEdit::textChanged, p, &ZoteroBrowser::invalidateGroupList);

        lineEditApiKey = new QLineEdit(container);
        lineEditApiKey->setSizePolicy(sizePolicy);
        lineEditApiKey->setReadOnly(true);
        containerForm->addRow(i18n("API key:"), lineEditApiKey);
        connect(lineEditApiKey, &QLineEdit::textChanged, p, &ZoteroBrowser::invalidateGroupList);

        QBoxLayout *containerButtonLayout = new QHBoxLayout();
        containerLayout->addLayout(containerButtonLayout, 0);
        containerButtonLayout->setMargin(0);
        QPushButton *buttonGetOAuthCredentials = new QPushButton(QIcon::fromTheme(QStringLiteral("preferences-web-browser-identification")), i18n("Get New Credentials"), container);
        containerButtonLayout->addWidget(buttonGetOAuthCredentials, 0);
        connect(buttonGetOAuthCredentials, &QPushButton::clicked, p, &ZoteroBrowser::getOAuthCredentials);
        containerButtonLayout->addStretch(1);

        /// Collection browser
        collectionBrowser = new QTreeView(tabWidget);
        tabWidget->addTab(collectionBrowser, QIcon::fromTheme(QStringLiteral("folder-yellow")), i18n("Collections"));
        collectionBrowser->setHeaderHidden(true);
        collectionBrowser->setExpandsOnDoubleClick(false);
        connect(collectionBrowser, &QTreeView::doubleClicked, p, &ZoteroBrowser::collectionDoubleClicked);

        /// Tag browser
        tagBrowser = new QListView(tabWidget);
        tabWidget->addTab(tagBrowser, QIcon::fromTheme(QStringLiteral("mail-tagged")), i18n("Tags"));
        connect(tagBrowser, &QListView::doubleClicked, p, &ZoteroBrowser::tagDoubleClicked);
    }

    void queueReadOAuthCredentials() {
        if (wallet != nullptr && wallet->isOpen())
            p->readOAuthCredentials(true);
        else {
            /// Wallet is closed or not initialized
            if (wallet != nullptr)
                /// Delete existing but closed wallet, will be replaced by new, open wallet soon
                delete wallet;
            p->setEnabled(false);
            p->setCursor(Qt::WaitCursor);
            wallet = Wallet::openWallet(Wallet::NetworkWallet(), p->winId(), Wallet::Asynchronous);
            connect(wallet, &Wallet::walletOpened, p, &ZoteroBrowser::readOAuthCredentials);
        }
    }

    void queueWriteOAuthCredentials() {
        if (wallet != nullptr && wallet->isOpen())
            p->writeOAuthCredentials(true);
        else {
            /// Wallet is closed or not initialized
            if (wallet != nullptr)
                /// Delete existing but closed wallet, will be replaced by new, open wallet soon
                delete wallet;
            p->setEnabled(false);
            p->setCursor(Qt::WaitCursor);
            wallet = Wallet::openWallet(Wallet::NetworkWallet(), p->winId(), Wallet::Asynchronous);
            connect(wallet, &Wallet::walletOpened, p, &ZoteroBrowser::writeOAuthCredentials);
        }
    }
};

const QString ZoteroBrowser::Private::walletFolderOAuth = QStringLiteral("OAuth");
const QString ZoteroBrowser::Private::walletEntryKBibTeXZotero = QStringLiteral("KBibTeX/Zotero");
const QString ZoteroBrowser::Private::walletKeyZoteroId = QStringLiteral("UserId");
const QString ZoteroBrowser::Private::walletKeyZoteroApiKey = QStringLiteral("ApiKey");

ZoteroBrowser::ZoteroBrowser(SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), d(new ZoteroBrowser::Private(searchResults, this))
{
    /// Forece GUI update
    updateButtons();
    radioButtonsToggled();
}

ZoteroBrowser::~ZoteroBrowser()
{
    delete d;
}

void ZoteroBrowser::visibiltyChanged(bool v) {
    if (v && d->lineEditApiKey->text().isEmpty())
        /// If Zotero dock became visible and no API key is set, check KWallet for credentials
        d->queueReadOAuthCredentials();
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

    if (!d->tags->busy() && !d->collection->busy() && !(d->collection->initialized() && d->tags->initialized()))
        KMessageBox::information(this, i18n("KBibTeX failed to retrieve the bibliography from Zotero. Please check that the provided user id and API key are valid."), i18n("Failed to retrieve data from Zotero"));
}

void ZoteroBrowser::collectionDoubleClicked(const QModelIndex &index)
{
    setCursor(Qt::WaitCursor);
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (slot reenableWidget)

    const QString collectionId = index.data(Zotero::CollectionModel::CollectionIdRole).toString();
    d->searchResults->clear();
    d->items->retrieveItemsByCollection(collectionId);
}

void ZoteroBrowser::tagDoubleClicked(const QModelIndex &index)
{
    setCursor(Qt::WaitCursor);
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (slot reenableWidget)

    const QString tag = index.data(Zotero::TagModel::TagRole).toString();
    d->searchResults->clear();
    d->items->retrieveItemsByTag(tag);
}

void ZoteroBrowser::showItem(QSharedPointer<Element> e)
{
    d->searchResults->insertElement(e);
    emit itemToShow();
}

void ZoteroBrowser::reenableWidget()
{
    setCursor(d->nonBusyCursor);
    setEnabled(true);
}

void ZoteroBrowser::updateButtons()
{
    const bool validNumericIdAndApiKey = !d->lineEditNumericUserId->text().isEmpty() && !d->lineEditApiKey->text().isEmpty();
    d->radioGroupLibrary->setEnabled(validNumericIdAndApiKey);
    d->radioPersonalLibrary->setEnabled(validNumericIdAndApiKey);
    d->needToApplyCredentials = true;
}

bool ZoteroBrowser::applyCredentials()
{
    bool ok = false;
    const int userId = d->lineEditNumericUserId->text().toInt(&ok);
    const QString apiKey = d->lineEditApiKey->text();
    if (ok && !apiKey.isEmpty()) {
        setCursor(Qt::WaitCursor);
        setEnabled(false);

        ok = false;
        int groupId = d->comboBoxGroupList->itemData(d->comboBoxGroupList->currentIndex()).toInt(&ok);
        if (!ok) groupId = -1;

        disconnect(d->tags, &Zotero::Tags::finishedLoading, this, &ZoteroBrowser::reenableWidget);
        disconnect(d->items, &Zotero::Items::stoppedSearch, this, &ZoteroBrowser::reenableWidget);
        disconnect(d->items, &Zotero::Items::foundElement, this, &ZoteroBrowser::showItem);
        disconnect(d->tagModel, &Zotero::TagModel::modelReset, this, &ZoteroBrowser::modelReset);
        disconnect(d->collectionModel, &Zotero::CollectionModel::modelReset, this, &ZoteroBrowser::modelReset);

        d->collection->deleteLater();
        d->items->deleteLater();
        d->tags->deleteLater();
        d->collectionModel->deleteLater();
        d->tagModel->deleteLater();
        d->api.clear();

        const bool makeGroupRequest = d->radioGroupLibrary->isChecked() && groupId > 0;
        d->api = QSharedPointer<Zotero::API>(new Zotero::API(makeGroupRequest ? Zotero::API::RequestScope::Group : Zotero::API::RequestScope::User, makeGroupRequest ? groupId : userId, d->lineEditApiKey->text(), this));
        d->items = new Zotero::Items(d->api, this);
        d->tags = new Zotero::Tags(d->api, this);
        d->tagModel = new Zotero::TagModel(d->tags, this);
        d->tagBrowser->setModel(d->tagModel);
        d->collection = new Zotero::Collection(d->api, this);
        d->collectionModel = new Zotero::CollectionModel(d->collection, this);
        d->collectionBrowser->setModel(d->collectionModel);

        connect(d->collectionModel, &Zotero::CollectionModel::modelReset, this, &ZoteroBrowser::modelReset);
        connect(d->tagModel, &Zotero::TagModel::modelReset, this, &ZoteroBrowser::modelReset);
        connect(d->items, &Zotero::Items::foundElement, this, &ZoteroBrowser::showItem);
        connect(d->items, &Zotero::Items::stoppedSearch, this, &ZoteroBrowser::reenableWidget);
        connect(d->tags, &Zotero::Tags::finishedLoading, this, &ZoteroBrowser::reenableWidget);

        d->needToApplyCredentials = false;

        return true;
    } else
        return false;
}

void ZoteroBrowser::radioButtonsToggled() {
    d->comboBoxGroupList->setEnabled(d->comboBoxGroupListInitialized && d->comboBoxGroupList->count() > 0 && d->radioGroupLibrary->isChecked());
    if (!d->comboBoxGroupListInitialized && d->radioGroupLibrary->isChecked())
        retrieveGroupList();
    d->needToApplyCredentials = true;
}

void ZoteroBrowser::groupListChanged() {
    d->needToApplyCredentials = true;
}

void ZoteroBrowser::retrieveGroupList() {
    bool ok = false;
    const int userId = d->lineEditNumericUserId->text().toInt(&ok);
    if (ok) {
        setCursor(Qt::WaitCursor);
        setEnabled(false);
        d->comboBoxGroupList->clear();
        d->comboBoxGroupListInitialized = false;

        disconnect(d->groups, &Zotero::Groups::finishedLoading, this, &ZoteroBrowser::gotGroupList);
        d->groups->deleteLater();
        d->api.clear();

        d->api = QSharedPointer<Zotero::API>(new Zotero::API(Zotero::API::RequestScope::User, userId, d->lineEditApiKey->text(), this));
        d->groups = new Zotero::Groups(d->api, this);

        connect(d->groups, &Zotero::Groups::finishedLoading, this, &ZoteroBrowser::gotGroupList);
    }
}

void ZoteroBrowser::invalidateGroupList() {
    d->comboBoxGroupList->clear();
    d->comboBoxGroupListInitialized = false;
    d->comboBoxGroupList->addItem(i18n("No groups available or no permissions"));
    d->comboBoxGroupList->setEnabled(false);
    d->radioPersonalLibrary->setChecked(true);
}

void ZoteroBrowser::gotGroupList() {
    const QMap<int, QString> groupMap = d->groups->groups();
    for (QMap<int, QString>::ConstIterator it = groupMap.constBegin(); it != groupMap.constEnd(); ++it) {
        d->comboBoxGroupList->addItem(it.value(), QVariant::fromValue<int>(it.key()));
    }
    if (groupMap.isEmpty()) {
        invalidateGroupList();
    } else {
        d->comboBoxGroupListInitialized = true;
        d->comboBoxGroupList->setEnabled(true);
        d->needToApplyCredentials = true;
    }

    reenableWidget();
}

void ZoteroBrowser::getOAuthCredentials()
{
    QPointer<Zotero::OAuthWizard> wizard = new Zotero::OAuthWizard(this);
    if (wizard->exec() && !wizard->apiKey().isEmpty() && wizard->userId() >= 0) {
        d->lineEditApiKey->setText(wizard->apiKey());
        d->lineEditNumericUserId->setText(QString::number(wizard->userId()));
        d->queueWriteOAuthCredentials();
        updateButtons();
        retrieveGroupList();
    }
    delete wizard;
}

void ZoteroBrowser::readOAuthCredentials(bool ok) {
    /// Do not call this slot a second time
    disconnect(d->wallet, &Wallet::walletOpened, this, &ZoteroBrowser::readOAuthCredentials);

    if (ok && (d->wallet->hasFolder(ZoteroBrowser::Private::walletFolderOAuth) || d->wallet->createFolder(ZoteroBrowser::Private::walletFolderOAuth)) && d->wallet->setFolder(ZoteroBrowser::Private::walletFolderOAuth)) {
        if (d->wallet->hasEntry(ZoteroBrowser::Private::walletEntryKBibTeXZotero)) {
            QMap<QString, QString> map;
            if (d->wallet->readMap(ZoteroBrowser::Private::walletEntryKBibTeXZotero, map) == 0) {
                if (map.contains(ZoteroBrowser::Private::walletKeyZoteroId) && map.contains(ZoteroBrowser::Private::walletKeyZoteroApiKey)) {
                    d->lineEditNumericUserId->setText(map.value(ZoteroBrowser::Private::walletKeyZoteroId, QString()));
                    d->lineEditApiKey->setText(map.value(ZoteroBrowser::Private::walletKeyZoteroApiKey, QString()));
                    updateButtons();
                    retrieveGroupList();
                } else
                    qCWarning(LOG_KBIBTEX_PROGRAM) << "Failed to locate Zotero Id and/or API key in KWallet";
            } else
                qCWarning(LOG_KBIBTEX_PROGRAM) << "Failed to access Zotero data in KWallet";
        } else
            qCDebug(LOG_KBIBTEX_PROGRAM) << "No Zotero credentials stored in KWallet";
    } else
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Accessing KWallet to sync API key did not succeed";
    reenableWidget();
}

void ZoteroBrowser::writeOAuthCredentials(bool ok) {
    disconnect(d->wallet, &Wallet::walletOpened, this, &ZoteroBrowser::writeOAuthCredentials);

    if (ok && (d->wallet->hasFolder(ZoteroBrowser::Private::walletFolderOAuth) || d->wallet->createFolder(ZoteroBrowser::Private::walletFolderOAuth)) && d->wallet->setFolder(ZoteroBrowser::Private::walletFolderOAuth)) {
        QMap<QString, QString> map;
        map.insert(ZoteroBrowser::Private::walletKeyZoteroId, d->lineEditNumericUserId->text());
        map.insert(ZoteroBrowser::Private::walletKeyZoteroApiKey, d->lineEditApiKey->text());
        if (d->wallet->writeMap(ZoteroBrowser::Private::walletEntryKBibTeXZotero, map) != 0)
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Writing API key to KWallet failed";
    } else
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Accessing KWallet to sync API key did not succeed";
    reenableWidget();
}

void ZoteroBrowser::tabChanged(int newTabIndex) {
    if (newTabIndex > 0 /** tabs after credential tab*/ && d->needToApplyCredentials) {
        const bool success = applyCredentials();
        for (int i = 1; i < d->tabWidget->count(); ++i)
            d->tabWidget->widget(i)->setEnabled(success);
    }
}
