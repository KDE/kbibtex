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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHSEMANTICSCHOLAR_H
#define KBIBTEX_NETWORKING_ONLINESEARCHSEMANTICSCHOLAR_H

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF

class KBIBTEXNETWORKING_EXPORT OnlineSearchSemanticScholar : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchSemanticScholar(QObject *parent);
    ~OnlineSearchSemanticScholar() override;

#ifdef HAVE_QTWIDGETS
    void startSearchFromForm() override;
#endif // HAVE_QTWIDGETS
    void startSearch(const QMap<QueryKey, QString> &query, int numResults) override;
    QString label() const override;
#ifdef HAVE_QTWIDGETS
    OnlineSearchAbstract::Form *customWidget(QWidget *parent) override;
#endif // HAVE_QTWIDGETS
    QUrl homepage() const override;

private Q_SLOTS:
    void downloadDone();

private:
    class OnlineSearchQueryFormSemanticScholar;
    class OnlineSearchSemanticScholarPrivate;
    OnlineSearchSemanticScholarPrivate *d;
};

#endif // KBIBTEX_NETWORKING_ONLINESEARCHSEMANTICSCHOLAR_H
