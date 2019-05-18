/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_NETWORKING_ZOTERO_COLLECTION_H
#define KBIBTEX_NETWORKING_ZOTERO_COLLECTION_H

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QUrl>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

namespace Zotero
{

class API;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT Collection : public QObject
{
    Q_OBJECT
public:
    Collection(QSharedPointer<Zotero::API> api, QObject *parent);
    ~Collection() override;

    bool initialized() const;
    bool busy() const;

    QString collectionLabel(const QString &collectionId) const;
    QString collectionParent(const QString &collectionId) const;
    QVector<QString> collectionChildren(const QString &collectionId) const;
    uint collectionNumericId(const QString &collectionId) const;
    QString collectionFromNumericId(uint numericId) const;

signals:
    void finishedLoading();

private:
    class Private;
    Private *const d;

    void emitFinishedLoading();

private slots:
    void finishedFetchingCollection();
};

} // end of namespace Zotero

#endif // KBIBTEX_NETWORKING_ZOTERO_COLLECTION_H
