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
#include <QTreeView>
#include <QStyledItemDelegate>

#include "bibtexfilemodel.h"

static const int SortRole = Qt::UserRole + 113;
static const int SearchTextRole = Qt::UserRole + 114;

class KBIBTEXGUI_EXPORT ValueListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

private:
    QString m_fieldName;
    QTreeView *m_parent;

public:
    ValueListDelegate(QTreeView *parent = NULL)
            : QStyledItemDelegate(parent), m_fieldName(QString::null), m_parent(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setFieldName(const QString &fieldName) {
        m_fieldName = fieldName;
    }

private slots:
    void commitAndCloseEditor();
};

class KBIBTEXGUI_EXPORT ValueListModel : public QAbstractTableModel
{
public:
    enum SortBy { SortByText, SortByCount };

private:
    struct ValueLine {
        QString text;
        QString sortBy;
        Value value;
        int count;
    };

    typedef QVector<ValueLine> ValueLineList;

    const File *file;
    const QString fName;
    ValueLineList values;
    QMap<QString, QString> colorToLabel;
    bool showCountColumn;
    SortBy sortBy;

public:
    ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent);

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void setShowCountColumn(bool showCountColumn);
    void setSortBy(SortBy sortBy);

private:
    void updateValues();
    void insertValue(const Value &value);
    int indexOf(const QString &text);
    QString htmlize(const QString &text) const;
};


#endif // KBIBTEX_PROGRAM_VALUELISTMODEL_H
