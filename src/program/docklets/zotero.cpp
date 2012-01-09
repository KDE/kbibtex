/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QGridLayout>
#include <QTreeView>
#include <QSplitter>

#include <KLineEdit>
#include <KPushButton>
#include <KLocale>

#include "clipboard.h"
#include "bibtexfilemodel.h"
#include "bibtexeditor.h"
#include <zotero/readapi.h>
#include "zotero.h"

class Zotero::ZoteroPrivate
{
private:
    Zotero *p;

public:
    BibTeXEditor *editor;
    ZoteroReadAPICollectionsModel *collectionsModel;
    BibTeXFileModel *bibTeXmodel;
    SortFilterBibTeXFileModel *sortedModel;
    ZoteroReadAPI *readAPI;
    KPushButton *buttonSync;
    QTreeView *zoteroCollectionsTree;
    BibTeXEditor *zoteroItemsTree;
    Clipboard *clipboard;

    ZoteroPrivate(Zotero *parent)
            : p(parent) {
        collectionsModel = new ZoteroReadAPICollectionsModel(p);
        readAPI = new ZoteroReadAPI(p);

        bibTeXmodel = new BibTeXFileModel(p);
        sortedModel = new SortFilterBibTeXFileModel(p);
        bibTeXmodel->setBibTeXFile(&(readAPI->bibTeXfile));
        sortedModel->setSourceModel(bibTeXmodel);
        setupGUI();
    }

    ~ZoteroPrivate() {
        delete collectionsModel;
        delete readAPI;
        delete clipboard;
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        buttonSync = new KPushButton(KIcon("system-software-update"), QString(), p);
        layout->addWidget(buttonSync, 0, 1, 1, 1);
        buttonSync->setToolTip(i18n("Synchronize with Zotero"));

        QSplitter *splitter = new QSplitter(Qt::Vertical, p);
        layout->addWidget(splitter, 1, 0, 1, 2);

        zoteroCollectionsTree = new QTreeView(splitter);
        splitter->addWidget(zoteroCollectionsTree);
        zoteroCollectionsTree->setModel(collectionsModel);
        zoteroCollectionsTree->setAllColumnsShowFocus(true);

        zoteroItemsTree = new BibTeXEditor(QLatin1String("Zotero"), splitter);
        splitter->addWidget(zoteroItemsTree);
        zoteroItemsTree->setModel(sortedModel);
        zoteroItemsTree->setAllColumnsShowFocus(true);
        zoteroItemsTree->setReadOnly(true);
        zoteroItemsTree->setFrameShadow(QFrame::Sunken);
        zoteroItemsTree->setFrameShape(QFrame::StyledPanel);

        clipboard = new Clipboard(zoteroItemsTree);
    }
};

Zotero::Zotero(QWidget *parent)
        : QWidget(parent), d(new ZoteroPrivate(this))
{
    connect(d->readAPI, SIGNAL(busy(bool)), this, SLOT(changeBusyStatus(bool)));
    connect(d->readAPI, SIGNAL(done(ZoteroReadAPI*)), d->collectionsModel, SLOT(setReadAPI(ZoteroReadAPI*)));
    connect(d->zoteroCollectionsTree, SIGNAL(clicked(QModelIndex)), this, SLOT(updateFilter(QModelIndex)));
    connect(d->buttonSync, SIGNAL(clicked()), d->readAPI, SLOT(scanLibrary()));
}

Zotero::~Zotero()
{
    delete d;
}

void Zotero::setEditor(BibTeXEditor *editor)
{
    d->editor = editor;
}

void Zotero::changeBusyStatus(bool isBusy)
{
    if (isBusy)
        setCursor(Qt::WaitCursor);
    else
        unsetCursor();

    setEnabled(!isBusy);
    d->bibTeXmodel->reset();
}

void Zotero::updateFilter(const QModelIndex &index)
{
    SortFilterBibTeXFileModel::FilterQuery fq;
    fq.terms = index.data(ZoteroReadAPICollectionsModel::FilterStringListRole).toStringList();
    fq.combination = SortFilterBibTeXFileModel::AnyTerm;
    fq.field = QLatin1String("^id");
    fq.searchPDFfiles = false;
    d->sortedModel->updateFilter(fq);
}
