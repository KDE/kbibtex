/***************************************************************************
 *   Copyright (C) 2017-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <KLocalizedString>

#include "onlinesearch/onlinesearchabstract.h"
#include "onlinesearch/onlinesearchacmportal.h"
#include "onlinesearch/onlinesearcharxiv.h"
#include "onlinesearch/onlinesearchbibsonomy.h"
#include "onlinesearch/onlinesearchgooglescholar.h"
#include "onlinesearch/onlinesearchieeexplore.h"
#include "onlinesearch/onlinesearchingentaconnect.h"
#include "onlinesearch/onlinesearchjstor.h"
#include "onlinesearch/onlinesearchpubmed.h"
#include "onlinesearch/onlinesearchsciencedirect.h"
#include "onlinesearch/onlinesearchspringerlink.h"
#include "bibliographymodel.h"

SearchEngineList::SearchEngineList()
{
    OnlineSearchAbstract *osa = new OnlineSearchAcmPortal(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchArXiv(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchBibsonomy(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchGoogleScholar(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchIEEEXplore(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchIngentaConnect(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchJStor(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchScienceDirect(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchPubMed(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    osa = new OnlineSearchSpringerLink(this);
    append(osa);
    connect(osa, &OnlineSearchAbstract::foundEntry, this, &SearchEngineList::foundEntry);
    connect(osa, &OnlineSearchAbstract::busyChanged, this, &SearchEngineList::busyChanged);
    connect(osa, &OnlineSearchAbstract::progress, this, &SearchEngineList::collectingProgress);

    connect(this, &SearchEngineList::dataChanged, [this](const QModelIndex &, const QModelIndex &, const QVector<int> &roles = QVector<int>()) {
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
    case EngineEnabledRole: {
        // TODO make it configurable which search engines to start
        return true;
    }
    default:
        return QVariant();
    }
}

bool SearchEngineList::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (index.row() < 0 || index.row() >= size() || index.parent() != QModelIndex() || index.column() != 0 || role != EngineEnabledRole)
        return false;

    // TODO make it configurable which search engines to start
    Q_UNUSED(value)

    return true;
}

QString SearchEngineList::humanReadableSearchEngines() const {
    if (empty()) return QString();

    QStringList enabledSearchEnginesLabels;
    for (int i = 0; i < size(); ++i)
        // TODO make it configurable which search engines to start/enable
        enabledSearchEnginesLabels.append(at(i)->label());

    switch (enabledSearchEnginesLabels.size()) {
    case 0: return QString(); ///< empty selection
    case 1: return enabledSearchEnginesLabels.first(); ///< just one search engine selected
    case 2:
        //: Two search engines selected
        //% "%1 and %2"
        return i18n("human-readable-two-search-engines").arg(enabledSearchEnginesLabels.at(0), enabledSearchEnginesLabels.at(1));
    case 3:
        //: Three search engines selected
        //% "%1, %2, and %3"
        return i18n("human-readable-three-search-engines").arg(enabledSearchEnginesLabels.at(0), enabledSearchEnginesLabels.at(1), enabledSearchEnginesLabels.at(2));
    case 4:
        //: Four search engines selected
        //% "%1, %2, %3, and %4"
        return i18n("human-readable-four-search-engines").arg(enabledSearchEnginesLabels.at(0), enabledSearchEnginesLabels.at(1), enabledSearchEnginesLabels.at(2), enabledSearchEnginesLabels.at(3));
    case 5:
        //: Five search engines selected
        //% "%1, %2, %3, %4, and %5"
        return i18n("human-readable-five-search-engines").arg(enabledSearchEnginesLabels.at(0), enabledSearchEnginesLabels.at(1), enabledSearchEnginesLabels.at(2), enabledSearchEnginesLabels.at(3), enabledSearchEnginesLabels.at(4));
    case 6:
        //: Six search engines selected
        //% "%1, %2, %3, %4, %5, and %6"
        return i18n("human-readable-six-search-engines").arg(enabledSearchEnginesLabels.at(0), enabledSearchEnginesLabels.at(1), enabledSearchEnginesLabels.at(2), enabledSearchEnginesLabels.at(3), enabledSearchEnginesLabels.at(4), enabledSearchEnginesLabels.at(5));
    case 7:
        //: Seven search engines selected
        //% "%1, %2, %3, %4, %5, %6, and %7"
        return i18n("human-readable-seven-search-engines").arg(enabledSearchEnginesLabels.at(0), enabledSearchEnginesLabels.at(1), enabledSearchEnginesLabels.at(2), enabledSearchEnginesLabels.at(3), enabledSearchEnginesLabels.at(4), enabledSearchEnginesLabels.at(5), enabledSearchEnginesLabels.at(6));
    default:
        return enabledSearchEnginesLabels.join(", ");
    }
}

int SearchEngineList::getSearchEngineCount() const {
    if (empty()) return 0;

    int count = 0;
    for (int i = 0; i < size(); ++i)
        // TODO make it configurable which search engines to start/count
        ++count;
    return count;
}

QHash<int, QByteArray> SearchEngineList::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[LabelRole] = "label";
    roles[EngineEnabledRole] = "engineEnabled";
    return roles;
}

void SearchEngineList::resetProgress() {
    m_collectedProgress.clear();
}

int SearchEngineList::progress() const {
    int count = 0, sum = 0;
    for (QHash<QObject *, int>::ConstIterator it = m_collectedProgress.constBegin(); it != m_collectedProgress.constEnd(); ++it, ++count)
        sum += it.value();
    return count > 0 ? sum / count : 0;
}

void SearchEngineList::collectingProgress(int cur, int total) {
    if (cur > total) cur = total;
    m_collectedProgress.insert(sender(), total > 0 ? cur * 1000 / total : 0);
    emit progressChanged();
}
