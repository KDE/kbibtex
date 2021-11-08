/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHZBMATH_H
#define KBIBTEX_NETWORKING_ONLINESEARCHZBMATH_H

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *
 * See https://oai.zbmath.org/ for more technical information about zbMATH.
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchZbMath : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchZbMath(QObject *parent);
    ~OnlineSearchZbMath() override;

    void startSearch(const QMap<QueryKey, QString> &query, int numResults) override;
    QString label() const override;
    QUrl homepage() const override;

private slots:
    void doneFetchingOAI();

private:
    class Private;
    Private *d;
};

#endif // KBIBTEX_NETWORKING_ONLINESEARCHZBMATH_H
