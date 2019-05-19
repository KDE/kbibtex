/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_GUI_SORTFILTERFILEMODEL_H
#define KBIBTEX_GUI_SORTFILTERFILEMODEL_H

#include <QSortFilterProxyModel>

#include <models/FileModel>

#include "kbibtexgui_export.h"

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

    explicit SortFilterFileModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *model) override;
    FileModel *fileSourceModel() const;

public slots:
    void updateFilter(const SortFilterFileModel::FilterQuery &);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    FileModel *m_internalModel;
    SortFilterFileModel::FilterQuery m_filterQuery;

    bool simpleLessThan(const QModelIndex &left, const QModelIndex &right) const;
};

#endif // KBIBTEX_GUI_SORTFILTERFILEMODEL_H
