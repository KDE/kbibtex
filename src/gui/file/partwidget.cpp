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

#include "partwidget.h"

#include <QLayout>

#include "widgets/filterbar.h"
#include "fileview.h"
#include "filedelegate.h"

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

        fileView = new FileView(QStringLiteral("Main"), parent);
        layout->addWidget(fileView, 0xffffff);
        fileView->setFilterBar(filterBar);
        fileView->setItemDelegate(new FileDelegate(fileView));

        fileView->setFocus();

        connect(fileView, &FileView::searchFor, p, &PartWidget::searchFor);
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

void PartWidget::searchFor(const QString &text) {
    SortFilterFileModel::FilterQuery fq;
    fq.combination = SortFilterFileModel::FilterCombination::EveryTerm;
    fq.field = QString();
    fq.searchPDFfiles = false;
    fq.terms = QStringList() << text;
    d->filterBar->setFilter(fq);
    d->filterBar->setFocus();
}
