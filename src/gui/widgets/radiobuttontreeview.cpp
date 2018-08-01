/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "radiobuttontreeview.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

RadioButtonItemDelegate::RadioButtonItemDelegate(QObject *p)
        : QStyledItemDelegate(p)
{
    /// nothing
}

void RadioButtonItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data(RadioButtonTreeView::IsRadioRole).canConvert<bool>() && index.data(RadioButtonTreeView::IsRadioRole).toBool()) {
        /// determine size and spacing of radio buttons in current style
        const int radioButtonWidth = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &option);
        const int radioButtonHeight = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
        const int spacing = QApplication::style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, &option);

        /// draw default appearance (text, highlighting) shifted to the left
        QStyleOptionViewItem myOption = option;
        const int left = myOption.rect.left();
        myOption.rect.setLeft(left + spacing * 3 / 2 + radioButtonWidth);
        QStyledItemDelegate::paint(painter, myOption, index);

        /// draw radio button in the open space
        myOption.rect.setLeft(left + spacing / 2);
        myOption.rect.setWidth(radioButtonWidth);
        myOption.rect.setTop(option.rect.top() + (option.rect.height() - radioButtonHeight) / 2);
        myOption.rect.setHeight(radioButtonHeight);
        if (index.data(RadioButtonTreeView::RadioSelectedRole).canConvert<bool>()) {
            /// change radio button's visual appearance if selected or not
            bool radioButtonSelected = index.data(RadioButtonTreeView::RadioSelectedRole).toBool();
            myOption.state |= radioButtonSelected ? QStyle::State_On : QStyle::State_Off;
        }
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &myOption, painter);
    } else
        QStyledItemDelegate::paint(painter, option, index);
}

QSize RadioButtonItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    if (index.data(RadioButtonTreeView::IsRadioRole).toBool()) {
        /// determine size of radio buttons in current style
        int radioButtonHeight = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
        /// ensure that line is tall enough to draw radio button
        s.setHeight(qMax(s.height(), radioButtonHeight));
    }
    return s;
}



RadioButtonTreeView::RadioButtonTreeView(QWidget *parent)
        : QTreeView(parent)
{
    setItemDelegate(new RadioButtonItemDelegate(this));
}

void RadioButtonTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.data(IsRadioRole).toBool()) {
        /// clicking on an alternative's item in tree view should select alternative
        switchRadioFlag(index);
        event->accept();
    } else
        QTreeView::mouseReleaseEvent(event);
}

void RadioButtonTreeView::keyReleaseEvent(QKeyEvent *event)
{
    QModelIndex index = currentIndex();
    if (index.data(IsRadioRole).toBool() && event->key() == Qt::Key_Space) {
        /// pressing space on an alternative's item in tree view should select alternative
        switchRadioFlag(index);
        event->accept();
    } else
        QTreeView::keyReleaseEvent(event);
}

void RadioButtonTreeView::switchRadioFlag(QModelIndex &index)
{
    const int maxRow = 1024;
    const int col = index.column();
    for (int row = 0; row < maxRow; ++row) {
        const QModelIndex &sib = index.sibling(row, col);
        model()->setData(sib, QVariant::fromValue(sib == index), RadioSelectedRole);
    }
}
