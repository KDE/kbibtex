/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2017 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_FINDPDFUI_P_H
#define KBIBTEX_GUI_FINDPDFUI_P_H

#include <KWidgetItemDelegate>

#include <FindPDF>

class QListView;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class PDFItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

private:
    QListView *m_parent;

public:
    PDFItemDelegate(QListView *itemView, QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /// update the widgets
    void updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const override;

protected:
    /// get the list of widgets
    QList<QWidget *> createItemWidgets(const QModelIndex &index) const override;

private slots:
    void slotViewPDF();
    void slotRadioNoDownloadToggled(bool);
    void slotRadioDownloadToggled(bool);
    void slotRadioURLonlyToggled(bool);
};


/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class PDFListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PDFListModel(QList<FindPDF::ResultItem> &resultList, QObject *parent = nullptr);

    enum PDFListModelRole {
        /// URL of a PDF file
        URLRole = Qt::UserRole + 1234,
        /// A textual representation, e.g. text extracted from the PDF file
        TextualPreviewRole = Qt::UserRole + 1237,
        /// Local, temporal copy of a remote PDF file
        TempFileNameRole = Qt::UserRole + 1236,
        /// Returns a FindPDF::DownloadMode, may be either
        /// 'NoDownload', 'Download', or 'URLonly'
        DownloadModeRole = Qt::UserRole + 1235
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QList<FindPDF::ResultItem> &m_resultList;
};


#endif // KBIBTEX_GUI_FINDPDFUI_P_H
