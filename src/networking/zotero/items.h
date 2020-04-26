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

#ifndef KBIBTEX_NETWORKING_ZOTERO_ITEMS_H
#define KBIBTEX_NETWORKING_ZOTERO_ITEMS_H

#include <QObject>
#include <QSharedPointer>
#include <QUrl>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

class Element;

namespace Zotero
{

class API;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT Items : public QObject
{
    Q_OBJECT
public:
    explicit Items(QSharedPointer<Zotero::API> api, QObject *parent = nullptr);
    ~Items() override;

    void retrieveItemsByCollection(const QString &collectionId);
    void retrieveItemsByTag(const QString &tag);

signals:
    void foundElement(QSharedPointer<Element>);
    void stoppedSearch(int);

private:
    class Private;
    Private *const d;

private slots:
    void finishedFetchingItems();
};

} // end of namespace Zotero

#endif // KBIBTEX_NETWORKING_ZOTERO_ITEMS_H
