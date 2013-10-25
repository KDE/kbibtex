/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#ifndef KBIBTEX_GUI_BIBTEXFILEMODEL_H
#define KBIBTEX_GUI_BIBTEXFILEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QLatin1String>
#include <QList>
#include <QRegExp>
#include <QStringList>

#include <KSharedConfig>

#include "kbibtexgui_export.h"

#include "file.h"
#include "entry.h"

class BibTeXFileModel;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT SortFilterBibTeXFileModel : public QSortFilterProxyModel
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

    SortFilterBibTeXFileModel(QObject *parent = 0);

    virtual void setSourceModel(QAbstractItemModel *model);
    BibTeXFileModel *bibTeXSourceModel() const;

public slots:
    void updateFilter(SortFilterBibTeXFileModel::FilterQuery);

protected:
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    BibTeXFileModel *m_internalModel;
    SortFilterBibTeXFileModel::FilterQuery m_filterQuery;

    KSharedConfigPtr config;
    static const QString configGroupName;
    bool m_showComments, m_showMacros;

    void loadState();
    bool simpleLessThan(const QModelIndex &left, const QModelIndex &right) const;
};


/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXFileModel : public QAbstractTableModel
{
public:
    static const int SortRole;
    static const QString keyShowComments;
    static const bool defaultShowComments;
    static const QString keyShowMacros;
    static const bool defaultShowMacros;

    BibTeXFileModel(QObject *parent = 0);

    File *bibTeXFile();
    virtual void setBibTeXFile(File *bibtexFile);

    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    virtual bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    bool removeRowList(const QList<int> &rows);
    bool insertRow(QSharedPointer<Element> element, int row, const QModelIndex &parent = QModelIndex());

    QSharedPointer<Element> element(int row) const;
    int row(QSharedPointer<Element> element) const;

    void reset();

    QVariant entryData(const Entry *entry, const QString &raw, const QString &rawAlt, int role, bool followCrossRef) const;

private:
    File *m_bibtexFile;
    QMap<QString, QString> colorToLabel;
};


#endif // KBIBTEX_GUI_BIBTEXFILEMODEL_H
