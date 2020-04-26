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

#ifndef KBIBTEX_NETWORKING_ZOTERO_API_H
#define KBIBTEX_NETWORKING_ZOTERO_API_H

#include <QObject>
#include <QNetworkRequest>
#include <QUrl>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

namespace Zotero
{

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT API : public QObject
{
    Q_OBJECT

public:
    /**
     * Maximum number of items asked for in one request.
     * Multiple requests may be necessary to retrieve all data
     * the user asked for.
     */
    static const int limit;

    /**
     * Scope of a request. Can be either a group request or a user request.
     */
    enum class RequestScope { User, Group };

    /**
     * Generate an API object encapsulating low-level interaction with Zotero.
     * @param requestScope determines if a group's or a user's data shall be queried
     * @param userOrGroupPrefix identifier for group or user (not username)
     * @param apiKey necessary API key to authenticate and to get authorization for requests on private data
     * @param parent used for Qt-internal operations
     */
    explicit API(RequestScope requestScope, int userOrGroupPrefix, const QString &apiKey, QObject *parent = nullptr);

    ~API() override;

    /**
     * Add a limit parameter to a given Zotero URL.
     * @param url value of this parameter will be changed to contain the limit
     */
    void addLimitToUrl(QUrl &url) const;

    /**
     * Base URL for all requests to Zotero. Contains user/group id, API key, ...
     * @return Basic URL which only needs to have a proper path set
     */
    QUrl baseUrl() const;

    /**
     * @return Group or user prefix as specified in constructor.
     */
    int userOrGroupPrefix() const;

    /**
     * Create a request to Zotero based on the provided URL,
     * having proper HTTP headers set. This request can be passed
     * to a QNetworkAccessManager
     * @param url URL to base the request on
     * @return Request to Zotero
     */
    QNetworkRequest request(const QUrl &url) const;

    /**
     * A user of this API may receive a 'backoff' or 'retry-after'
     * notification from Zotero. In this case, this function has
     * to be called to notify this API object of the duration of
     * this backoff. Users can test if the API object is in 'backoff'
     * mode by calling @see inBackoffMode.
     * @param duration backoff's duration in seconds
     */
    void startBackoff(int duration);

    /**
     * Due to high load at Zotero or due to excessive use by
     * the current user, further request to Zotero shall be
     * backed off for some time. Before a request is made
     * through this API, call this function to test if the
     * API object is in 'backoff' mode.
     * The backoff is not enforced by this API, but Zotero
     * may deny answering request during this time period.
     * @return true if backoff mode, false otherwise
     */
    bool inBackoffMode() const;

    /**
     * Computes the number of seconds still left for backoff
     * mode (>=0). If the API is not in backoff mode, 0 will
     * be returned.
     * @return a non-negative number of seconds left in backoff mode
     */
    qint64 backoffSecondsLeft() const;

signals:
    /**
     * Signal gets emitted when backoff mode is entered.
     */
    void backoffModeStart();
    /**
     * Signal gets emitted when backoffmode is left.
     */
    void backoffModeEnd();

private:
    class Private;
    Private *const d;
};

} // end of namespace Zotero

#endif // KBIBTEX_NETWORKING_ZOTERO_API_H
