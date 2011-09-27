/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QGridLayout>
#include <QListView>
#include <QAbstractListModel>
#include <QUrl>
#include <QPainter>
#include <QToolButton>
#include <QApplication>

#include <KDialog>
#include <KLocale>
#include <KDebug>
#include <KWidgetItemDelegate>
#include <KSqueezedTextLabel>

#include "findpdfui.h"

class PDFListModel;

const int posLabel = 0;
const int posToolButton = 1;

class PDFItemDelegate : public KWidgetItemDelegate
{
private:
    QListView *m_parent;

public:
    /// inspired by KNewStuff3's ItemsViewDelegate
    PDFItemDelegate(QListView *itemView, QObject *parent)
            : KWidgetItemDelegate(itemView, parent), m_parent(itemView) {
        // TODO
    }

    /// paint the item at index with all its attributes shown
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
        /* int margin = option.fontMetrics.height() / 2;

           QStyle *style = QApplication::style();
           style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

           painter->save();

           if (option.state & QStyle::State_Selected) {
               painter->setPen(QPen(option.palette.highlightedText().color()));
           } else {
               painter->setPen(QPen(option.palette.text().color()));
           }

           const PDFListModel *model = qobject_cast<const PDFListModel*>(index.model());

           painter->restore();
        */
        painter->save();

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, (index.row() % 2 == 0 ? option.palette.base() : option.palette.alternateBase()));
            painter->setPen(QPen(option.palette.window().color()));
            painter->drawRect(option.rect);
        }

        if (option.state & QStyle::State_Selected) {
            painter->setPen(QPen(option.palette.highlightedText().color()));
        } else {
            painter->setPen(QPen(option.palette.text().color()));
        }

        painter->restore();
    }

    /// get the list of widgets
    virtual QList<QWidget*> createItemWidgets() const {
        QList<QWidget*> list;

        KSqueezedTextLabel *label = new KSqueezedTextLabel();
        label->setBackgroundRole(QPalette::NoRole);
        list << label;

        /*
        Q_ASSERT(list.count() == posLabel + 1);

              QToolButton *button = new QToolButton();
              list << button;
              Q_ASSERT(list.count() == posToolButton + 1);

              button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
              button->setPopupMode(QToolButton::InstantPopup);
              list << button;
              // TODO setBlockedEventTypes(button, QList<QEvent::Type>() << QEvent::MouseButtonPress << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick);
              // TODO connect(installButton, SIGNAL(clicked()), this, SLOT(slotInstallClicked()));
              // TODO connect(installButton, SIGNAL(triggered(QAction *)), this, SLOT(slotInstallActionTriggered(QAction *)));
        */
        return list;
    }

    /// update the widgets
    virtual void updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const {
        if (!index.isValid()) return;

        const PDFListModel *model = qobject_cast<const PDFListModel*>(index.model());
        if (model == NULL) {
            kDebug() << "WARNING - INVALID MODEL!";
            return;
        }

        /// setup label
        KSqueezedTextLabel *label = qobject_cast<KSqueezedTextLabel*>(widgets[0]);
        if (label != NULL) {
            label->setText(QLatin1String("Label"));
        }
        /*

        /// setup the install button
        QToolButton *installButton = qobject_cast<QToolButton*>(widgets.at(posToolButton));
        if (installButton != NULL) {
            int margin = option.fontMetrics.height() / 2;
            int right = option.rect.width();
            QString text(i18n("Download"));
            KIcon icon("download");
            installButton->setText(text);
            installButton->setIcon(icon);
        }
        */

        // TODO
    }

    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
        /*   QSize size;
           size.setWidth(option.fontMetrics.height() * 4);
           size.setHeight(option.fontMetrics.height() * 7);
           return size;
           */
        Q_UNUSED(option);
        Q_UNUSED(index);

        return QSize(410, 210);
    }

};

/*
class PDFStyledItemDelegate : public QStyledItemDelegate
{
private:
    QListView *m_parent;

public:
    PDFStyledItemDelegate(QListView *parent = NULL)
            : QStyledItemDelegate(parent), m_parent(parent) {
        // TODO
    }

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), option.fontMetrics.height() * 3)); // TODO calculate height better
        return size;
    }

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const {
        QStyleOptionViewItemV4 *noTextOption = qstyleoption_cast<QStyleOptionViewItemV4 *>(option);
        QStyledItemDelegate::initStyleOption(noTextOption, index);
        if (option->decorationPosition != QStyleOptionViewItem::Top) {
            /// remove text from style (do not draw text)
            noTextOption->text.clear();
        }

    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        /// save painter's state, restored before leaving this function
        painter->save();

        /// first, paint the basic, but without the text. We remove the text
        /// in initStyleOption(), which gets called by QStyledItemDelegate::paint().
        QStyledItemDelegate::paint(painter, option, index);

        /// now, we retrieve the correct style option by calling intiStyleOption from
        /// the superclass.
        QStyleOptionViewItemV4 option4 = option;
        QStyledItemDelegate::initStyleOption(&option4, index);
        QString urlText = option4.text;

        /// now calculate the rectangle for the text
        QStyle *s = m_parent->style();
        const QWidget *widget = option4.widget;
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &option4, widget);

        if (option.state & QStyle::State_Selected) {
            /// selected lines are drawn with different color
            painter->setPen(option.palette.highlightedText().color());
        }

        /// squeeze the folder text if it is to big and calculate the rectangles
        /// where the folder text and the unread count will be drawn to
        QFontMetrics fm(painter->fontMetrics());
        int urlTextWidth = fm.width(urlText);
        if (urlTextWidth > textRect.width()) {
            /// text plus count is too wide for column, cut text and insert "..."
            urlText = fm.elidedText(urlText, Qt::ElideRight, textRect.width());
            urlTextWidth = fm.width(urlText);
        }

        /// determine rects to draw field
        int top = textRect.top() + (textRect.height() - fm.height()) / 2;
        QRect  urlTextRect = textRect;
        urlTextRect.setTop(top);
        urlTextRect.setHeight(fm.height());

        /// left-align text
        urlTextRect.setLeft(urlTextRect.left() + 4); ///< hm, indent necessary?
        urlTextRect.setRight(urlTextRect.left() + urlTextWidth);

        /// draw field name
        painter->drawText(urlTextRect, Qt::AlignLeft, urlText);

        /// restore painter's state
        painter->restore();
    }

};
*/


PDFListModel::PDFListModel(const QList<FindPDF::ResultItem> &resultList, QObject *parent)
        : QAbstractListModel(parent), m_resultList(resultList)
{
    // nothing
}

int PDFListModel::rowCount(const QModelIndex & parent) const
{
    int count = parent == QModelIndex() ? /*m_resultList.count()*/ 1 : 0;
    return count;
}

QVariant PDFListModel::data(const QModelIndex &index, int role) const
{
    if (index != QModelIndex() && index.parent() == QModelIndex() && index.row() < m_resultList.count()) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            //return m_resultList[index.row()].url.toString();
            return QLatin1String("aaaa");
        } else
            return QVariant();
    }
    return QVariant();
}

QVariant PDFListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if (section == 0) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            return i18n("Result");
        } else
            return QVariant();
    }
    return QVariant();
}


FindPDFUI::FindPDFUI(Entry &entry, QWidget *parent)
        : QWidget(parent), m_findpdf(new FindPDF(this))
{
    createGUI();

    connect(m_findpdf, SIGNAL(finished()), this, SLOT(searchFinished()));
    m_findpdf->search(entry);
}

void FindPDFUI::interactiveFindPDF(Entry &entry, QWidget *parent)
{
    KDialog dlg(parent);
    FindPDFUI widget(entry, &dlg);
    dlg.setMainWidget(&widget);
    dlg.enableButtonOk(false);

    connect(&widget, SIGNAL(resultAvailable(bool)), &dlg, SLOT(enableButtonOk(bool)));

    if (dlg.exec() == KDialog::Accepted) {
        // TODO
    }
}

void FindPDFUI::apply()
{
    // TODO
}

void FindPDFUI::createGUI()
{
    QGridLayout *layout = new QGridLayout(this);

    m_listViewResult = new QListView(this);
    layout->addWidget(m_listViewResult, 0, 0, 1, 1);
    m_listViewResult->setEnabled(false);
    m_listViewResult->setItemDelegate(new PDFItemDelegate(m_listViewResult, this));

    static_cast<QWidget*>(parent())->setCursor(Qt::WaitCursor);
}

void FindPDFUI::searchFinished()
{
    m_listViewResult->setModel(new PDFListModel(m_findpdf->results(), m_listViewResult));
    m_listViewResult->setEnabled(true);

    static_cast<QWidget*>(parent())->unsetCursor();
    emit resultAvailable(true);
}
