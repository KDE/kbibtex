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

#include "oauthwizard.h"

#include <QLabel>
#include <QLayout>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QtNetworkAuth>

#include <KLocalizedString>
#include <KRun>
#include <kio_version.h>

#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

using namespace Zotero;

class Zotero::OAuthWizard::Private
{
private:
    Zotero::OAuthWizard *p;
    QOAuth1 *qOAuth;

    QLineEdit *lineEditAuthorizationUrl;
    QPushButton *buttonCopyAuthorizationUrl, *buttonOpenAuthorizationUrl;

public:

    int userId;
    QString apiKey;

    Private(Zotero::OAuthWizard *parent)
            : p(parent), userId(-1) {
        /// Configure the OAuth 1 object, including KBibTeX's app-specific credentials
        /// and various URLs required for the OAuth handshakes
        qOAuth = new QOAuth1(&InternalNetworkAccessManager::instance(), parent);
        qOAuth->setClientCredentials(InternalNetworkAccessManager::reverseObfuscate("\xb7\xd4\x7d\x48\xc0\xf9\x3f\x9\x3c\x5d\x10\x26\x36\x53\xf6\xcf\x54\x65\xee\xd6\x35\x50\xdb\xea\xb8\x8e\xf4\xcd\x14\x75\xbe\x8a\x55\x33\x4d\x28\xbe\xdb\x60\x51"), InternalNetworkAccessManager::reverseObfuscate("\x95\xa1\xa9\xcb\xc8\xfd\xd8\xbc\x43\x70\x54\x64\x5f\x68\x2b\x1e\xb3\xd1\x29\x1b\x39\xf\x89\xbb\x55\x66\x20\x42\xa7\xc5\x1a\x28\x4e\x7c\xf9\x9b\x9\x6b\x75\x4d"));
        qOAuth->setSignatureMethod(QOAuth1::SignatureMethod::Hmac_Sha1);
        qOAuth->setTemporaryCredentialsUrl(QUrl(QStringLiteral("https://www.zotero.org/oauth/request")));
        qOAuth->setAuthorizationUrl(QUrl(QStringLiteral("https://www.zotero.org/oauth/authorize")));
        qOAuth->setTokenCredentialsUrl(QUrl(QStringLiteral("https://www.zotero.org/oauth/access")));
        /// The QOAuthHttpServerReplyHandler will start a small webserver to which the
        /// user's webbrowser will be redirected to in case that KBibTeX got the
        /// requested permissions granted by the user when being logged in in Zotero.
        /// If the correct URL with an authorization token is requested from this
        /// local webserver, the credentials will be stored and this wizard dialog
        /// be closed.
        const quint16 port = static_cast<quint16>(qrand() % 50000) + 15000;
        QOAuthHttpServerReplyHandler *replyHandler = new QOAuthHttpServerReplyHandler(port, parent);
        replyHandler->setCallbackPath("kbibtex-zotero-oauth");
        replyHandler->setCallbackText(i18n("<html><head><title>KBibTeX authorized to use Zotero</title></head><body><p>KBibTeX got successfully authorized to read your Zotero database.</p></body></html>"));
        qOAuth->setReplyHandler(replyHandler);

        setupGUI();
        /// Disable various controls until delayed initialization is complete
        lineEditAuthorizationUrl->setEnabled(false);
        buttonCopyAuthorizationUrl->setEnabled(false);
        buttonOpenAuthorizationUrl->setEnabled(false);
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        connect(qOAuth, &QAbstractOAuth::authorizeWithBrowser, parent, [this](QUrl url) {
            QUrlQuery query(url);
            query.addQueryItem(QStringLiteral("name"), QStringLiteral("KBibTeX"));
            query.addQueryItem(QStringLiteral("library_access"), QStringLiteral("1"));
            query.addQueryItem(QStringLiteral("notes_access"), QStringLiteral("0"));
            query.addQueryItem(QStringLiteral("write_access"), QStringLiteral("0"));
            query.addQueryItem(QStringLiteral("all_groups"), QStringLiteral("read"));
            url.setQuery(query);
            /// Received a URL from Zotero which the user can open in a web browser.
            /// To do that, re-enable various controls.
            lineEditAuthorizationUrl->setText(url.toDisplayString());
            lineEditAuthorizationUrl->setEnabled(true);
            buttonCopyAuthorizationUrl->setEnabled(true);
            buttonOpenAuthorizationUrl->setEnabled(true);
            QApplication::restoreOverrideCursor();
        });
        connect(replyHandler, &QOAuthHttpServerReplyHandler::tokensReceived, parent, [this, parent](const QVariantMap &tokens) {
            /// Upon successful authorization, when the web browser is redirected
            /// to the local webserver run by the QOAuthHttpServerReplyHandler instance,
            /// extract the relevant parameters passed along as part of the GET
            /// request sent to this webserver.
            static const QString oauthTokenKey = QStringLiteral("oauth_token");
            static const QString userIdKey = QStringLiteral("userID");
            if (tokens.contains(oauthTokenKey) && tokens.contains(userIdKey)) {
                apiKey = tokens[oauthTokenKey].toString();
                bool ok = false;
                userId = tokens[userIdKey].toString().toInt(&ok);
                if (!ok) {
                    userId = -1;
                    apiKey.clear();
                    parent->reject();
                } else
                    parent->accept();
            } else {
                apiKey.clear();
                userId = -1;
            }
        });

        qOAuth->grant();
    }

    void setupGUI() {
        QGridLayout *gridLayout = new QGridLayout(p);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 0);
        QLabel *labelExplanation = new QLabel(i18n("<qt><p>To allow <strong>KBibTeX</strong> access your <strong>Zotero bibliography</strong>, this KBibTeX instance has to be authorized.</p><p>The process of authorization involves multiple steps:</p><ol><li>Open the URL as shown below in a web browser.</li><li>Log in at Zotero and approve the permissions for KBibTeX.</li><li>If successful, you will be redirected to a web page telling you that KBibTeX got authorized.<br/>This window will be closed automatically.</li></ol></qt>"), p);
        gridLayout->addWidget(labelExplanation, 0, 0, 1, 3);
        gridLayout->setRowMinimumHeight(1, p->fontMetrics().xHeight() * 2);
        lineEditAuthorizationUrl = new QLineEdit(p);
        lineEditAuthorizationUrl->setReadOnly(true);
        gridLayout->addWidget(lineEditAuthorizationUrl, 2, 0, 1, 3);
        buttonCopyAuthorizationUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy URL"), p);
        gridLayout->addWidget(buttonCopyAuthorizationUrl, 3, 1, 1, 1);
        connect(buttonCopyAuthorizationUrl, &QPushButton::clicked, p, [this]() {
            QApplication::clipboard()->setText(lineEditAuthorizationUrl->text());
        });
        buttonOpenAuthorizationUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open-remote")), i18n("Open URL"), p);
        gridLayout->addWidget(buttonOpenAuthorizationUrl, 3, 2, 1, 1);
        connect(buttonOpenAuthorizationUrl, &QPushButton::clicked, p, [this]() {
            KRun::runUrl(QUrl(lineEditAuthorizationUrl->text()), QStringLiteral("text/html"), p, KRun::RunFlags());
        });

        gridLayout->setRowMinimumHeight(4, p->fontMetrics().xHeight() * 2);
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, p);
        gridLayout->addWidget(buttonBox, 5, 0, 1, 3);
        connect(buttonBox, &QDialogButtonBox::clicked, [this, buttonBox](QAbstractButton *button) {
            if (button == buttonBox->button(QDialogButtonBox::Close))
                p->reject();
        });

        p->setWindowTitle(i18n("Zotero OAuth Key Exchange"));
    }
};


OAuthWizard::OAuthWizard(QWidget *parent)
        : QDialog(parent), d(new OAuthWizard::Private(this))
{
    /// nothing
}

OAuthWizard::~OAuthWizard()
{
    delete d;
}

int OAuthWizard::exec()
{
    const int result = QDialog::exec();
    if (result != Accepted || d->userId < 0) {
        d->userId = -1;
        d->apiKey.clear();
        return Rejected;
    } else
        return Accepted;
}

int OAuthWizard::userId() const
{
    return d->userId;
}

QString OAuthWizard::apiKey() const
{
    return d->apiKey;
}

#include "oauthwizard.moc"
