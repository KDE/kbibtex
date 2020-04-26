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

#ifndef KBIBTEX_CONFIG_NOTIFICATIONHUB_H
#define KBIBTEX_CONFIG_NOTIFICATIONHUB_H

#include "kbibtexconfig_export.h"

#include <QtGlobal>

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
    ~NotificationHub();

    static const int EventAny;
    static const int EventConfigurationChanged;
    static const int EventBibliographySystemChanged;
    static const int EventUserDefined;

    static void registerNotificationListener(NotificationListener *listener, int eventId = NotificationHub::EventAny);
    static void unregisterNotificationListener(NotificationListener *listener, int eventId = NotificationHub::EventAny);

    static void publishEvent(int eventId);

private:
    Q_DISABLE_COPY(NotificationHub)

    class NotificationHubPrivate;
    NotificationHubPrivate *const d;

    static NotificationHub &instance();

    NotificationHub();
};

#endif // KBIBTEX_CONFIG_NOTIFICATIONHUB_H
