/*****************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#ifndef CONFIG_NOTIFICATIONHUB_H
#define CONFIG_NOTIFICATIONHUB_H

#include "kbibtexconfig_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXCONFIG_EXPORT NotificationListener
{
public:
    virtual void notificationEvent(int eventId) = 0;
    virtual ~NotificationListener();
};

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXCONFIG_EXPORT NotificationHub
{
public:
    NotificationHub(const NotificationHub &other) = delete;
    NotificationHub &operator= (const NotificationHub &other) = delete;
    ~NotificationHub();

    static const int EventAny;
    static const int EventConfigurationChanged;
    static const int EventUserDefined;

    static void registerNotificationListener(NotificationListener *listener, int eventId = NotificationHub::EventAny);
    static void unregisterNotificationListener(NotificationListener *listener, int eventId = NotificationHub::EventAny);

    static void publishEvent(int eventId);

private:
    class NotificationHubPrivate;
    NotificationHubPrivate *const d;

    static NotificationHub *getHub();

    NotificationHub();
};

#endif // CONFIG_NOTIFICATIONHUB_H
