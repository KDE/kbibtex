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

#ifndef KBIBTEX_PROGRAM_SEARCHFORM_H
#define KBIBTEX_PROGRAM_SEARCHFORM_H

#include <QWidget>

class QListWidgetItem;

class Entry;
class MDIWidget;
class SearchResults;

class SearchForm : public QWidget
{
    Q_OBJECT

public:
    SearchForm(MDIWidget *mdiWidget, SearchResults *searchResults, QWidget *parent);

signals:
    void doneSearching();

public slots:
    void updatedConfiguration();

private:
    class SearchFormPrivate;
    SearchFormPrivate *d;

private slots:
    void switchToEngines();
    void startSearch();
    void foundEntry(Entry *entry);
    void stoppedSearch(int resultCode);
    void tabSwitched(int newTab);
    void itemCheckChanged();
    void openHomepage();
    void enginesListCurrentChanged(QListWidgetItem*, QListWidgetItem*);
    void currentStackWidgetChanged(int);
};


#endif // KBIBTEX_PROGRAM_SEARCHFORM_H
