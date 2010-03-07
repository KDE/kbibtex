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
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QMap>

#include <KDebug>

#include <entrylistview.h>
#include <entrylistmodel.h>
#include <fieldlineedit.h>

using namespace KBibTeX::GUI::Widgets;

QMap<QWidget*, QModelIndex> widgetToIndex;

const QLatin1String dummyText = QLatin1String("iyWgYGa967pQq");

class ValueItemDelegate::ValueItemDelegatePrivate
{
private:
    ValueItemDelegate *p;

public:
    QAbstractItemModel *model;
    int uiRecommendedHeight;
    int uiMargin;
    int uiDummyTextWidth;

    ValueItemDelegatePrivate(ValueItemDelegate *parent) {
        p = parent;
    }
};

ValueItemDelegate::ValueItemDelegate(QAbstractItemView *itemView, QObject *parent)
        : KWidgetItemDelegate(itemView, parent), d(new ValueItemDelegatePrivate(this))
{
    d->model = itemView->model();
    FieldLineEdit fle;
    d->uiMargin = fle.fontMetrics().height() / 2;
    d->uiRecommendedHeight = fle.sizeHint().height() + d->uiMargin * 2;
    d->uiDummyTextWidth = fle.fontMetrics().width(dummyText);
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

    QRect textRect(option.rect.left() + d->uiMargin, option.rect.top() + d->uiMargin, d->uiDummyTextWidth, option.fontMetrics.height());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, index.data(EntryListModel::LabelRole).toString());

    painter->restore();
}

QList<QWidget*> ValueItemDelegate::createItemWidgets() const
{
    QList<QWidget*> list;

    QWidget *newWidget = new FieldLineEdit(FieldLineEdit::Source);
    connect(newWidget, SIGNAL(editingFinished()), this, SLOT(slotEditingFinished()));
    list << newWidget;

    // TODO

    return list;
}

void ValueItemDelegate::updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit*>(widgets.at(0));
    QSize size(option.rect.width() - d->uiDummyTextWidth - 3*d->uiMargin, fieldLineEdit->sizeHint().height());
    fieldLineEdit->resize(size);
    fieldLineEdit->move(option.rect.left() + 2*d->uiMargin + d->uiDummyTextWidth, (option.rect.height() - fieldLineEdit->sizeHint().height()) / 2);

    KBibTeX::IO::Value value = index.data(EntryListModel::ValuePointerRole).value<KBibTeX::IO::Value>();
    fieldLineEdit->setValue(value);
    FieldLineEdit::TypeFlags flags = index.data(EntryListModel::TypeFlagsRole).value<FieldLineEdit::TypeFlags>();
    fieldLineEdit->setTypeFlags(flags);

    widgetToIndex.remove(fieldLineEdit);
    widgetToIndex.insert(fieldLineEdit, index);
}

QSize ValueItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    QSize size;
    size.setWidth(d->uiDummyTextWidth * 4);
    size.setHeight(d->uiRecommendedHeight);

    return size;
}


void ValueItemDelegate::slotEditingFinished()
{
    FieldLineEdit *fle = qobject_cast<FieldLineEdit*>(sender());
    if (fle->isModified()) {
        QModelIndex index = widgetToIndex.value(fle);
        KBibTeX::IO::Value value;
        fle->applyTo(value);
        kDebug() << KBibTeX::IO::PlainTextValue::text(value);
        d->model->setData(index, qVariantFromValue(value), EntryListModel::ValuePointerRole);
        emit modified();
    }
}



EntryListView::EntryListView(QWidget* parent)
        : QListView(parent)
{

}
