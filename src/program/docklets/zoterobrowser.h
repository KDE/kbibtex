/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_PROGRAM_ZOTEROBROWSER_H
#define KBIBTEX_PROGRAM_ZOTEROBROWSER_H

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
    ~ZoteroBrowser();

private:
    class Private;
    Private *const d;

    void setupGUI();

private slots:
    void modelReset();
    void collectionDoubleClicked(const QModelIndex &index);
    void tagDoubleClicked(const QModelIndex &index);
    void showItem(QSharedPointer<Element>);
    void reenableWidget();
    void updateButtons();
    void applyCredentials();
    void radioButtonsToggled();
    void retrieveGroupList();
    void invalidateGroupList();
    void gotGroupList();
#ifdef HAVE_QTOAUTH // krazy:exclude=cpp
    void getOAuthCredentials();
#endif // HAVE_QTOAUTH
};


#endif // KBIBTEX_PROGRAM_ZOTEROBROWSER_H
