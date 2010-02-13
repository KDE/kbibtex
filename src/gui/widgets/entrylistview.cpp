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

#include <QApplication>
#include <QPainter>
#include <QPen>

#include <entrylistview.h>
#include <entrylistmodel.h>
#include <fieldlineedit.h>

using namespace KBibTeX::GUI::Widgets;

ValueItemDelegate::ValueItemDelegate(QAbstractItemView *itemView, QObject *parent)
        : KWidgetItemDelegate(itemView, parent)
{
    // TODO
}

void ValueItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlightedText().color()));
    } else {
        painter->setPen(QPen(option.palette.text().color()));
    }
    // TODO

    QRect textRect = option.rect;
    textRect.setTop(textRect.top() + 4);
    textRect.setLeft(textRect.left() + 4);
    painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignTop, index.data(EntryListModel::LabelRole).toString());

    painter->restore();
}

QList<QWidget*> ValueItemDelegate::createItemWidgets() const
{
    QList<QWidget*> list;

    list << new FieldLineEdit(FieldLineEdit::Source);
    // TODO

    return list;
}

void ValueItemDelegate::updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    Q_UNUSED(index);

    int right = option.rect.width();
    int margin = option.fontMetrics.height() / 2;
    // TODO

    FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit*>(widgets.at(0));
    QSize size(option.rect.width() - option.fontMetrics.width('A') * 10, fieldLineEdit->sizeHint().height());
    fieldLineEdit->resize(size);
    fieldLineEdit->move(right - fieldLineEdit->width() - margin, margin);

    KBibTeX::IO::Value value = index.data(EntryListModel::ValuePointerRole).value<KBibTeX::IO::Value>();
    fieldLineEdit->setValue(value);
}

QSize ValueItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    QSize size;

    size.setWidth(option.fontMetrics.width('A') * 25);
    size.setHeight(option.fontMetrics.height() * 2); // up to 6 lines of text, and two margins

    // TODO

    return size;
}




EntryListView::EntryListView(QWidget* parent)
        : QListView(parent)
{

}
