/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef KBIBTEX_NETWORKING_INTERNALNAM_H
#define KBIBTEX_NETWORKING_INTERNALNAM_H

#include "kbibtexnetworking_export.h"

#include <QNetworkAccessManager>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class HTTPEquivCookieJar;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT InternalNetworkAccessManager : public QNetworkAccessManager
{
public:
    static InternalNetworkAccessManager *self();
    QNetworkReply *get(QNetworkRequest &request, const QUrl &oldUrl);
    QNetworkReply *get(QNetworkRequest &request, const QNetworkReply *oldReply = NULL);

    void mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url);

protected:
    InternalNetworkAccessManager(QObject *parent = NULL);
    class HTTPEquivCookieJar;
    HTTPEquivCookieJar *cookieJar;

private:
    static InternalNetworkAccessManager *instance;
    static QString userAgentString;

    static QString userAgent();

};

#endif // KBIBTEX_NETWORKING_INTERNALNAM_H
