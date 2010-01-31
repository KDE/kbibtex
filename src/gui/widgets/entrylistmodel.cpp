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
    QString key = m_keys[index.row()];
    switch (role) {
    case LabelRole:
        return key;
    case SourceRole: {
        KBibTeX::IO::Value value = m_entry.value(key);
        return KBibTeX::IO::PlainTextValue::text(value);
    }
    }

    return QVariant();
}

void EntryListModel::setEntry(const KBibTeX::IO::Entry& entry)
{
    m_entry = KBibTeX::IO::Entry(entry);
    m_keys = QStringList(m_entry.keys());
}

KBibTeX::IO::Value EntryListModel::valueForIndex(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return KBibTeX::IO::Value(); // FIXME
}

