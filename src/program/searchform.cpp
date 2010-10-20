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

#include <KPushButton>
#include <KLineEdit>
#include <KLocale>
#include <KIcon>
#include <KDebug>
#include <KMessageBox>
#include <KTemporaryFile>
#include <kparts/part.h>

#include <websearchabstract.h>
#include <websearchbibsonomy.h>
#include <websearchgooglescholar.h>
#include <fileexporterbibtex.h>
#include <file.h>
#include <comment.h>
#include <openfileinfo.h>
#include <bibtexeditor.h>
#include <bibtexfilemodel.h>
#include <mdiwidget.h>
#include <searchresults.h>
#include "searchform.h"

class SearchForm::SearchFormPrivate
{
private:
    SearchForm *p;
    QTabWidget *tabWidget;
    QWidget *queryTermsContainer;
    QWidget *listContainer;
    QListWidget *enginesList;
    QLabel *whichEnginesLabel;

public:
    MDIWidget *m;
    SearchResults *sr;
    QMap<QListWidgetItem*, WebSearchAbstract*> itemToWebSearch;
    QMap<QString, KLineEdit*> queryFields;
    QSpinBox *numResultsField;
    int runningSearches;
    KPushButton *searchButton;

    SearchFormPrivate(MDIWidget *mdiWidget, SearchResults *searchResults, SearchForm *parent)
            : p(parent), whichEnginesLabel(NULL), m(mdiWidget), sr(searchResults), runningSearches(0) {
// TODO
    }

    virtual ~SearchFormPrivate() {
    }

    void createGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        tabWidget = new QTabWidget(p);
        layout->addWidget(tabWidget, 0, 0, 1, 2);
        connect(tabWidget, SIGNAL(currentChanged(int)), p, SLOT(tabSwitched(int)));

        searchButton = new KPushButton(KIcon("edit-find"), i18n("Search"), p);
        layout->addWidget(searchButton, 1, 1, 1, 1);

        queryTermsContainer = new QWidget(tabWidget);
        tabWidget->addTab(queryTermsContainer, KIcon("edit-rename"), i18n("Query Terms"));

        layout = new QGridLayout(queryTermsContainer);
        whichEnginesLabel = new QLabel(queryTermsContainer);
        whichEnginesLabel->setWordWrap(true);
        layout->addWidget(whichEnginesLabel, 0, 0, 1, 2);
        connect(whichEnginesLabel, SIGNAL(linkActivated(QString)), p, SLOT(switchToEngines()));

        QSpacerItem *spacer = new QSpacerItem(8, 8);
        layout->addItem(spacer, 1, 0, 1, 1);

        QLabel *label = new QLabel(i18n("Free text:"), queryTermsContainer);
        layout->addWidget(label, 2, 0, 1, 1);
        KLineEdit *lineEdit = new KLineEdit(queryTermsContainer);
        lineEdit->setClearButtonShown(true);
        layout->addWidget(lineEdit, 2, 1, 1, 1);
        queryFields.insert(WebSearchAbstract::queryKeyFreeText, lineEdit);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Title:"), queryTermsContainer);
        layout->addWidget(label, 3, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        lineEdit->setClearButtonShown(true);
        queryFields.insert(WebSearchAbstract::queryKeyTitle, lineEdit);
        layout->addWidget(lineEdit, 3, 1, 1, 1);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Author:"), queryTermsContainer);
        layout->addWidget(label, 4, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        lineEdit->setClearButtonShown(true);
        queryFields.insert(WebSearchAbstract::queryKeyAuthor, lineEdit);
        layout->addWidget(lineEdit, 4, 1, 1, 1);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Year:"), queryTermsContainer);
        layout->addWidget(label, 5, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        lineEdit->setClearButtonShown(true);
        queryFields.insert(WebSearchAbstract::queryKeyYear, lineEdit);
        layout->addWidget(lineEdit, 5, 1, 1, 1);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Number of Results:"), queryTermsContainer);
        layout->addWidget(label, 6, 0, 1, 1);
        numResultsField = new QSpinBox(queryTermsContainer);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);
        layout->addWidget(numResultsField, 6, 1, 1, 1);
        label->setBuddy(numResultsField);

        layout->setRowStretch(7, 100);

        listContainer = new QWidget(tabWidget);
        tabWidget->addTab(listContainer, KIcon("applications-engineering"), i18n("Engines"));
        layout = new QGridLayout(listContainer);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);

        enginesList = new QListWidget(listContainer);
        layout->addWidget(enginesList, 0, 0, 1, 1);
        connect(enginesList, SIGNAL(itemChanged(QListWidgetItem*)), p, SLOT(itemCheckChanged()));
        enginesList->setSelectionMode(QAbstractItemView::NoSelection);

        loadEngines();
    }

    void loadEngines() {
        enginesList->clear();

        addEngine(new WebSearchBibsonomy(p));
        addEngine(new WebSearchGoogleScholar(p));

        p->itemCheckChanged();
        updateGUI();
    }

    void addEngine(WebSearchAbstract *engine) {
        QListWidgetItem *item = new QListWidgetItem(engine->label(), enginesList);
        item->setCheckState(Qt::Checked);
        itemToWebSearch.insert(item, engine);
        connect(engine, SIGNAL(foundEntry(Entry*)), p, SLOT(foundEntry(Entry*)));
        connect(engine, SIGNAL(stoppedSearch(int)), p, SLOT(stoppedSearch(int)));
    }

    void switchToSearch() {
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            disconnect(searchButton, SIGNAL(clicked()), it.value(), SLOT(cancel()));

        for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it)
            connect(it.value(), SIGNAL(returnPressed(const QString &)), p, SLOT(startSearch()));

        connect(searchButton, SIGNAL(clicked()), p, SLOT(startSearch()));
        searchButton->setText(i18n("Search"));
        searchButton->setIcon(KIcon("media-playback-start"));
        tabWidget->setEnabled(true);
        sr->setEnabled(true);
        tabWidget->unsetCursor();
        sr->unsetCursor();
    }

    void switchToCancel() {
        disconnect(searchButton, SIGNAL(clicked()), p, SLOT(startSearch()));
        for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it)
            disconnect(it.value(), SIGNAL(returnPressed(const QString &)), p, SLOT(startSearch()));

        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            connect(searchButton, SIGNAL(clicked()), it.value(), SLOT(cancel()));
        searchButton->setText(i18n("Cancel"));
        searchButton->setIcon(KIcon("media-playback-stop"));
        tabWidget->setEnabled(false);
        sr->setEnabled(false);
        tabWidget->setCursor(Qt::WaitCursor);
        sr->setCursor(Qt::WaitCursor);
    }

    void switchToEngines() {
        tabWidget->setCurrentWidget(listContainer);
    }

    void updateGUI() {
        if (whichEnginesLabel == NULL) return;

        QStringList checkedEngines;
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            if (it.key()->checkState() == Qt::Checked)
                checkedEngines << it.key()->text();

        switch (checkedEngines.size()) {
        case 0: whichEnginesLabel->setText(i18n("No search engine selected. <a href=\"changeEngine\">Change</a>"));break;
        case 1: whichEnginesLabel->setText(i18n("Search engine <b>%1</b> is selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first()));break;
        case 2: whichEnginesLabel->setText(i18n("Search engines <b>%1</b> and <b>%2</b> are selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first(), checkedEngines.at(1)));break;
        case 3:whichEnginesLabel->setText(i18n("Search engines <b>%1</b>, <b>%2</b>, and <b>%3</b> are selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first(), checkedEngines.at(1), checkedEngines.at(2)));break;
        default:whichEnginesLabel->setText(i18n("Search engines <b>%1</b>, <b>%2</b>, and more are selected. <a href=\"changeEngine\">Change</a>", checkedEngines.first(), checkedEngines.at(1)));break;
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

void SearchForm::switchToEngines()
{
    d->switchToEngines();
}

void SearchForm::startSearch()
{
    QMap<QString, QString> queryTerms;

    for (QMap<QString, KLineEdit*>::ConstIterator it = d->queryFields.constBegin(); it != d->queryFields.constEnd(); ++it) {
        if (!it.value()->text().isEmpty())
            queryTerms.insert(it.key(), it.value()->text());
    }

    if (queryTerms.isEmpty()) {
        KMessageBox::error(this, i18n("Could not start searching the Internet:\nNo search terms entered."), i18n("Searching the Internet"));
        return;
    }

    d->runningSearches = 0;
    d->sr->clear();

    for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it)
        if (it.key()->checkState() == Qt::Checked) {
            it.value()->startSearch(queryTerms, d->numResultsField->value());
            ++d->runningSearches;
        }
    if (d->runningSearches <= 0) {
        /// if no search engine has been checked (selected), something went wrong
        return;
    }

    d->switchToCancel();
}

void SearchForm::foundEntry(Entry*entry)
{
    d->sr->insertElement(entry);
}

void SearchForm::stoppedSearch(int resultCode)
{
    WebSearchAbstract *engine = dynamic_cast<WebSearchAbstract *>(sender());
    kDebug() << " search from engine " << engine->label() << " stopped with code " << resultCode << " (" << (resultCode == 0 ? "OK)" : "Error)");

    --d->runningSearches;
    if (d->runningSearches <= 0) {
        /// last search engine stopped
        d->switchToSearch();
        emit doneSearching();
    }
}

void SearchForm::tabSwitched(int newTab)
{
    Q_UNUSED(newTab);
    d->updateGUI();
}

void SearchForm::itemCheckChanged()
{
    int numCheckedEngines = 0;
    for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it)
        if (it.key()->checkState() == Qt::Checked)
            ++numCheckedEngines;

    d->searchButton->setEnabled(numCheckedEngines > 0);
}
