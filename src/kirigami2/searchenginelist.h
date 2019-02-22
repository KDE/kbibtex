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

#ifndef SEARCHENGINELIST_H
#define SEARCHENGINELIST_H

#include <QAbstractListModel>
#include <QHash>

#include "entry.h"

class OnlineSearchAbstract;

class SearchEngineList : public QAbstractListModel, public QVector<OnlineSearchAbstract *>
{
    Q_OBJECT
    Q_PROPERTY(int searchEngineCount READ getSearchEngineCount NOTIFY searchEngineCountChanged)

public:
    enum Roles {LabelRole = Qt::DisplayRole, EngineEnabledRole = Qt::UserRole + 1000};

    explicit SearchEngineList();
    explicit SearchEngineList(const SearchEngineList &);

    SearchEngineList *operator =(const SearchEngineList *);

    Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE virtual bool setData(const QModelIndex &index, const QVariant &value, int role = EngineEnabledRole) override;

    Q_INVOKABLE QString humanReadableSearchEngines() const;
    int getSearchEngineCount() const;

    QHash<int, QByteArray> roleNames() const override;

    void resetProgress();
    int progress() const;

signals:
    void foundEntry(QSharedPointer<Entry>);
    void busyChanged();
    void progressChanged();
    void searchEngineCountChanged();

private slots:
    void collectingProgress(int, int);

private:
    QHash<QObject *, int> m_collectedProgress;
};

Q_DECLARE_METATYPE(SearchEngineList)

#endif // SEARCHENGINELIST_H
