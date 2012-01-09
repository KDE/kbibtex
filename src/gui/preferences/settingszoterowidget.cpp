/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <QFormLayout>
#include <QNetworkReply>
#include <QLabel>
#include <QTextStream>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>

#include "internalnetworkaccessmanager.h"
#include "zotero/readapi.h"
#include "settingszoterowidget.h"

class SettingsZoteroWidget::SettingsZoteroWidgetPrivate
{
private:
    SettingsZoteroWidget *p;

    KPushButton *buttonTestZoteroSettings;

    KSharedConfigPtr config;

public:
    KLineEdit *lineEditZoteroUserId;
    KLineEdit *lineEditZoteroPrivateKey;
    QLabel *labelLibrary;

    SettingsZoteroWidgetPrivate(SettingsZoteroWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, ZoteroReadAPI::configGroupNameZotero);
        lineEditZoteroUserId->setText(configGroup.readEntry(ZoteroReadAPI::keyUserID, ZoteroReadAPI::defaultUserID));
        lineEditZoteroPrivateKey->setText(configGroup.readEntry(ZoteroReadAPI::keyPrivateKey, ZoteroReadAPI::defaultPrivateKey));
    }

    void saveState() {
        KConfigGroup configGroup(config, ZoteroReadAPI::configGroupNameZotero);
        configGroup.writeEntry(ZoteroReadAPI::keyUserID, lineEditZoteroUserId->text());
        configGroup.writeEntry(ZoteroReadAPI::keyPrivateKey, lineEditZoteroPrivateKey->text());
        config->sync();
    }

    void resetToDefaults() {
        lineEditZoteroUserId->setText(ZoteroReadAPI::defaultUserID);
        lineEditZoteroPrivateKey->setText(ZoteroReadAPI::defaultPrivateKey);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        lineEditZoteroUserId = new KLineEdit(p);
        layout->addRow(i18n("Numeric user id:"), lineEditZoteroUserId);
        connect(lineEditZoteroUserId, SIGNAL(textChanged(QString)), p, SIGNAL(changed()));

        lineEditZoteroPrivateKey = new KLineEdit(p);
        layout->addRow(i18n("Alpha-numeric private key:"), lineEditZoteroPrivateKey);
        connect(lineEditZoteroPrivateKey, SIGNAL(textChanged(QString)), p, SIGNAL(changed()));

        buttonTestZoteroSettings = new KPushButton(i18n("Test Zotero settings"), p);
        layout->addRow(QString(), buttonTestZoteroSettings);
        connect(buttonTestZoteroSettings, SIGNAL(clicked()), p, SLOT(slotTestZoteroSettings()));

        layout->addRow(i18n("Library"), labelLibrary = new QLabel(QLatin1String("?"), p));
    }
};

SettingsZoteroWidget::SettingsZoteroWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsZoteroWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

void SettingsZoteroWidget::loadState()
{
    d->loadState();
}

void SettingsZoteroWidget::saveState()
{
    d->saveState();
}

void SettingsZoteroWidget::resetToDefaults()
{
    d->resetToDefaults();
}

void SettingsZoteroWidget::slotTestZoteroSettings()
{
    KUrl url(ZoteroReadAPI::zoteroApiUrlPrefix + QString("/users/%1/keys/%2?format=atom&key=%2").arg(d->lineEditZoteroUserId->text()).arg(d->lineEditZoteroPrivateKey->text()));
    setEnabled(false);
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(slotTestResultsFinished()));
}

void SettingsZoteroWidget::slotTestResultsFinished()
{
    d->labelLibrary->setText(i18n("Access denied"));

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        QTextStream ts(reply);
        const QString xml = ts.readAll();

        int p1 = xml.indexOf(QLatin1String(" library=\"1"));
        if (p1 > 0)
            d->labelLibrary->setText(i18n("Access allowed"));
    }
    setEnabled(true);
}
