/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#ifndef KBIBTEX_PROGRAM_VALUELISTMODEL_H
#define KBIBTEX_PROGRAM_VALUELISTMODEL_H

#include <QAbstractTableModel>

#include <bibtexfilemodel.h>

class ValueListModel : public QAbstractTableModel
{
private:
    const File *file;
    const QString fName;
    QStringList sortedValues;
    QMap<QString, int> valueToCount;

public:
    ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    void updateValues();
    void insertValue(const Value &value);
};


#endif // KBIBTEX_PROGRAM_VALUELISTMODEL_H
