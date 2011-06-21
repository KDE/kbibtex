/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#include <QMap>
#include <QTabWidget>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QDesktopServices>
#include <QStackedWidget>
#include <QProgressBar>
#include <QTimer>
#include <QSet>

#include <KPushButton>
#include <KLineEdit>
#include <KLocale>
#include <KIcon>
#include <KDebug>
#include <KMessageBox>
#include <KTemporaryFile>
#include <KAction>
#include <kparts/part.h>
#include <KStandardDirs>
#include <KConfigGroup>

#include <websearchabstract.h>
#include <websearchgeneral.h>
#include <websearchbibsonomy.h>
#include <websearchgooglescholar.h>
#include <websearchpubmed.h>
#include <websearchieeexplore.h>
#include <websearchacmportal.h>
#include <websearchsciencedirect.h>
#include <websearchspringerlink.h>
#include <websearcharxiv.h>
#include <websearchjstor.h>
#include <fileexporterbibtex.h>
#include <element.h>
#include <file.h>
#include <comment.h>
#include <openfileinfo.h>
#include <bibtexeditor.h>
#include <bibtexfilemodel.h>
#include <mdiwidget.h>
#include <searchresults.h>
#include "searchform.h"

const int HomepageRole = Qt::UserRole + 5;
const int WidgetRole = Qt::UserRole + 6;
const int NameRole = Qt::UserRole + 7;

class SearchForm::SearchFormPrivate
{
private:
    SearchForm *p;
    QStackedWidget *queryTermsStack;
    QWidget *listContainer;
    QListWidget *enginesList;
    QLabel *whichEnginesLabel;
    KAction *actionOpenHomepage;

public:
    KSharedConfigPtr config;
    const QString configGroupName;

    MDIWidget *m;
    SearchResults *sr;
    QMap<QListWidgetItem*, WebSearchAbstract*> itemToWebSearch;
    QSet<WebSearchAbstract*> runningSearches;
    KPushButton *searchButton;
    KPushButton *useEntryButton;
    WebSearchQueryFormGeneral *generalQueryTermsForm;
    QTabWidget *tabWidget;
    Entry *currentEntry;
    QProgressBar *progressBar;
    QMap<WebSearchAbstract*, int> progressMap;

    SearchFormPrivate(MDIWidget *mdiWidget, SearchResults *searchResults, SearchForm *parent)
            : p(parent), whichEnginesLabel(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
            configGroupName(QLatin1String("Search Engines Docklet")), m(mdiWidget), sr(searchResults), currentEntry(NULL) {
        // nothing
    }

    WebSearchQueryFormAbstract *currentQueryForm() {
        return static_cast<WebSearchQueryFormAbstract*>(queryTermsStack->currentWidget());
    }

    QWidget* createQueryTermsStack(QWidget *parent) {
        QWidget *container = new QWidget(parent);
        QVBoxLayout *vLayout = new QVBoxLayout(container);

        whichEnginesLabel = new QLabel(container);
        whichEnginesLabel->setWordWrap(true);
        vLayout->addWidget(whichEnginesLabel);
        vLayout->setStretch(0, 10);
        connect(whichEnginesLabel, SIGNAL(linkActivated(QString)), p, SLOT(switchToEngines()));

        vLayout->addSpacing(8);

        queryTermsStack = new QStackedWidget(parent);
        vLayout->addWidget(queryTermsStack);
        queryTermsStack->addWidget(createGeneralQueryTermsForm(queryTermsStack));
        connect(queryTermsStack, SIGNAL(currentChanged(int)), p, SLOT(currentStackWidgetChanged(int)));

        vLayout->addSpacing(8);

        QHBoxLayout *hLayout = new QHBoxLayout();
        vLayout->addLayout(hLayout);
        useEntryButton = new KPushButton(i18n("Use Entry"), parent);
        hLayout->addWidget(useEntryButton);
        useEntryButton->setEnabled(false);
        connect(useEntryButton, SIGNAL(clicked()), p, SLOT(copyFromEntry()));
        hLayout->addStretch(10);

        vLayout->addStretch(100);

        return container;
    }

    QWidget* createGeneralQueryTermsForm(QWidget *parent) {
        generalQueryTermsForm = new WebSearchQueryFormGeneral(parent);
        return generalQueryTermsForm;
    }

    QWidget* createEnginesGUI(QWidget *parent) {
        listContainer = new QWidget(parent);
        QGridLayout *layout = new QGridLayout(listContainer);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);

        enginesList = new QListWidget(listContainer);
        layout->addWidget(enginesList, 0, 0, 1, 1);
        connect(enginesList, SIGNAL(itemChanged(QListWidgetItem*)), p, SLOT(itemCheckChanged(QListWidgetItem*)));
        connect(enginesList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), p, SLOT(enginesListCurrentChanged(QListWidgetItem*, QListWidgetItem*)));
        enginesList->setSelectionMode(QAbstractItemView::NoSelection);

        actionOpenHomepage = new KAction(KIcon("internet-web-browser"), i18n("Go to Homepage"), p);
        connect(actionOpenHomepage, SIGNAL(triggered()), p, SLOT(openHomepage()));
        enginesList->addAction(actionOpenHomepage);
        enginesList->setContextMenuPolicy(Qt::ActionsContextMenu);

        return listContainer;
    }

    void createGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setMargin(0);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        tabWidget = new QTabWidget(p);
        layout->addWidget(tabWidget, 0, 0, 1, 2);
        connect(tabWidget, SIGNAL(currentChanged(int)), p, SLOT(tabSwitched(int)));

        QWidget *widget = createQueryTermsStack(tabWidget);
        tabWidget->addTab(widget, KIcon("edit-rename"), i18n("Query Terms"));

        QWidget *listContainer = createEnginesGUI(tabWidget);
        tabWidget->addTab(listContainer, KIcon("applications-engineering"), i18n("Engines"));

        progressBar = new QProgressBar(p);
        layout->addWidget(progressBar, 1, 0, 1, 1);
        progressBar->setMaximum(1000);
        progressBar->hide();

        searchButton = new KPushButton(KIcon("edit-find"), i18n("Search"), p);
        layout->addWidget(searchButton, 1, 1, 1, 1);
        connect(generalQueryTermsForm, SIGNAL(returnPressed()), searchButton, SIGNAL(clicked()));

        loadEngines();
        updateGUI();
    }

    void loadEngines() {
        enginesList->clear();

        addEngine(new WebSearchBibsonomy(p));
        addEngine(new WebSearchGoogleScholar(p));
        addEngine(new WebSearchAcmPortal(p));
        addEngine(new WebSearchArXiv(p));
        addEngine(new WebSearchPubMed(p));
        addEngine(new WebSearchIEEEXplore(p));
        addEngine(new WebSearchScienceDirect(p));
        addEngine(new WebSearchSpringerLink(p));
        addEngine(new WebSearchJStor(p));

        p->itemCheckChanged(NULL);
        updateGUI();
    }

    void addEngine(WebSearchAbstract *engine) {
        KConfigGroup configGroup(config, configGroupName);

        QListWidgetItem *item = new QListWidgetItem(engine->label(), enginesList);
        item->setCheckState(configGroup.readEntry(engine->name(), false) ? Qt::Checked : Qt::Unchecked);
        item->setIcon(engine->icon());
        item->setData(HomepageRole, engine->homepage());
        item->setData(NameRole, engine->name());

        WebSearchQueryFormAbstract *widget = engine->customWidget(queryTermsStack);
        item->setData(WidgetRole, QVariant::fromValue<WebSearchQueryFormAbstract*>(widget));
        if (widget != NULL)
            queryTermsStack->addWidget(widget);

        itemToWebSearch.insert(item, engine);
        connect(engine, SIGNAL(foundEntry(Entry*)), p, SLOT(foundEntry(Entry*)));
        connect(engine, SIGNAL(stoppedSearch(int)), p, SLOT(stoppedSearch(int)));
        connect(engine, SIGNAL(progress(int, int)), p, SLOT(updateProgress(int, int)));
    }

    void switchToSearch() {
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            disconnect(searchButton, SIGNAL(clicked()), it.value(), SLOT(cancel()));

        connect(searchButton, SIGNAL(clicked()), p, SLOT(startSearch()));
        searchButton->setText(i18n("Search"));
        searchButton->setIcon(KIcon("media-playback-start"));
        tabWidget->setEnabled(true);
        tabWidget->unsetCursor();
    }

    void switchToCancel() {
        disconnect(searchButton, SIGNAL(clicked()), p, SLOT(startSearch()));

        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            connect(searchButton, SIGNAL(clicked()), it.value(), SLOT(cancel()));
        searchButton->setText(i18n("Cancel"));
        searchButton->setIcon(KIcon("media-playback-stop"));
        tabWidget->setEnabled(false);
        tabWidget->setCursor(Qt::WaitCursor);
    }

    void switchToEngines() {
        tabWidget->setCurrentWidget(listContainer);
    }

    void updateGUI() {
        if (whichEnginesLabel == NULL) return;

        QStringList checkedEngines;
        QListWidgetItem *cursor = NULL;
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            if (it.key()->checkState() == Qt::Checked) {
                checkedEngines << it.key()->text();
                cursor = it.key();
            }

        switch (checkedEngines.size()) {
        case 0: whichEnginesLabel->setText(i18n("No search engine selected. <a href=\"changeEngine\">Change</a>"));break;
        case 1: whichEnginesLabel->setText(i18n("Search engine <b>%1</b> is selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first()));break;
        case 2: whichEnginesLabel->setText(i18n("Search engines <b>%1</b> and <b>%2</b> are selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first(), checkedEngines.at(1)));break;
        case 3: whichEnginesLabel->setText(i18n("Search engines <b>%1</b>, <b>%2</b>, and <b>%3</b> are selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first(), checkedEngines.at(1), checkedEngines.at(2)));break;
        default: whichEnginesLabel->setText(i18n("Search engines <b>%1</b>, <b>%2</b>, and more are selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first(), checkedEngines.at(1)));break;
        }

        WebSearchQueryFormAbstract *currentQueryWidget = NULL;
        if (checkedEngines.size() == 1)
            currentQueryWidget = cursor->data(WidgetRole).value<WebSearchQueryFormAbstract*>();
        if (currentQueryWidget == NULL)
            currentQueryWidget = generalQueryTermsForm;
        queryTermsStack->setCurrentWidget(currentQueryWidget);
    }

    void openHomepage() {
        QListWidgetItem *item = enginesList->currentItem();
        if (item != NULL) {
            KUrl url = item->data(HomepageRole).value<KUrl>();
            QDesktopServices::openUrl(url); // TODO KDE way?
        }
    }

    void enginesListCurrentChanged(QListWidgetItem *current) {
        actionOpenHomepage->setEnabled(current != NULL);
    }

    void currentStackWidgetChanged(int index) {
        for (int i = queryTermsStack->count() - 1; i >= 0; --i) {
            WebSearchQueryFormAbstract *wsqfa = static_cast<WebSearchQueryFormAbstract*>(queryTermsStack->widget(i));
            if (i == index)
                connect(wsqfa, SIGNAL(returnPressed()), searchButton, SLOT(click()));
            else
                disconnect(wsqfa, SIGNAL(returnPressed()), searchButton, SLOT(click()));
        }
    }
};

SearchForm::SearchForm(MDIWidget *mdiWidget, SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), d(new SearchFormPrivate(mdiWidget, searchResults, this))
{
    d->createGUI();
    d->switchToSearch();
}

void SearchForm::updatedConfiguration()
{
    d->loadEngines();
}

void SearchForm::setElement(Element *element, const File *)
{
    d->currentEntry = dynamic_cast<Entry*>(element);
    d->useEntryButton->setEnabled(d->currentEntry != NULL);
}

void SearchForm::switchToEngines()
{
    d->switchToEngines();
}

void SearchForm::startSearch()
{
    WebSearchQueryFormAbstract *currentForm = d->currentQueryForm();
    if (!currentForm->readyToStart()) {
        KMessageBox::sorry(this, i18n("Could not start searching the Internet:\nThe search terms are not complete or invalid."), i18n("Searching the Internet"));
        return;
    }

    d->runningSearches.clear();
    d->sr->clear();
    d->progressBar->setValue(0);
    d->progressMap.clear();
    d->progressBar->show();

    if (currentForm == d->generalQueryTermsForm) {
        /// start search using the general-purpose form's values

        QMap<QString, QString> queryTerms = d->generalQueryTermsForm->getQueryTerms();
        int numResults = d->generalQueryTermsForm->getNumResults();
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it)
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

        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it)
            if (it.key()->checkState() == Qt::Checked) {
                it.value()->startSearch();
                d->runningSearches.insert(it.value());
            }
        if (d->runningSearches.isEmpty()) {
            /// if no search engine has been checked (selected), something went wrong
            return;
        }
    }

    d->switchToCancel();
}

void SearchForm::foundEntry(Entry*entry)
{
    d->sr->insertElement(entry);
}

void SearchForm::stoppedSearch(int resultCode)
{
    WebSearchAbstract *engine = static_cast<WebSearchAbstract *>(sender());
    if (d->runningSearches.remove(engine)) {
        kDebug() << "Search from engine" << engine->label() << "stopped with code" << resultCode  << (resultCode == 0 ? "(OK)" : "(Error)");
        if (d->runningSearches.isEmpty()) {
            /// last search engine stopped
            d->switchToSearch();
            emit doneSearching();

            QTimer::singleShot(1000, d->progressBar, SLOT(hide()));
        } else {
            QStringList remainingEngines;
            foreach(WebSearchAbstract *running, d->runningSearches) {
                remainingEngines.append(running->label());
            }
            if (!remainingEngines.isEmpty())
                kDebug() << "Remaining running engines:" << remainingEngines.join(", ");
        }
    }
}

void SearchForm::tabSwitched(int newTab)
{
    Q_UNUSED(newTab);
    d->updateGUI();
}

void SearchForm::itemCheckChanged(QListWidgetItem *item)
{
    int numCheckedEngines = 0;
    for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it)
        if (it.key()->checkState() == Qt::Checked)
            ++numCheckedEngines;

    d->searchButton->setEnabled(numCheckedEngines > 0);

    if (item != NULL) {
        KConfigGroup configGroup(d->config, d->configGroupName);
        QString name = item->data(NameRole).toString();
        configGroup.writeEntry(name, item->checkState() == Qt::Checked);
        d->config->sync();
    }
}

void SearchForm::openHomepage()
{
    d->openHomepage();
}

void SearchForm::enginesListCurrentChanged(QListWidgetItem *current, QListWidgetItem *)
{
    d->enginesListCurrentChanged(current);
}

void SearchForm::currentStackWidgetChanged(int index)
{
    d->currentStackWidgetChanged(index);
}

void SearchForm::copyFromEntry()
{
    Q_ASSERT(d->currentEntry != NULL);

    d->currentQueryForm()->copyFromEntry(*(d->currentEntry));
}

void SearchForm::updateProgress(int cur, int total)
{
    WebSearchAbstract *ws = static_cast<WebSearchAbstract*>(sender());
    d->progressMap[ws] = total > 0 ? cur * 1000 / total : 0;

    int progress = 0, count = 0;
    for (QMap<WebSearchAbstract*, int>::ConstIterator it = d->progressMap.constBegin(); it != d->progressMap.constEnd(); ++it, ++count)
        progress += it.value();

    d->progressBar->setValue(count >= 1 ? progress / count : 0);
}
