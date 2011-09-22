/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QNetworkCookieJar>

class QNetworkAccessManager;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class HTTPEquivCookieJar: public QNetworkCookieJar
{
    Q_OBJECT

public:
    static QNetworkAccessManager *networkAccessManager();
    static QString userAgent();

    void checkForHttpEqiuv(const QString &htmlCode, const QUrl &url);

protected:
    HTTPEquivCookieJar(QObject *parent = NULL);

private:
    static QNetworkAccessManager *m_networkAccessManager;
    static const QStringList m_userAgentList;
    static QString m_userAgent;
};
