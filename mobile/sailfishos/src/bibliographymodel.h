/***************************************************************************
 *   Copyright (C) 2016-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef BIBLIOGRAPHY_MODEL_H
#define BIBLIOGRAPHY_MODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QSet>

#include "file.h"
#include "value.h"
#include "entry.h"
#include "searchenginelist.h"

class OnlineSearchAbstract;
class BibliographyModel;

class BibliographyRoles
{
public:
    enum Roles {BibTeXIdRole = Qt::UserRole + 100, FoundViaRole = Qt::UserRole + 102, EntryRole = Qt::UserRole + 999, TitleRole = Qt::UserRole + 1000, AuthorRole = Qt::UserRole + 1010, AuthorShortRole = Qt::UserRole + 1011, YearRole = Qt::UserRole + 1020, WherePublishedRole = Qt::UserRole + 1100, UrlRole = Qt::UserRole + 1110, DoiRole = Qt::UserRole + 1111};
};

class SortedBibliographyModel : public QSortFilterProxyModel, public BibliographyRoles {
    Q_OBJECT
public:
    static const int SortAuthorNewestTitle, SortAuthorOldestTitle, SortNewestAuthorTitle, SortOldestAuthorTitle;

    SortedBibliographyModel();
    ~SortedBibliographyModel();

    virtual QHash<int, QByteArray> roleNames() const;

    bool isBusy() const;
    int sortOrder() const;
    void setSortOrder(int sortOrder);
    Q_PROPERTY(int sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_INVOKABLE QStringList humanReadableSortOrder() const;

    Q_INVOKABLE void startSearch(const QString &freeText, const QString &title, const QString &author);
    Q_INVOKABLE void clear();
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

signals:
    void busyChanged();
    void sortOrderChanged(int sortOrder);

protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
    enum SortingTriState {True = -1, Undecided = 0, False = 1};
    enum AgeSorting {MostRecentFirst = 0, LeastRecentFirst = 1};

    SortingTriState compareAuthors(const QSharedPointer<const Entry> entryLeft, const QSharedPointer<const Entry> entryRight, SortingTriState previousDecision = Undecided) const;
    SortingTriState compareYears(const QSharedPointer<const Entry> entryLeft, const QSharedPointer<const Entry> entryRight, AgeSorting ageSorting, SortingTriState previousDecision = Undecided) const;
    SortingTriState compareTitles(const QSharedPointer<const Entry> entryLeft, const QSharedPointer<const Entry> entryRight, SortingTriState previousDecision = Undecided) const;

    int m_sortOrder;
    BibliographyModel *model;

    QString removeNobiliaryParticle(const QString &lastname) const;
};

class BibliographyModel : public QAbstractListModel, public BibliographyRoles {
    Q_OBJECT
public:
    explicit BibliographyModel();
    ~BibliographyModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    const QSharedPointer<const Entry> entry(int row) const;
    static QString valueToText(const Value &value);

    bool isBusy() const;

    void startSearch(const QString &freeText, const QString &title, const QString &author);
    void clear();

signals:
    void busyChanged();

private slots:
    void newEntry(QSharedPointer<Entry>);
    void searchFinished();

private:
    File *m_file;
    int m_runningSearches;
    SearchEngineList *m_searchEngineList;

    static QStringList valueToList(const Value &value);
    static QString personToText(const QSharedPointer<const Person> &person);
    static QString valueItemToText(const QSharedPointer<ValueItem> &valueItem);
    static QString beautifyLaTeX(const QString &input);
};

#endif // BIBLIOGRAPHY_MODEL_H
