/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHSIMPLEBIBTEXDOWNLOAD_H
#define KBIBTEX_NETWORKING_ONLINESEARCHSIMPLEBIBTEXDOWNLOAD_H

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchSimpleBibTeXDownload : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchSimpleBibTeXDownload(QObject *parent);

    void startSearch(const QMap<QString, QString> &query, int numResults) override;

protected:
    virtual QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) = 0;
    virtual QString processRawDownload(const QString &download);

private slots:
    void downloadDone();
};

#endif // KBIBTEX_NETWORKING_ONLINESEARCHSIMPLEBIBTEXDOWNLOAD_H
