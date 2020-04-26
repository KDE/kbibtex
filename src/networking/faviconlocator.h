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

#ifndef KBIBTEX_NETWORKING_FAVICONLOCATOR_H
#define KBIBTEX_NETWORKING_FAVICONLOCATOR_H

#include <QObject>
#include <QIcon>
#include <QUrl>

/**
 * This class tries to locate and download the favicon for a given
 * webpage. Upon success, a signal will be triggered.
 * At any point in time, the icon can be queried using a function,
 * but as long as no specific favicon got retrieved, this function
 * will return a generic icon only.
 * Favicons located once will be cached on a per-application base.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class FavIconLocator : public QObject
{
    Q_OBJECT
public:
    /**
     * Start searching for a favicon belonging to a given webpage.
     * Search will run asynchronous, its end is signalled with
     * @see gotIcon().
     * @param webpageUrl Webpage where to search for a favicon
     * @param parent QObject-based parent of this object
     */
    explicit FavIconLocator(const QUrl &webpageUrl, QObject *parent);

    /**
     * Icon know for the webpage this specific object was created
     * for. In case no favicon has been downloaded yet (still in
     * progress or download failed), then a generic icon will be
     * returned.
     *
     * @return Either the webpage's favicon or a generic icon
     */
    QIcon icon() const;

signals:
    /**
     * Asynchronous search for a favicon concluded either with the
     * retrieved favicon or, in case of any failure, a generic icon.
     */
    void gotIcon(QIcon);

private:
    QIcon favIcon;
};

#endif // KBIBTEX_NETWORKING_FAVICONLOCATOR_H
