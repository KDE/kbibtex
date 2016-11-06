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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef KBIBTEX_PROGRAM_STATISTICS_H
#define KBIBTEX_PROGRAM_STATISTICS_H

#include <QWidget>

class QItemSelectionModel;

class FileView;

class OpenFileInfoManager;

class Statistics : public QWidget
{
    Q_OBJECT

public:
    explicit Statistics(OpenFileInfoManager *ofim, QWidget *parent);
    ~Statistics();

    void setFileView(FileView *);

private slots:
    void update();

private:
    class StatisticsPrivate;
    StatisticsPrivate *d;
};

#endif // KBIBTEX_PROGRAM_STATISTICS_H
