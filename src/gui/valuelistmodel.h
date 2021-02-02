/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_VALUELISTMODEL_H
#define KBIBTEX_GUI_VALUELISTMODEL_H

#include <QAbstractTableModel>
#include <QTreeView>
#include <QStyledItemDelegate>

#include <NotificationHub>
#include <Value>
#include <models/FileModel>

#include "kbibtexgui_export.h"

class KBIBTEXGUI_EXPORT ValueListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ValueListDelegate(QTreeView *parent = nullptr);
    ~ValueListDelegate();

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setFieldName(const QString &fieldName);

private:
    class Private;
    Private *const d;
};

class KBIBTEXGUI_EXPORT ValueListModel : public QAbstractTableModel, private NotificationListener
{
    Q_OBJECT

public:
    enum ValueListModelRole {
        /// How many occurrences a value has
        CountRole = Qt::UserRole + 112,
        /// Role to sort values by
        SortRole = Qt::UserRole + 113,
        /// Role to get text to filter for
        SearchTextRole = Qt::UserRole + 114
    };

    enum class SortBy { Text, Count };

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

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void removeValue(const QModelIndex &index);

    void setShowCountColumn(bool showCountColumn);
    void setSortBy(SortBy sortBy);

    void notificationEvent(int eventId) override;

private:
    void readConfiguration();
    void updateValues();
    void insertValue(const Value &value);
    int indexOf(const QString &text);
    QString htmlize(const QString &text) const;

    bool searchAndReplaceValueInEntries(const QModelIndex &index, const Value &newValue);
    bool searchAndReplaceValueInModel(const QModelIndex &index, const Value &newValue);
    void removeValueFromEntries(const QModelIndex &index);
    void removeValueFromModel(const QModelIndex &index);
};


#endif // KBIBTEX_GUI_VALUELISTMODEL_H
