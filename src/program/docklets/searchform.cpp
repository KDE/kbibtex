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

#include "searchform.h"

#include <QLayout>
#include <QMap>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QProgressBar>
#include <QMimeType>
#include <QTimer>
#include <QSet>
#include <QAction>
#include <QScrollArea>
#include <QIcon>
#include <QPushButton>

#include <kio_version.h>
#include <KLocalizedString>
#if KIO_VERSION >= 0x054700 // >= 5.71.0
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#else // < 5.71.0
#include <KRun>
#endif // KIO_VERSION >= 0x054700
#include <KMessageBox>
#include <KParts/Part>
#include <KParts/ReadOnlyPart>
#include <KConfigGroup>
#include <KSharedConfig>

#include <Element>
#include <File>
#include <Comment>
#include <FileExporterBibTeX>
#include <onlinesearch/OnlineSearchAbstract>
#include <onlinesearch/OnlineSearchGeneral>
#include <onlinesearch/OnlineSearchBibsonomy>
#include <onlinesearch/onlinesearchgooglescholar.h>
#include <onlinesearch/OnlineSearchPubMed>
#include <onlinesearch/OnlineSearchIEEEXplore>
#include <onlinesearch/OnlineSearchAcmPortal>
#include <onlinesearch/OnlineSearchScienceDirect>
#include <onlinesearch/OnlineSearchSpringerLink>
#include <onlinesearch/OnlineSearchArXiv>
#ifdef HAVE_WEBENGINEWIDGETS
#include <onlinesearch/OnlineSearchJStor>
#endif // HAVE_WEBENGINEWIDGETS
#include <onlinesearch/OnlineSearchMathSciNet>
#include <onlinesearch/OnlineSearchMRLookup>
#include <onlinesearch/OnlineSearchInspireHep>
#include <onlinesearch/OnlineSearchCERNDS>
#include <onlinesearch/OnlineSearchIngentaConnect>
#include <onlinesearch/OnlineSearchSOANASAADS>
#include <onlinesearch/OnlineSearchIDEASRePEc>
#include <onlinesearch/OnlineSearchDOI>
#include <onlinesearch/OnlineSearchBioRxiv>
#include <onlinesearch/OnlineSearchSemanticScholar>
#include <file/FileView>
#include <models/FileModel>
#include "openfileinfo.h"
#include "searchresults.h"
#include "logging_program.h"

class SearchForm::SearchFormPrivate
{
private:
    SearchForm *p;
    QStackedWidget *queryTermsStack;
    QWidget *listContainer;
    QListWidget *enginesList;
    QLabel *whichEnginesLabel;
    QAction *actionOpenHomepage;

public:
    KSharedConfigPtr config;
    const QString configGroupName;

    SearchResults *sr;
    QMap<QListWidgetItem *, OnlineSearchAbstract *> itemToOnlineSearch;
    QSet<OnlineSearchAbstract *> runningSearches;
    QPushButton *searchButton;
    QPushButton *useEntryButton;
    OnlineSearchGeneral::Form *generalQueryTermsForm;
    QTabWidget *tabWidget;
    QSharedPointer<const Entry> currentEntry;
    QProgressBar *progressBar;
    QMap<OnlineSearchAbstract *, int> progressMap;
    QMap<OnlineSearchAbstract::Form *, QScrollArea *> formToScrollArea;

    enum SearchFormPrivateRole {
        /// Homepage of a search engine
        HomepageRole = Qt::UserRole + 5,
        /// Special widget for a search engine
        WidgetRole = Qt::UserRole + 6,
        /// Name of a search engine
        NameRole = Qt::UserRole + 7
    };

    SearchFormPrivate(SearchResults *searchResults, SearchForm *parent)
            : p(parent), whichEnginesLabel(nullptr), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))),
          configGroupName(QStringLiteral("Search Engines Docklet")), sr(searchResults), searchButton(nullptr), useEntryButton(nullptr), currentEntry(nullptr) {
        createGUI();
    }

    OnlineSearchAbstract::Form *currentQueryForm() {
        QScrollArea *area = qobject_cast<QScrollArea *>(queryTermsStack->currentWidget());
        return formToScrollArea.key(area, nullptr);
    }

    QScrollArea *wrapInScrollArea(OnlineSearchAbstract::Form *form, QWidget *parent) {
        QScrollArea *scrollArea = new QScrollArea(parent);
        form->setParent(scrollArea);
        scrollArea->setWidget(form);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        formToScrollArea.insert(form, scrollArea);
        return scrollArea;
    }

    QWidget *createQueryTermsStack(QWidget *parent) {
        QWidget *container = new QWidget(parent);
        QVBoxLayout *vLayout = new QVBoxLayout(container);

        whichEnginesLabel = new QLabel(container);
        whichEnginesLabel->setWordWrap(true);
        vLayout->addWidget(whichEnginesLabel);
        vLayout->setStretchFactor(whichEnginesLabel, 0);
        connect(whichEnginesLabel, &QLabel::linkActivated, p, [this]() {
            switchToEngines();
        });

        vLayout->addSpacing(8);

        queryTermsStack = new QStackedWidget(container);
        vLayout->addWidget(queryTermsStack);
        vLayout->setStretchFactor(queryTermsStack, 5);

        QScrollArea *scrollArea = wrapInScrollArea(createGeneralQueryTermsForm(queryTermsStack), queryTermsStack);
        queryTermsStack->addWidget(scrollArea);

        return container;
    }

    OnlineSearchGeneral::Form *createGeneralQueryTermsForm(QWidget *parent = nullptr) {
        generalQueryTermsForm = new OnlineSearchGeneral::Form(parent);
        return generalQueryTermsForm;
    }

    QWidget *createEnginesGUI(QWidget *parent) {
        listContainer = new QWidget(parent);
        QGridLayout *layout = new QGridLayout(listContainer);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);

        enginesList = new QListWidget(listContainer);
        layout->addWidget(enginesList, 0, 0, 1, 1);
        connect(enginesList, &QListWidget::itemChanged, p, &SearchForm::itemCheckChanged);
        connect(enginesList, &QListWidget::currentItemChanged, p, [this](QListWidgetItem * current, QListWidgetItem *) {
            enginesListCurrentChanged(current);
        });
        enginesList->setSelectionMode(QAbstractItemView::NoSelection);

        actionOpenHomepage = new QAction(QIcon::fromTheme(QStringLiteral("internet-web-browser")), i18n("Go to Homepage"), p);
        connect(actionOpenHomepage, &QAction::triggered, p, [this]() {
            openHomepage();
        });
        enginesList->addAction(actionOpenHomepage);
        enginesList->setContextMenuPolicy(Qt::ActionsContextMenu);

        return listContainer;
    }

    void createGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setMargin(0);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);
        layout->setColumnStretch(0, 0);
        layout->setColumnStretch(1, 1);
        layout->setColumnStretch(2, 0);

        tabWidget = new QTabWidget(p);
        tabWidget->setDocumentMode(true);
        layout->addWidget(tabWidget, 0, 0, 1, 3);

        QWidget *widget = createQueryTermsStack(tabWidget);
        tabWidget->addTab(widget, QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("Query Terms"));

        QWidget *listContainer = createEnginesGUI(tabWidget);
        tabWidget->addTab(listContainer, QIcon::fromTheme(QStringLiteral("applications-engineering")), i18n("Engines"));

        connect(tabWidget, &QTabWidget::currentChanged, p, [this]() {
            updateGUI();
        });

        useEntryButton = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), i18n("Use Entry"), p);
        layout->addWidget(useEntryButton, 1, 0, 1, 1);
        useEntryButton->setEnabled(false);
        connect(useEntryButton, &QPushButton::clicked, p, &SearchForm::copyFromEntry);

        progressBar = new QProgressBar(p);
        layout->addWidget(progressBar, 1, 1, 1, 1);
        progressBar->setMaximum(1000);
        progressBar->hide();

        searchButton = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-find")), i18n("Search"), p);
        layout->addWidget(searchButton, 1, 2, 1, 1);
        connect(generalQueryTermsForm, &OnlineSearchAbstract::Form::returnPressed, searchButton, &QPushButton::click);

        updateGUI();
    }

    void loadEngines() {
        enginesList->clear();

        addEngine(new OnlineSearchAcmPortal(p));
        addEngine(new OnlineSearchArXiv(p));
        addEngine(new OnlineSearchBioRxiv(p));
        addEngine(new OnlineSearchBibsonomy(p));
        addEngine(new OnlineSearchGoogleScholar(p));
        addEngine(new OnlineSearchIEEEXplore(p));
        addEngine(new OnlineSearchIngentaConnect(p));
#ifdef HAVE_WEBENGINEWIDGETS
        addEngine(new OnlineSearchJStor(p));
#endif // HAVE_WEBENGINEWIDGETS
        addEngine(new OnlineSearchMathSciNet(p));
        addEngine(new OnlineSearchMRLookup(p));
        addEngine(new OnlineSearchInspireHep(p));
        addEngine(new OnlineSearchCERNDS(p));
        addEngine(new OnlineSearchPubMed(p));
        addEngine(new OnlineSearchScienceDirect(p));
        addEngine(new OnlineSearchSpringerLink(p));
        addEngine(new OnlineSearchSOANASAADS(p));
        /// addEngine(new OnlineSearchIsbnDB(p)); /// disabled as provider switched to a paid model on 2017-12-26
        addEngine(new OnlineSearchIDEASRePEc(p));
        addEngine(new OnlineSearchDOI(p));
        addEngine(new OnlineSearchSemanticScholar(p));

        p->itemCheckChanged(nullptr);
        updateGUI();
    }

    void addEngine(OnlineSearchAbstract *engine) {
        KConfigGroup configGroup(config, configGroupName);

        /// Disable signals while updating the widget and its items
        QSignalBlocker enginesListSignalBlocker(enginesList);

        QListWidgetItem *item = new QListWidgetItem(engine->label(), enginesList);
        static const QSet<QString> enginesEnabledByDefault {QStringLiteral("GoogleScholar"), QStringLiteral("Bibsonomy")};
        item->setCheckState(configGroup.readEntry(engine->name(), enginesEnabledByDefault.contains(engine->name())) ? Qt::Checked : Qt::Unchecked);
        item->setIcon(engine->icon(item));
        item->setToolTip(engine->label());
        item->setData(HomepageRole, engine->homepage());
        item->setData(NameRole, engine->name());

        OnlineSearchAbstract::Form *widget = engine->customWidget(queryTermsStack);
        item->setData(WidgetRole, QVariant::fromValue<OnlineSearchAbstract::Form *>(widget));
        if (widget != nullptr) {
            connect(widget, &OnlineSearchAbstract::Form::returnPressed, searchButton, &QPushButton::click);
            QScrollArea *scrollArea = wrapInScrollArea(widget, queryTermsStack);
            queryTermsStack->addWidget(scrollArea);
        }

        itemToOnlineSearch.insert(item, engine);
        connect(engine, &OnlineSearchAbstract::foundEntry, p, [this](QSharedPointer<Entry> entry) {
            sr->insertElement(entry);
        });
        connect(engine, &OnlineSearchAbstract::stoppedSearch, p, &SearchForm::stoppedSearch);
        connect(engine, &OnlineSearchAbstract::progress, p, &SearchForm::updateProgress);
    }

    void switchToSearch() {
        for (QMap<QListWidgetItem *, OnlineSearchAbstract *>::ConstIterator it = itemToOnlineSearch.constBegin(); it != itemToOnlineSearch.constEnd(); ++it)
            disconnect(searchButton, &QPushButton::clicked, it.value(), &OnlineSearchAbstract::cancel);

        connect(searchButton, &QPushButton::clicked, p, &SearchForm::startSearch);
        searchButton->setText(i18n("Search"));
        searchButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        for (int i = tabWidget->count() - 1; i >= 0; --i)
            tabWidget->widget(i)->setEnabled(true);
        tabWidget->unsetCursor();
    }

    void switchToCancel() {
        disconnect(searchButton, &QPushButton::clicked, p, &SearchForm::startSearch);

        for (QMap<QListWidgetItem *, OnlineSearchAbstract *>::ConstIterator it = itemToOnlineSearch.constBegin(); it != itemToOnlineSearch.constEnd(); ++it)
            connect(searchButton, &QPushButton::clicked, it.value(), &OnlineSearchAbstract::cancel);
        searchButton->setText(i18n("Stop"));
        searchButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
        for (int i = tabWidget->count() - 1; i >= 0; --i)
            tabWidget->widget(i)->setEnabled(false);
        tabWidget->setCursor(Qt::WaitCursor);
    }

    void switchToEngines() {
        tabWidget->setCurrentWidget(listContainer);
    }

    void updateGUI() {
        if (whichEnginesLabel == nullptr) return;

        QStringList checkedEngines;
        QListWidgetItem *cursor = nullptr;
        for (QMap<QListWidgetItem *, OnlineSearchAbstract *>::ConstIterator it = itemToOnlineSearch.constBegin(); it != itemToOnlineSearch.constEnd(); ++it)
            if (it.key()->checkState() == Qt::Checked) {
                checkedEngines << it.key()->text();
                cursor = it.key();
            }

        switch (checkedEngines.size()) {
        case 0: whichEnginesLabel->setText(i18n("No search engine selected (<a href=\"changeEngine\">change</a>).")); break;
        case 1: whichEnginesLabel->setText(i18n("Search engine <b>%1</b> is selected (<a href=\"changeEngine\">change</a>).", checkedEngines.first())); break;
        case 2: whichEnginesLabel->setText(i18n("Search engines <b>%1</b> and <b>%2</b> are selected (<a href=\"changeEngine\">change</a>).", checkedEngines.first(), checkedEngines.at(1))); break;
        case 3: whichEnginesLabel->setText(i18n("Search engines <b>%1</b>, <b>%2</b>, and <b>%3</b> are selected (<a href=\"changeEngine\">change</a>).", checkedEngines.first(), checkedEngines.at(1), checkedEngines.at(2))); break;
        default: whichEnginesLabel->setText(i18n("Search engines <b>%1</b>, <b>%2</b>, and more are selected (<a href=\"changeEngine\">change</a>).", checkedEngines.first(), checkedEngines.at(1))); break;
        }

        OnlineSearchAbstract::Form *currentQueryWidget = nullptr;
        if (cursor != nullptr && checkedEngines.size() == 1)
            currentQueryWidget = cursor->data(WidgetRole).value<OnlineSearchAbstract::Form *>();
        if (currentQueryWidget == nullptr)
            currentQueryWidget = generalQueryTermsForm;
        QScrollArea *area = formToScrollArea.value(currentQueryWidget, nullptr);
        if (area != nullptr)
            queryTermsStack->setCurrentWidget(area);

        if (useEntryButton != nullptr)
            useEntryButton->setEnabled(!currentEntry.isNull() && tabWidget->currentIndex() == 0);
    }

    void openHomepage() {
        QListWidgetItem *item = enginesList->currentItem();
        if (item != nullptr) {
            QUrl url = item->data(HomepageRole).toUrl();
            /// Guess mime type for url to open
            QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
            const QString mimeTypeName = mimeType.name();
            /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < 0x054700 // < 5.71.0
            KRun::runUrl(url, mimeTypeName, p, KRun::RunFlags());
#else // KIO_VERSION < 0x054700 // >= 5.71.0
            KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url, mimeTypeName);
            job->setUiDelegate(new KIO::JobUiDelegate());
            job->start();
#endif // KIO_VERSION < 0x054700
        }
    }

    void enginesListCurrentChanged(QListWidgetItem *current) {
        actionOpenHomepage->setEnabled(current != nullptr);
    }
};

SearchForm::SearchForm(SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), d(new SearchFormPrivate(searchResults, this))
{
    d->loadEngines();
    d->switchToSearch();
}

SearchForm::~SearchForm()
{
    delete d;
}

void SearchForm::setElement(QSharedPointer<Element> element, const File *)
{
    d->currentEntry = element.dynamicCast<const Entry>();
    d->useEntryButton->setEnabled(!d->currentEntry.isNull() && d->tabWidget->currentIndex() == 0);
}

void SearchForm::startSearch()
{
    OnlineSearchAbstract::Form *currentForm = d->currentQueryForm();
    if (!currentForm->readyToStart()) {
        KMessageBox::sorry(this, i18n("Could not start searching the Internet:\nThe search terms are not complete or invalid."), i18n("Searching the Internet"));
        return;
    }

    d->runningSearches.clear();
    d->sr->clear();
    d->progressBar->setValue(0);
    d->progressMap.clear();
    d->useEntryButton->hide();
    d->progressBar->show();

    if (currentForm == d->generalQueryTermsForm) {
        /// start search using the general-purpose form's values

        QMap<OnlineSearchAbstract::QueryKey, QString> queryTerms = d->generalQueryTermsForm->getQueryTerms();
        int numResults = d->generalQueryTermsForm->getNumResults();
        for (QMap<QListWidgetItem *, OnlineSearchAbstract *>::ConstIterator it = d->itemToOnlineSearch.constBegin(); it != d->itemToOnlineSearch.constEnd(); ++it)
            if (it.key()->checkState() == Qt::Checked) {
                it.value()->startSearch(queryTerms, numResults);
                d->runningSearches.insert(it.value());
            }
        if (d->runningSearches.isEmpty()) {
            /// if no search engine has been checked (selected), something went wrong
            return;
        }
    } else {
        /// use the single selected search engine's specific form

        for (QMap<QListWidgetItem *, OnlineSearchAbstract *>::ConstIterator it = d->itemToOnlineSearch.constBegin(); it != d->itemToOnlineSearch.constEnd(); ++it)
            if (it.key()->checkState() == Qt::Checked) {
                it.value()->startSearchFromForm();
                d->runningSearches.insert(it.value());
            }
        if (d->runningSearches.isEmpty()) {
            /// if no search engine has been checked (selected), something went wrong
            return;
        }
    }

    d->switchToCancel();
}

void SearchForm::stoppedSearch(int)
{
    OnlineSearchAbstract *engine = static_cast<OnlineSearchAbstract *>(sender());
    if (d->runningSearches.remove(engine)) {
        if (d->runningSearches.isEmpty()) {
            /// last search engine stopped
            d->switchToSearch();
            emit doneSearching();

            QTimer::singleShot(1000, d->progressBar, &QProgressBar::hide);
            QTimer::singleShot(1100, d->useEntryButton, &QPushButton::show);
        } else {
            QStringList remainingEngines;
            remainingEngines.reserve(d->runningSearches.size());
            for (OnlineSearchAbstract *running : const_cast<const QSet<OnlineSearchAbstract *> &>(d->runningSearches)) {
                remainingEngines.append(running->label());
            }
            if (!remainingEngines.isEmpty())
                qCDebug(LOG_KBIBTEX_PROGRAM) << "Remaining running engines:" << remainingEngines.join(QStringLiteral(", "));
        }
    }
}

void SearchForm::itemCheckChanged(QListWidgetItem *item)
{
    int numCheckedEngines = 0;
    for (QMap<QListWidgetItem *, OnlineSearchAbstract *>::ConstIterator it = d->itemToOnlineSearch.constBegin(); it != d->itemToOnlineSearch.constEnd(); ++it)
        if (it.key()->checkState() == Qt::Checked)
            ++numCheckedEngines;

    d->searchButton->setEnabled(numCheckedEngines > 0);

    if (item != nullptr) {
        KConfigGroup configGroup(d->config, d->configGroupName);
        QString name = item->data(SearchForm::SearchFormPrivate::NameRole).toString();
        configGroup.writeEntry(name, item->checkState() == Qt::Checked);
        d->config->sync();
    }
}

void SearchForm::copyFromEntry()
{
    Q_ASSERT_X(!d->currentEntry.isNull(), "SearchForm::copyFromEntry", "d->currentEntry is NULL");

    d->currentQueryForm()->copyFromEntry(*(d->currentEntry));
}

void SearchForm::updateProgress(int cur, int total)
{
    OnlineSearchAbstract *ws = static_cast<OnlineSearchAbstract *>(sender());
    d->progressMap[ws] = total > 0 ? cur * 1000 / total : 0;

    int progress = 0, count = 0;
    for (QMap<OnlineSearchAbstract *, int>::ConstIterator it = d->progressMap.constBegin(); it != d->progressMap.constEnd(); ++it, ++count)
        progress += it.value();

    d->progressBar->setValue(count >= 1 ? progress / count : 0);
}
