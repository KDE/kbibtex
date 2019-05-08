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

#ifndef KBIBTEX_PROGRAM_DOCKLET_SEARCHFORM_H
#define KBIBTEX_PROGRAM_DOCKLET_SEARCHFORM_H

#include <QWidget>

class QListWidgetItem;

class Element;
class File;
class Entry;
class MDIWidget;
class SearchResults;

/**
 * Widget for a dock widget where users can select search engines and
 * enter search queries (title, author, number of hits, ...)
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class SearchForm : public QWidget
{
    Q_OBJECT

public:
    /**
     * Create a new search form, which has to know where to store results
     * and which (dock) widget is its parent.
     * @param searchResults SearchResults object where to send results to
     * @param parent parent widget for this widget
     */
    SearchForm(SearchResults *searchResults, QWidget *parent);
    ~SearchForm() override;

signals:
    /**
     * This signal gets emitted once the last of possibly several parallel
     * online searches is done.
     */
    void doneSearching();

public slots:
    /**
     * Notify this widget about changes in the configuration settings,
     * e.g. to update its list of search engines.
     */
    void updatedConfiguration();

    /**
     * Notify this widget about a new current element selected in the
     * main list view. Allows the widget to put use in the "Use Entry"
     * button, i.e. copy title, author, etc from the entry to the search
     * form.
     * @param element BibTeX element, which will be cast to an Entry internally
     * @param file BibTeX file where entry belongs to
     */
    void setElement(QSharedPointer<Element> element, const File *file);

private:
    class SearchFormPrivate;
    SearchFormPrivate *d;

private slots:
    void switchToEngines();
    void startSearch();
    void foundEntry(QSharedPointer<Entry> entry);
    void stoppedSearch(int resultCode);
    void tabSwitched(int newTab);
    void itemCheckChanged(QListWidgetItem *);
    void openHomepage();
    void enginesListCurrentChanged(QListWidgetItem *, QListWidgetItem *);
    void copyFromEntry();
    void updateProgress(int, int);
};

#endif // KBIBTEX_PROGRAM_DOCKLET_SEARCHFORM_H
