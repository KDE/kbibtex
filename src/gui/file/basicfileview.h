/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_BASICFILEVIEW_H
#define KBIBTEX_GUI_BASICFILEVIEW_H

#include <QTreeView>

#include "kbibtexgui_export.h"

class FileModel;
class QSortFilterProxyModel;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT BasicFileView : public QTreeView
{
    Q_OBJECT
public:
    explicit BasicFileView(const QString &name, QWidget *parent = nullptr);
    ~BasicFileView() override;

    void setModel(QAbstractItemModel *model) override;
    FileModel *fileModel();
    QSortFilterProxyModel *sortFilterProxyModel();

signals:
    void searchFor(const QString &);
    /**
     * Signal emitted if this view's selection changes.
     * @param hasSelection true if at least one row is selected, false otherwise
     */
    void hasSelectionChanged(bool hasSelection);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    class Private;
    Private *d;

private slots:
    void headerColumnVisibilityToggled();
    void sort(int, Qt::SortOrder);
    void noSorting();
    void showHeaderContextMenu(const QPoint &pos);
};


#endif // KBIBTEX_GUI_BASICFILEVIEW_H
