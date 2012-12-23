/*****************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#include <QPainter>
#include <QPixmap>

#include <KDebug>
#include <KIconLoader>

#include "guihelper.h"

int GUIHelper::selectValue(QAbstractItemModel *model, const QString &value, int role)
{
    int row = 0;
    QModelIndex index;
    const QString lowerValue = value.toLower();
    while (row < model->rowCount() && (index = model->index(row, 0, QModelIndex())) != QModelIndex()) {
        QString line = model->data(index, role).toString();
        if (line.toLower() == lowerValue)
            return row;
        ++row;
    }

    kDebug() << "Could not find matching row in model for value " << value << " in role" << role;

    return -1;
}

void GUIHelper::paintStars(QPainter *painter, int numActiveStars, int numTotalStars, const QSize &maxSize, const QPoint &pos)
{
    painter->save();

    static const int margin = 2;
    const int maxHeight = qMin(maxSize.height() - 2 * margin, (maxSize.width() - 2 * margin) / numTotalStars);

    QPixmap starPixmap = KIconLoader::global()->loadIcon(QLatin1String("rating"), KIconLoader::Small, maxHeight);

    int x = pos.x() + margin;
    const int y = pos.y() + (maxSize.height() - maxHeight) / 2;
    for (int i = 0; i < numActiveStars; ++i, x += starPixmap.width())
        painter->drawPixmap(x, y, starPixmap);

    starPixmap = KIconLoader::global()->loadIcon(QLatin1String("rating"), KIconLoader::Small, maxHeight, KIconLoader::DisabledState);
    for (int i = 0; i < numTotalStars - numActiveStars; ++i, x += starPixmap.width())
        painter->drawPixmap(x, y, starPixmap);

    painter->restore();
}
