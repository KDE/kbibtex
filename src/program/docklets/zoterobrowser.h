/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_PROGRAM_DOCKLET_ZOTEROBROWSER_H
#define KBIBTEX_PROGRAM_DOCKLET_ZOTEROBROWSER_H

#include <QWidget>
#include <QModelIndex>

class Element;
class SearchResults;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class ZoteroBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit ZoteroBrowser(SearchResults *searchResults, QWidget *parent);
    ~ZoteroBrowser() override;

public slots:
    void visibiltyChanged(bool);

signals:
    void itemToShow();

private:
    class Private;
    Private *const d;

private slots:
    void modelReset();
    void collectionDoubleClicked(const QModelIndex &index);
    void tagDoubleClicked(const QModelIndex &index);
    void showItem(QSharedPointer<Element>);
    void reenableWidget();
    void updateButtons();
    bool applyCredentials();
    void radioButtonsToggled();
    void groupListChanged();
    void retrieveGroupList();
    void invalidateGroupList();
    void gotGroupList();
    void getOAuthCredentials();
    void readOAuthCredentials(bool);
    void writeOAuthCredentials(bool);
    void tabChanged(int);
};

#endif // KBIBTEX_PROGRAM_DOCKLET_ZOTEROBROWSER_H
