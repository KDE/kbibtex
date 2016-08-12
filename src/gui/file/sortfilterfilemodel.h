/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_GUI_SORTFILTERFILEMODEL_H
#define KBIBTEX_GUI_SORTFILTERFILEMODEL_H

#include "kbibtexgui_export.h"

#include <QSortFilterProxyModel>

#include "models/filemodel.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT SortFilterFileModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum FilterCombination {AnyTerm = 0, EveryTerm = 1 };
    struct FilterQuery {
        QStringList terms;
        FilterCombination combination;
        QString field;
        bool searchPDFfiles;
    };

    explicit SortFilterFileModel(QObject *parent = 0);

    virtual void setSourceModel(QAbstractItemModel *model);
    FileModel *fileSourceModel() const;

public slots:
    void updateFilter(const SortFilterFileModel::FilterQuery &);

protected:
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    FileModel *m_internalModel;
    SortFilterFileModel::FilterQuery m_filterQuery;

    KSharedConfigPtr config;
    static const QString configGroupName;
    bool m_showComments, m_showMacros;

    void loadState();
    bool simpleLessThan(const QModelIndex &left, const QModelIndex &right) const;
};

#endif // KBIBTEX_GUI_SORTFILTERFILEMODEL_H
