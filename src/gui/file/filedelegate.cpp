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

#include "filedelegate.h"

#include <KIconLoader>
#include <KRatingPainter>

#include <BibTeXFields>
#include <Entry>
#include <models/FileModel>
#include "widgets/starrating.h"

void FileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    static const int numTotalStars = 8;
    bool ok = false;
    double percent = index.data(FileModel::NumberRole).toDouble(&ok);
    if (ok) {
        const FieldDescription &fd = BibTeXFields::instance().at(index.column());
        if (fd.upperCamelCase.toLower() == Entry::ftStarRating) {
            static KRatingPainter ratingPainter;
            ratingPainter.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            ratingPainter.setHalfStepsEnabled(false);
            ratingPainter.setMaxRating(numTotalStars);
            ratingPainter.setLayoutDirection(qobject_cast<QWidget *>(parent())->layoutDirection());
            ratingPainter.paint(painter, option.rect, static_cast<int>(percent / 100.0 * numTotalStars));
        }
    }
}
