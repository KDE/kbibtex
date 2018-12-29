/***************************************************************************
 *   Copyright (C) 2017-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "searchenginelist.h"

#include <QSettings>

#include "onlinesearchabstract.h"
#include "onlinesearchacmportal.h"
#include "onlinesearcharxiv.h"
#include "onlinesearchbibsonomy.h"
#include "onlinesearchgooglescholar.h"
#include "onlinesearchieeexplore.h"
#include "onlinesearchingentaconnect.h"
#include "onlinesearchjstor.h"
#include "onlinesearchpubmed.h"
#include "onlinesearchsciencedirect.h"
#include "onlinesearchspringerlink.h"
#include "bibliographymodel.h"

SearchEngineList::SearchEngineList()
{
    OnlineSearchAbstract *osa = new OnlineSearchAcmPortal(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchArXiv(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchBibsonomy(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchGoogleScholar(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchIEEEXplore(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchIngentaConnect(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchJStor(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchScienceDirect(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchPubMed(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    osa = new OnlineSearchSpringerLink(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);

    connect(this,&SearchEngineList::dataChanged,[this](const QModelIndex &, const QModelIndex &, const QVector<int> &roles = QVector<int>()){
        if (roles.contains(EngineEnabledRole))
            emit searchEngineCountChanged();
    });
}

SearchEngineList::SearchEngineList(const SearchEngineList &other)
    : QAbstractListModel(), QVector<OnlineSearchAbstract *>(other)
{
    /// Nothing to do here, QVector constructor does the heavy lifting
}

SearchEngineList *SearchEngineList::operator =(const SearchEngineList *other) {
    /// Not much to do here, QVector constructor does the heavy lifting
    QVector<OnlineSearchAbstract *>::operator =(*other);
    return this;
}

int SearchEngineList::rowCount(const QModelIndex &parent) const {
    return parent == QModelIndex() ? size() : 0;
}

QVariant SearchEngineList::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= size() || index.parent() != QModelIndex() || index.column() != 0)
        return QVariant();
    const int row = index.row();

    switch (role) {
    case Qt::DisplayRole:
        return at(row)->label();
        break;
    case EngineEnabledRole: {
        const QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
        return isSearchEngineEnabled(settings, at(row));
    }
    default:
        return QVariant();
    }
}

bool SearchEngineList::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (index.row() < 0 || index.row() >= size() || index.parent() != QModelIndex() || index.column() != 0 || role != EngineEnabledRole)
        return false;

    static QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
    const int row = index.row();
    const bool toBeSetEnabled = value.toBool();
    const bool currentlyEnabled = isSearchEngineEnabled(settings, at(row));

    if (toBeSetEnabled != currentlyEnabled) {
        setSearchEngineEnabled(settings, at(row), toBeSetEnabled);
        settings.sync();
        static const QVector<int> roles = QVector<int>(1, EngineEnabledRole);
        emit dataChanged(index, index, roles);
    }

    return true;
}

QString SearchEngineList::humanReadableSearchEngines() const {
    if (empty()) return QString();

    const QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
    QStringList enabledSearchEnginesLabels;
    for (int i = 0; i < size(); ++i)
        if (isSearchEngineEnabled(settings, at(i)))
            enabledSearchEnginesLabels.append(at(i)->label());
    if (enabledSearchEnginesLabels.isEmpty()) return QString();

    QString result = enabledSearchEnginesLabels.first();
    if (enabledSearchEnginesLabels.size() == 1) return result;
    if (enabledSearchEnginesLabels.size() == 2) return result.append(" and ").append(enabledSearchEnginesLabels.last());
    /// assertion: enabledSearchEnginesLabels.size() >= 3
    for (int i = 1 ; i < enabledSearchEnginesLabels.size() - 1; ++i)
        result.append(QStringLiteral(", ")).append(enabledSearchEnginesLabels[i]);
    result.append(", and ").append(enabledSearchEnginesLabels.last());

    return result;
}

int SearchEngineList::getSearchEngineCount() const {
    if (empty()) return 0;

    const QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
    int count = 0;
    for (int i = 0; i < size(); ++i)
        if (isSearchEngineEnabled(settings, at(i)))
            ++count;
    return count;
}

QHash<int, QByteArray> SearchEngineList::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[LabelRole] = "label";
    roles[EngineEnabledRole] = "engineEnabled";
    return roles;
}
