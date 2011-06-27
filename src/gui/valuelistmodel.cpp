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
#include <KDebug>

#include <QListView>
#include <QGridLayout>
#include <QStringListModel>
#include <QLineEdit>

#include <fieldlineedit.h>
#include <bibtexfields.h>
#include <entry.h>
#include "valuelistmodel.h"

QWidget *ValueListDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &sovi, const QModelIndex &index) const
{
    if (index.column() == 0) {
        const FieldDescription &fd = BibTeXFields::self()->find(m_fieldName);
        FieldLineEdit *fieldLineEdit = new FieldLineEdit(fd.preferredTypeFlag, fd.typeFlags, false, parent);
        return fieldLineEdit;
    } else
        return QStyledItemDelegate::createEditor(parent, sovi, index);
}

void ValueListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 0) {
        FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit*>(editor);
        if (fieldLineEdit != NULL)
            fieldLineEdit->reset(index.model()->data(index, Qt::EditRole).value<Value>());
    }
}

void ValueListDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit*>(editor);
    if (fieldLineEdit != NULL) {
        Value v;
        fieldLineEdit->apply(v);
        if (v.count() == 1) /// field should contain exactly one value item (no zero, not two or more)
            model->setData(index, QVariant::fromValue(v));
    }
}

QSize ValueListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), option.fontMetrics.height()*3 / 2)); // TODO calculate height better
    return size;
}

void ValueListDelegate::commitAndCloseEditor()
{
    QLineEdit *editor = qobject_cast<QLineEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

static QRegExp ignoredInSorting("[{}\\]+");

ValueListModel::ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent)
        : QAbstractTableModel(parent), file(bibtexFile), fName(fieldName.toLower())
{
    updateValues();
}

int ValueListModel::rowCount(const QModelIndex & parent) const
{
    return parent == QModelIndex() ? values.count() : 0;
}

int ValueListModel::columnCount(const QModelIndex & parent) const
{
    return parent == QModelIndex() ? 2 : 0;
}

QVariant ValueListModel::data(const QModelIndex & index, int role) const
{
    if (index.row() >= values.count() || index.column() >= 2)
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (index.column() == 0)
            return QVariant(values[index.row()].text);
        else
            return QVariant(values[index.row()].count);
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0)
            return QVariant(Qt::AlignLeft);
        else
            return QVariant(Qt::AlignRight);
    } else if (role == SortRole) {
        if (index.column() == 0) {
            QString buffer = values[index.row()].sortBy.isNull() ? values[index.row()].text : values[index.row()].sortBy;
            return QVariant(buffer.replace(ignoredInSorting, ""));
        } else
            return QVariant(values[index.row()].count);
    } else if (role == SearchTextRole)
        return QVariant(values[index.row()].text);
    else if (role == Qt::EditRole)
        return QVariant::fromValue(values[index.row()].value);
    else
        return QVariant();
}

bool ValueListModel::setData(const QModelIndex & index, const QVariant &value, int role)
{
    if (role == Qt::EditRole && index.column() == 0) {
        Value newValue = value.value<Value>(); /// nice names ... ;-)
        const QString origText = data(index, Qt::DisplayRole).toString();
        const QString newText = PlainTextValue::text(newValue);
        kDebug() << "replacing" << origText << "with" << newText << "for field" << fName;

        foreach(Element *element, *file) {
            Entry *entry = dynamic_cast< Entry*>(element);
            if (entry != NULL) {
                for (Entry::Iterator eit = entry->begin(); eit != entry->end(); ++eit) {
                    QString key = eit.key().toLower();
                    if (key == fName) {
                        const QString valueFullText = PlainTextValue::text(eit.value());
                        if (valueFullText == origText)
                            entry->insert(key, newValue);
                        else {
                            for (Value::Iterator vit = eit.value().begin(); vit != eit.value().end(); ++vit) {
                                const QString valueItemText = PlainTextValue::text(*(*vit));
                                if (valueItemText == origText) {
                                    vit = eit.value().erase(vit);
                                    vit = eit.value().insert(vit, newValue.first());
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        int index = indexOf(origText);
        values[index].text = newText;
        values[index].value = newValue;
        reset();

        return true;
    }
    return false;
}

Qt::ItemFlags ValueListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    /// make first column editable
    if (index.column() == 0)
        result |= Qt::ItemIsEditable;
    return result;
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
    values.clear();

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
    foreach(ValueItem *item, value) {
        const QString text = PlainTextValue::text(*item, file);
        int index = indexOf(text);

        if (index < 0) {
            /// previously unknown text
            ValueLine newValueLine;
            newValueLine.text = text;
            newValueLine.count = 1;
            Value v;
            v.append(item);
            newValueLine.value = v;

            /// memorize sorting criterium for persons, which is last name first
            const Person *person = dynamic_cast<const Person*>(item);
            newValueLine.sortBy = person == NULL ? QString::null : person->lastName() + QLatin1String(" ") + person->firstName();

            values << newValueLine;
        } else {
            ++values[index].count;
        }
    }
}

int ValueListModel::indexOf(const QString &text)
{
    int i = 0;
    /// this is really slow for large data sets: O(n^2)
    /// maybe use a hash table instead?
    foreach(const ValueLine &valueLine, values) {
        if (valueLine.text == text)
            return i;
        ++i;
    }
    return -1;
}
