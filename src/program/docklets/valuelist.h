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

#ifndef KBIBTEX_PROGRAM_DOCKLET_VALUELIST_H
#define KBIBTEX_PROGRAM_DOCKLET_VALUELIST_H

#include <QWidget>
#include <QModelIndex>

class Element;
class File;
class FileView;

class ValueList : public QWidget
{
    Q_OBJECT

public:
    explicit ValueList(QWidget *parent);
    ~ValueList() override;

    void setFileView(FileView *fileView);

public slots:
    void update();

protected slots:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void listItemActivated(const QModelIndex &);
    void searchSelection();
    void assignSelection();
    void removeSelection();
    void startItemRenaming();
    void deleteAllOccurrences();
    void showCountColumnToggled();
    void sortByCountToggled();
    void columnsChanged();
    void editorSelectionChanged();
    void editorDestroyed();
    void fieldNamesChanged(int);

private:
    class ValueListPrivate;
    ValueListPrivate *d;
};

#endif // KBIBTEX_PROGRAM_DOCKLET_VALUELIST_H
