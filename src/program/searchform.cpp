/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <KPushButton>
#include <KLineEdit>
#include <KLocale>
#include <KIcon>
#include <KDebug>
#include <KMessageBox>
#include <KTemporaryFile>

#include <websearchabstract.h>
#include <websearchbibsonomy.h>
#include <fileexporterbibtex.h>
#include <file.h>
#include <comment.h>
#include <openfileinfo.h>
#include "searchform.h"

class SearchForm::SearchFormPrivate
{
private:
    SearchForm *p;
    QTabWidget *tabWidget;
    KPushButton *searchButton;
    QWidget *queryTermsContainer;
    QListWidget *enginesList;

public:
    QMap<QListWidgetItem*, WebSearchAbstract*> itemToWebSearch;
    QMap<QString, KLineEdit*> queryFields;
    int runningSearches;
    File *bibtexFile;

    SearchFormPrivate(SearchForm *parent)
            : p(parent), runningSearches(0), bibtexFile(NULL) {
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

        searchButton = new KPushButton(KIcon("edit-find"), i18n("Search"), p);
        layout->addWidget(searchButton, 1, 1, 1, 1);

        queryTermsContainer = new QWidget(tabWidget);
        tabWidget->addTab(queryTermsContainer, KIcon("edit-rename"), i18n("Query Terms"));

        layout = new QGridLayout(queryTermsContainer);
        QLabel *label = new QLabel(i18n("Free text:"), queryTermsContainer);
        layout->addWidget(label, 0, 0, 1, 1);
        KLineEdit *lineEdit = new KLineEdit(queryTermsContainer);
        layout->addWidget(lineEdit, 0, 1, 1, 1);
        queryFields.insert(WebSearchAbstract::queryKeyFreeText, lineEdit);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Title:"), queryTermsContainer);
        layout->addWidget(label, 1, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        queryFields.insert(WebSearchAbstract::queryKeyTitle, lineEdit);
        layout->addWidget(lineEdit, 1, 1, 1, 1);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Author:"), queryTermsContainer);
        layout->addWidget(label, 2, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        queryFields.insert(WebSearchAbstract::queryKeyAuthor, lineEdit);
        layout->addWidget(lineEdit, 2, 1, 1, 1);
        label->setBuddy(lineEdit);

        label = new QLabel(i18n("Year:"), queryTermsContainer);
        layout->addWidget(label, 3, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        queryFields.insert(WebSearchAbstract::queryKeyYear, lineEdit);
        layout->addWidget(lineEdit, 3, 1, 1, 1);
        label->setBuddy(lineEdit);

        layout->setRowStretch(4, 100);

        QWidget *listContainer = new QWidget(tabWidget);
        tabWidget->addTab(listContainer, KIcon("applications-engineering"), i18n("Engines"));
        layout = new QGridLayout(listContainer);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);

        enginesList = new QListWidget(listContainer);
        layout->addWidget(enginesList, 0, 0, 1, 1);
        enginesList->setSelectionMode(QAbstractItemView::NoSelection);
        label = new QLabel(i18n("Selecting no engine will make the search to use all engines."), listContainer);
        label->setWordWrap(true);
        layout->addWidget(label, 1, 0, 1, 1);

        loadEngines();
    }

    void loadEngines() {
        enginesList->clear();

        addEngine(new WebSearchBibsonomy());
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
        connect(searchButton, SIGNAL(clicked()), p, SLOT(startSearch()));
        searchButton->setText(i18n("Search"));
        searchButton->setIcon(KIcon("media-playback-start"));
        tabWidget->setEnabled(true);
    }

    void switchToCancel() {
        disconnect(searchButton, SIGNAL(clicked()), p, SLOT(startSearch()));
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = itemToWebSearch.constBegin(); it != itemToWebSearch.constEnd(); ++it)
            connect(searchButton, SIGNAL(clicked()), it.value(), SLOT(cancel()));
        searchButton->setText(i18n("Cancel"));
        searchButton->setIcon(KIcon("media-playback-stop"));
        tabWidget->setEnabled(false);
    }
};

SearchForm::SearchForm(QWidget *parent)
        : QWidget(parent), d(new SearchFormPrivate(this))
{
    d->createGUI();
    d->switchToSearch();
}

void SearchForm::updatedConfiguration()
{
    d->loadEngines();
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
    d->bibtexFile = new File();
    QStringList queryComment;
    for (QMap<QString, QString>::ConstIterator it = queryTerms.constBegin(); it != queryTerms.constEnd(); ++it)
        queryComment.append(it.key() + "->" + it.value());
    Comment *comment = new Comment(QLatin1String("x-kbibtex-search=yes\nx-kbibtex-search-query=") + queryComment.join(";"));
    d->bibtexFile->append(comment);

    for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it)
        if (it.key()->checkState() == Qt::Checked) {
            it.value()->startSearch(queryTerms, 20); // FIXME number of hits
            ++d->runningSearches;
        }
    if (d->runningSearches <= 0) {
        /// if no search engine has been checked (selected), use all engines at once
        for (QMap<QListWidgetItem*, WebSearchAbstract*>::ConstIterator it = d->itemToWebSearch.constBegin(); it != d->itemToWebSearch.constEnd(); ++it) {
            it.value()->startSearch(queryTerms, 20); // FIXME number of hits
            ++d->runningSearches;
        }
    }

    d->switchToCancel();
}

void SearchForm::foundEntry(Entry*entry)
{
    d->bibtexFile->append(entry);
}

void SearchForm::stoppedSearch(int resultCode)
{
    WebSearchAbstract *engine = dynamic_cast<WebSearchAbstract *>(sender());
    kDebug() << " search from engine " << engine->label() << " stopped with code " << resultCode << " (" << (resultCode == 0 ? "OK)" : "Error)");

    --d->runningSearches;
    if (d->runningSearches <= 0) {
        KTemporaryFile tempFile;
        tempFile.setSuffix(".bib");
        if (tempFile.open()) {
            QFile file(tempFile.fileName());
            file.open(QFile::WriteOnly);
            FileExporterBibTeX exporter;
            exporter.save(&file, d->bibtexFile);
            file.close();
            kDebug() << d->bibtexFile->count() << "  " << tempFile.fileName();
            OpenFileInfo *openFileInfo =   OpenFileInfoManager::getOpenFileInfoManager()->open(QLatin1String("file://") + tempFile.fileName());
            OpenFileInfoManager::getOpenFileInfoManager()->setCurrentFile(openFileInfo);
            kDebug() << openFileInfo->caption();
        }

        d->switchToSearch();
        delete d->bibtexFile;
    }

}
