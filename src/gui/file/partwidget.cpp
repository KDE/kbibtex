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

#include "partwidget.h"

#include <QLayout>

#include "filterbar.h"
#include "fileview.h"

class PartWidget::Private
{
private:
    PartWidget *p;

public:
    FileView *fileView;
    FilterBar *filterBar;

    Private(PartWidget *parent)
            : p(parent) {
        QBoxLayout *layout = new QVBoxLayout(parent);
        layout->setMargin(0);

        filterBar = new FilterBar(parent);
        layout->addWidget(filterBar, 0);

        fileView = new FileView(QLatin1String("Main"), parent);
        layout->addWidget(fileView, 0xffffff);
        fileView->setFilterBar(filterBar);
        fileView->setItemDelegate(new FileDelegate(fileView));

        fileView->setFocus();

        connect(fileView, SIGNAL(searchFor(QString)), p, SLOT(searchFor(QString)));
    }
};

PartWidget::PartWidget(QWidget *parent)
        : QWidget(parent), d(new PartWidget::Private(this)) {
    /// nothing
}

PartWidget::~PartWidget()
{
    delete d;
}

FileView *PartWidget::fileView() {
    return d->fileView;
}

FilterBar *PartWidget::filterBar() {
    return d->filterBar;
}

void PartWidget::searchFor(QString text) {
    SortFilterFileModel::FilterQuery fq;
    fq.combination = SortFilterFileModel::EveryTerm;
    fq.field = QString();
    fq.searchPDFfiles = false;
    fq.terms = QStringList() << text;
    d->filterBar->setFilter(fq);
    d->filterBar->setFocus();
}
