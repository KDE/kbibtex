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
#include "fieldlineedit.h"
#include "bibtexfields.h"
#include "entry.h"
#include "value.h"

using namespace KBibTeX::GUI::Widgets;

const int specialFieldOffset = 1;

EntryListModel::EntryListModel(QObject * parent)
        : QAbstractListModel(parent)
{
    // TODO
}

int EntryListModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return m_entry.count();
}

QVariant EntryListModel::data(const QModelIndex & index, int role) const
{
    QStringList keys = m_entry.keys();
    Q_ASSERT(index.row() >= 0 && index.row() < keys.size() + specialFieldOffset);

    QString key;
    switch (index.row()) {
    case 0: key = QLatin1String("Id"); break;
    default: key = keys[index.row()-specialFieldOffset];
    }

    switch (role) {
    case LabelRole:
        return KBibTeX::GUI::Config::BibTeXFields::self()->format(key, KBibTeX::GUI::Config::BibTeXFields::cCamelCase);
    case ValuePointerRole: {
        switch (index.row()) {
        case 0: return qVariantFromValue(m_entry.id());
        default: {
            Q_ASSERT(index.row() >= specialFieldOffset);
            KBibTeX::IO::Value value = m_entry.value(key);
            return qVariantFromValue(value);
        }
        }
    }
    case TypeFlagsRole: {
        FieldLineEdit::TypeFlags   flags = FieldLineEdit::Source;
        if (key.toLower() == "title") flags |= FieldLineEdit::Text;
        return qVariantFromValue(flags);
    }
    }

    return QVariant();
}

bool EntryListModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    QStringList keys = m_entry.keys();
    Q_ASSERT(index.row() >= 0 && index.row() < keys.size() + specialFieldOffset);
    bool result = false;

    switch (role) {
    case ValuePointerRole: {
        switch (index.row()) {
        case 0:
            m_entry.setId(qVariantValue<QString>(value));
            break;
        default: {
            Q_ASSERT(index.row() >= specialFieldOffset);
            QString key = keys[index.row() - specialFieldOffset];
            KBibTeX::IO::Value kbibtexValue = qVariantValue<KBibTeX::IO::Value>(value);
            m_entry[key] = kbibtexValue;
        }
        }
        result = true;
        break;
    }
    default:
        result = false;
    }

    if (result) emit dataChanged(index, index);
    return result;
}

void EntryListModel::setEntry(const KBibTeX::IO::Entry& entry)
{
    beginResetModel();
    m_entry = KBibTeX::IO::Entry(entry);
    endResetModel();
}

void EntryListModel::applyToEntry(KBibTeX::IO::Entry& entry)
{
    entry = m_entry;
}
