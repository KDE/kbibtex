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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHGENERAL_H
#define KBIBTEX_NETWORKING_ONLINESEARCHGENERAL_H

#include <KSharedConfig>

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

#ifdef HAVE_QTWIDGETS
namespace OnlineSearchGeneral {
/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT Form : public OnlineSearchAbstract::Form
{
public:
    explicit Form(QWidget *parent);
    ~Form();

    virtual bool readyToStart() const override;
    virtual void copyFromEntry(const Entry &) override;

    QMap<OnlineSearchAbstract::QueryKey, QString> getQueryTerms();
    int getNumResults();

private:
    class Private;
    Private *dg;
};
} /// namespace OnlineSearchGeneral
#endif // HAVE_QTWIDGETS


#endif // KBIBTEX_NETWORKING_ONLINESEARCHGENERAL_H
