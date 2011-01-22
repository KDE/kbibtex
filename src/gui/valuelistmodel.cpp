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

#include <typeinfo>

#include <KComboBox>
#include <KLocale>

#include <QListView>
#include <QGridLayout>
#include <QStringListModel>

#include <bibtexfields.h>
#include <entry.h>
#include "valuelistmodel.h"

static QRegExp ignoredInSorting("[{}\\]+");

ValueListModel::ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent)
        : QAbstractTableModel(parent), file(bibtexFile), fName(fieldName.toLower())
{
    updateValues();
}

int ValueListModel::rowCount(const QModelIndex & parent) const
{
    return parent == QModelIndex() ? sortedValues.count() : 0;
}

int ValueListModel::columnCount(const QModelIndex & parent) const
{
    return parent == QModelIndex() ? 2 : 0;
}

QVariant ValueListModel::data(const QModelIndex & index, int role) const
{
    if (index.row() >= sortedValues.count() || index.column() >= 2)
        return QVariant();
    if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return QVariant(sortedValues[index.row()]);
        else
            return QVariant(valueToCount[sortedValues[index.row()]]);
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0)
            return QVariant(Qt::AlignLeft);
        else
            return QVariant(Qt::AlignRight);
    } else if (role == SortRole) {
        if (index.column() == 0) {
            QString buffer =  sortedValues[index.row()];
            return QVariant(buffer.replace(ignoredInSorting, ""));
        } else
            return QVariant(valueToCount[sortedValues[index.row()]]);
    } else
        return QVariant();
}

QVariant ValueListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section >= 2 || orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    else if (section == 0)
        return QVariant(i18n("Value"));
    else
        return QVariant(i18n("Count"));
}

void ValueListModel::updateValues()
{
    sortedValues.clear();
    valueToCount.clear();

    for (File::ConstIterator fit = file->constBegin(); fit != file->constEnd(); ++fit) {
        const Entry *entry = dynamic_cast<const Entry*>(*fit);
        if (entry != NULL) {
            for (Entry::ConstIterator eit = entry->constBegin(); eit != entry->constEnd(); ++eit) {
                QString key = eit.key().toLower();
                if (key == fName) {
                    insertValue(eit.value());
                    break;
                }
            }
        }
    }
}

void ValueListModel::insertValue(const Value &value)
{
    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        if (typeid(*(*it)) == typeid(Person)) {
            const Person *person = static_cast<const Person*>(*it);
            QString text = person->lastName();
            if (!person->firstName().isEmpty()) text.append(", " + person->firstName());
            // TODO: other parts of name
            if (valueToCount.contains(text))
                valueToCount[text] = valueToCount[text] + 1;
            else {
                valueToCount[text] = 1;
                sortedValues << text;
            }
        } else {
            const QString text = PlainTextValue::text(*(*it), file);
            if (valueToCount.contains(text))
                valueToCount[text] = valueToCount[text] + 1;
            else {
                valueToCount[text] = 1;
                sortedValues << text;
            }
        }
    }
}
