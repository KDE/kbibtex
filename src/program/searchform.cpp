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

#include "searchform.h"

class SearchForm::SearchFormPrivate
{
private:
    enum QueryFields {qfFree, qfTitle, qfAuthor};

    SearchForm *p;
    QTabWidget *tabWidget;
    KPushButton *searchButton;
    QWidget *queryTermsContainer;
    QListWidget *enginesList;
    QMap<QueryFields, KLineEdit*> queryFields;

public:
    SearchFormPrivate(SearchForm *parent)
            : p(parent) {
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
        searchButton->setEnabled(false);

        queryTermsContainer = new QWidget(tabWidget);
        tabWidget->addTab(queryTermsContainer, KIcon("edit-rename"), i18n("Query Terms"));

        layout = new QGridLayout(queryTermsContainer);
        QLabel *label = new QLabel(i18n("Free text:"), queryTermsContainer);
        layout->addWidget(label, 0, 0, 1, 1);
        KLineEdit *lineEdit = new KLineEdit(queryTermsContainer);
        layout->addWidget(lineEdit, 0, 1, 1, 1);
        queryFields.insert(qfFree, lineEdit);
        label->setBuddy(lineEdit);
        label = new QLabel(i18n("Title:"), queryTermsContainer);
        layout->addWidget(label, 1, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        queryFields.insert(qfTitle, lineEdit);
        layout->addWidget(lineEdit, 1, 1, 1, 1);
        label->setBuddy(lineEdit);
        label = new QLabel(i18n("Author:"), queryTermsContainer);
        layout->addWidget(label, 2, 0, 1, 1);
        lineEdit = new KLineEdit(queryTermsContainer);
        queryFields.insert(qfAuthor, lineEdit);
        layout->addWidget(lineEdit, 2, 1, 1, 1);
        label->setBuddy(lineEdit);
        layout->setRowStretch(4, 100);

        enginesList = new QListWidget(tabWidget);
        tabWidget->addTab(enginesList, KIcon("applications-engineering"), i18n("Engines"));
        enginesList->setSelectionMode(QAbstractItemView::NoSelection);

        loadEngines();

        connect(enginesList, SIGNAL(itemActivated(QListWidgetItem*)), p, SLOT(formChanged()));
    }

    void loadEngines() {
        enginesList->clear();

        QListWidgetItem *item = new QListWidgetItem("Google", enginesList);
        item->setCheckState(Qt::Checked);
        item = new QListWidgetItem("ArXiv", enginesList);
        item->setCheckState(Qt::Unchecked);
    }
};

SearchForm::SearchForm(QWidget *parent)
        : QWidget(parent), d(new SearchFormPrivate(this))
{
    d->createGUI();
}

void SearchForm::updatedConfiguration()
{
    d->loadEngines();
}

void SearchForm::formChanged()
{
    qDebug("formChanged");
}

