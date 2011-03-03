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

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KConfigGroup>

#include "cookiemanager.h"

CookieManager *CookieManager::instance = NULL;

class CookieManager::CookieManagerPrivate
{
private:
    CookieManager *p;
    QMap<QString, QString> originalCookiesSettings;
    bool originalCookiesEnabled;
    int referenceCounter;

public:
    CookieManagerPrivate(CookieManager *parent)
            : p(parent), referenceCounter(0) {
        // nothing
    }


    bool cookiesEnabled() {
        QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<bool> reply = kded.call("loadModule", QString("kcookiejar"));

        KConfig cfg("kcookiejarrc");
        KConfigGroup group = cfg.group("Cookie Policy");
        QString answer = group.readEntry("Cookies", "false");
        return answer.toLower() == "true";
    }

    void setCookiesEnabled(bool enabled) {
        QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<bool> reply = kded.call("loadModule", QString("kcookiejar"));

        KConfig cfg("kcookiejarrc");
        KConfigGroup group = cfg.group("Cookie Policy");
        group.writeEntry("Cookies", enabled ? "true" : "false");

        if (enabled) {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
            kcookiejar.call("reloadPolicy");
        } else {
            QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
            kded.call("shutdown");
        }
    }

    bool changeCookieSettings(const QString &url) {
        Q_ASSERT(referenceCounter > 0);

        if (originalCookiesSettings.contains(url))
            return true;

        bool success = false;
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
        QDBusReply<QString> replyGetDomainAdvice = kcookiejar.call("getDomainAdvice", url);
        if (replyGetDomainAdvice.isValid()) {
            originalCookiesSettings.insert(url, replyGetDomainAdvice.value());
            QDBusReply<bool> replySetDomainAdvice = kcookiejar.call("setDomainAdvice", url, "accept");
            success = replySetDomainAdvice.isValid() && replySetDomainAdvice.value();
            if (success) {
                QDBusReply<void> reply = kcookiejar.call("reloadPolicy");
                success = reply.isValid();
            }
        }

        return success;
    }

    void initializeCookieSettings() {
        if (referenceCounter <= 0) {
            originalCookiesEnabled = cookiesEnabled();
            setCookiesEnabled(true);
            originalCookiesSettings.clear();
            referenceCounter = 0;
        }
        ++referenceCounter;
    }

    void restoreOldCookieSettings() {
        --referenceCounter;

        if (referenceCounter <= 0) {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
            for (QMap<QString, QString>::ConstIterator it = originalCookiesSettings.constBegin(); it != originalCookiesSettings.constEnd(); ++it)
                QDBusReply<bool> replySetDomainAdvice = kcookiejar.call("setDomainAdvice", it.key(), it.value());

            setCookiesEnabled(originalCookiesEnabled);
            kcookiejar.call("reloadPolicy");
            originalCookiesSettings.clear();
            referenceCounter = 0;
        }
    }

};

CookieManager::CookieManager()
        : d(new CookieManagerPrivate(this))
{
    // TODO
}

CookieManager *CookieManager::singleton()
{
    if (instance == NULL) {
        instance = new CookieManager();
    }

    return instance;
}

void CookieManager::disableCookieRestrictions()
{
    d->initializeCookieSettings();
}

void CookieManager::enableCookieRestrictions()
{
    d->restoreOldCookieSettings();
}

bool CookieManager::whitelistUrl(const QString &url)
{
    return d->changeCookieSettings(url);
}
