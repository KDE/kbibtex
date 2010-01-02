/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>

#include <KLocale>

#include "entrylistmodel.h"
#include "bibtexfields.h"
#include "entry.h"
#include "value.h"

using namespace KBibTeX::GUI::Widgets;

class EntryListModel::EntryListModelPrivate
{
public:
    EntryListModel *p;
    QAbstractItemView *view;
    KBibTeX::GUI::Config::BibTeXFields *bibtexFields;
    const KBibTeX::IO::Entry *entry;

    EntryListModelPrivate(const KBibTeX::IO::Entry *e, EntryListModel *parent)
            : p(parent), entry(e) {
        bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();
    }
};

EntryListModel::EntryListModel(const KBibTeX::IO::Entry *entry, QAbstractItemView * parent)
        : QAbstractItemModel(parent), d(new EntryListModelPrivate(entry, this))
{
    d->view = parent;
}

QModelIndex EntryListModel::index(int row, int column, const QModelIndex & /*parent*/) const
{
    return createIndex(row, column, (void*)NULL);
}

QModelIndex EntryListModel::parent(const QModelIndex & /*index*/) const
{
    return QModelIndex();
}

bool EntryListModel::hasChildren(const QModelIndex & parent) const
{
    return parent == QModelIndex();
}

int EntryListModel::rowCount(const QModelIndex & /*parent*/) const
{
    return d->bibtexFields->count(); // FIXME
}

int EntryListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant EntryListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return QVariant(d->bibtexFields->at(index.row()).label);
        } else if (index.column() == 1) {
            KBibTeX::IO::Value value = d->entry->value(d->bibtexFields->at(index.row()).raw);
            QString text = KBibTeX::IO::PlainTextValue::text(value, NULL);
            return QVariant(text);
        }
    } else if (role == Qt::EditRole) {
        if (index.column() == 1) {
            static KBibTeX::IO::Value value; // FIXME: Dirty solution!
            value = d->entry->value(d->bibtexFields->at(index.row()).raw);
            return qVariantFromValue((void*)&value);
        }
    }

    return QVariant();
}

QVariant EntryListModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (section == 0)
        return QVariant(i18n("Key"));
    else if (section == 1)
        return QVariant(i18n("Value"));

    return QVariant();
}

Qt::ItemFlags EntryListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if (index.column() == 1)
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractItemModel::flags(index);
}
