/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QtNetworkAuth>
#include <QWizardPage>
#include <QTimer>

#include <kio_version.h>
#include <KLocalizedString>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
#include <KIO/OpenUrlJob>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
#include <KIO/JobUiDelegateFactory>
#else // < 5.98.0
#include <KIO/JobUiDelegate>
#endif // QT_VERSION_CHECK(5, 98, 0)
#else // < 5.71.0
#include <KRun>
#endif // KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)

#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

using namespace Zotero;

void openLink(const QString &link, QWidget *parent)
{
    static const QString mimeHTML{QStringLiteral("text/html")};
#if KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
    KRun::runUrl(QUrl(link), mimeHTML, parent, KRun::RunFlags());
#else // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0) // >= 5.71.0
    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(QUrl(link), mimeHTML);
#if KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // < 5.98.0
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parent));
#else // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // >= 5.98.0
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parent));
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0)
    job->start();
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
}

class ValidatedLineEditPage : public QWizardPage
{
private:
    QValidator *validator;
    QLineEdit *lineEdit;
    QLabel *labelExplanation;

public:

    ValidatedLineEditPage(const QString &explanation, const QString &lineEditLabel, QValidator *_validator, QWizard *parent)
            : QWizardPage(parent), validator(_validator)
    {

        QBoxLayout *boxLayout = new QVBoxLayout(this);

        labelExplanation = new QLabel(explanation, this);
        boxLayout->addWidget(labelExplanation);

        boxLayout->addSpacing(labelExplanation->fontMetrics().averageCharWidth());

        QFormLayout *formLayout{new QFormLayout()};
        boxLayout->addLayout(formLayout);
        lineEdit = new QLineEdit(this);
        formLayout->addRow(lineEditLabel, lineEdit);
        lineEdit->setValidator(validator);
        validator->setParent(lineEdit);
        connect(validator, &QValidator::changed, this, [this]() {
            Q_EMIT completeChanged();
        });
        connect(lineEdit, &QLineEdit::textChanged, this, [this](const QString &) {
            Q_EMIT completeChanged();
        });
        connect(labelExplanation, &QLabel::linkActivated, this, [this](const QString & link) {
            if (link.startsWith(QStringLiteral("http")))
                openLink(link, this);
        });
    }

    bool isComplete() const override
    {
        QString s{lineEdit->text()};
        int i{0};
        const bool r{!s.isEmpty() && validator->validate(s, i) == QValidator::Acceptable};
        return r;
    }

    inline bool isValid() const
    {
        return isComplete();
    }

    QString enteredText() const
    {
        return lineEdit->text();
    }

    void clear()
    {
        lineEdit->clear();
    }
};

class Zotero::OAuthWizard::Private
{
private:
    Zotero::OAuthWizard *p;
    QOAuth1 *qOAuth;

    QLineEdit *lineEditAuthorizationUrl;
    QPushButton *buttonCopyAuthorizationUrl, *buttonOpenAuthorizationUrl;

public:

    ValidatedLineEditPage *validatedAPIkeyPage, *validatedUserIDpage;

    Private(Zotero::OAuthWizard *parent)
            : p(parent) {
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
        const quint16 port = static_cast<quint16>(QRandomGenerator::global()->bounded(1025, 65533) & 0xffff);
        QOAuthHttpServerReplyHandler *replyHandler = new QOAuthHttpServerReplyHandler(port, parent);
        replyHandler->setCallbackPath(QStringLiteral("kbibtex-zotero-oauth"));
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
        connect(qOAuth, &QAbstractOAuth::requestFailed, parent, [](const QAbstractOAuth::Error error) {
            static const QHash<QAbstractOAuth::Error, QString> errorMsg{
                {QAbstractOAuth::Error::NoError, QStringLiteral("No error has ocurred")},
                {QAbstractOAuth::Error::NetworkError, QStringLiteral("Failed to connect to the server")},
                {QAbstractOAuth::Error::ServerError, QStringLiteral("The server answered the request with an error, or its response was not successfully received")},
                {QAbstractOAuth::Error::OAuthTokenNotFoundError, QStringLiteral("The server's response to a token request provided no token identifier")},
                {QAbstractOAuth::Error::OAuthTokenSecretNotFoundError, QStringLiteral("The server's response to a token request provided no token secret")},
                {QAbstractOAuth::Error::OAuthCallbackNotVerified, QStringLiteral("The authorization server has not verified the supplied callback URI in the request")}
            };
            qCWarning(LOG_KBIBTEX_NETWORKING) << "OAuth1 failed:" << errorMsg.value(error, QStringLiteral("Unknown error"));
        });
        connect(qOAuth, &QAbstractOAuth::statusChanged, parent, [this](QAbstractOAuth::Status status) {
            static const QHash<QAbstractOAuth::Status, QString> statusMsg{
                {QAbstractOAuth::Status::NotAuthenticated, QStringLiteral("No token has been retrieved")},
                {QAbstractOAuth::Status::TemporaryCredentialsReceived, QStringLiteral("Temporary credentials have been received, this status is used in some OAuth authetication methods")},
                {QAbstractOAuth::Status::Granted, QStringLiteral("Token credentials have been received and authenticated calls are allowed")},
                {QAbstractOAuth::Status::RefreshingToken, QStringLiteral("New token credentials have been requested")}
            };
            qCInfo(LOG_KBIBTEX_NETWORKING) << "OAuth status changed:" << statusMsg.value(status, QStringLiteral("Unknown status"));
        });
#ifdef EXTRA_VERBOSE
        connect(qOAuth, &QAbstractOAuth::granted, parent, [this]() {
            qCInfo(LOG_KBIBTEX_NETWORKING) << "OAuth: authorization flow finished successfully:" << qOAuth->token();
        });
        connect(qOAuth, &QAbstractOAuth::tokenChanged, parent, [this](const QString & token) {
            qCInfo(LOG_KBIBTEX_NETWORKING) << "tokenChanged: token=" << token;
        });
        connect(qOAuth, &QAbstractOAuth::extraTokensChanged, parent, [this](const QVariantMap & tokens) {
            for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it) {
                qCInfo(LOG_KBIBTEX_NETWORKING) << "extraTokensChanged: " << it.key() << "=" << it.value().toString();
            }
        });
        connect(qOAuth, &QAbstractOAuth::contentTypeChanged, parent, [](QAbstractOAuth::ContentType contentType) {
            qCInfo(LOG_KBIBTEX_NETWORKING) << "contentTypeChanged: contentType=" << static_cast<int>(contentType);
        });
        connect(qOAuth, &QAbstractOAuth::clientIdentifierChanged, parent, [](const QString & clientIdentifier) {
            qCInfo(LOG_KBIBTEX_NETWORKING) << "clientIdentifierChanged: clientIdentifier=" << clientIdentifier;
        });
        connect(qOAuth, &QAbstractOAuth::authorizationUrlChanged, parent, [](const QUrl & url) {
            qCInfo(LOG_KBIBTEX_NETWORKING) << "authorizationUrlChanged: url=" << url.toDisplayString();
        });
        connect(replyHandler, &QOAuthHttpServerReplyHandler::tokensReceived, parent, [this, parent](const QVariantMap & tokens) {
            qCDebug(LOG_KBIBTEX_NETWORKING) << "QOAuthHttpServerReplyHandler::tokensReceived";
            for (auto it = tokens.constBegin(), end = tokens.constEnd(); it != end; ++it) {
                qCDebug(LOG_KBIBTEX_NETWORKING) << "  key=" << qPrintable(it.key()) << " value=" << it.value();
            }
        });
        connect(replyHandler, &QOAuthHttpServerReplyHandler::replyDataReceived, parent, [this, parent](const QByteArray & data) {
            qCDebug(LOG_KBIBTEX_NETWORKING) << "QOAuthHttpServerReplyHandler::replyDataReceived  data=" << data.left(256);
        });
        connect(replyHandler, &QOAuthHttpServerReplyHandler::callbackReceived, parent, [this, parent](const QVariantMap & values) {
            qCDebug(LOG_KBIBTEX_NETWORKING) << "QOAuthHttpServerReplyHandler::replyDataReceived";
            for (auto it = values.constBegin(), end = values.constEnd(); it != end; ++it) {
                qCDebug(LOG_KBIBTEX_NETWORKING) << "  key=" << qPrintable(it.key()) << " value=" << it.value();
            }
        });
        connect(replyHandler, &QOAuthHttpServerReplyHandler::callbackDataReceived, parent, [this, parent](const QByteArray & data) {
            qCDebug(LOG_KBIBTEX_NETWORKING) << "QOAuthHttpServerReplyHandler::callbackDataReceived  data=" << data.left(256);
        });
#endif // EXTRA_VERBOSE
        qOAuth->grant();
    }

    void setupGUI() {
        QWizardPage *page{new QWizardPage(p)};
        p->addPage(page);
        page->setTitle(i18n("Authentication via Web Browser"));

        QGridLayout *gridLayout = new QGridLayout(page);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 0);
        QLabel *labelExplanation = new QLabel(i18n("<qt><p>To allow <strong>KBibTeX</strong> access your <strong>Zotero bibliography</strong>, this KBibTeX instance has to be authorized.</p><p>The process of authorization involves multiple steps:</p><ol><li>Open the URL as shown below in a web browser.</li><li>Log in at Zotero and approve the permissions for KBibTeX.</li><li>If successful, you will be redirected to a web page showing you a green text on green background like this:<br/><span style=\"color:#1e622d;background-color:#d8f2dd;border: 1px solid #c8edd0;\">zqB0KmelCOJcBvY40J</span><br/>Click on 'Next' to get to the next page in this wizard to enter this text.</li></ol></qt>"), page);
        gridLayout->addWidget(labelExplanation, 0, 0, 1, 3);
        gridLayout->setRowMinimumHeight(1, p->fontMetrics().xHeight() * 2);
        lineEditAuthorizationUrl = new QLineEdit(page);
        lineEditAuthorizationUrl->setReadOnly(true);
        gridLayout->addWidget(lineEditAuthorizationUrl, 2, 0, 1, 3);
        buttonCopyAuthorizationUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy URL"), p);
        gridLayout->addWidget(buttonCopyAuthorizationUrl, 3, 1, 1, 1);
        connect(buttonCopyAuthorizationUrl, &QPushButton::clicked, p, [this]() {
            QApplication::clipboard()->setText(lineEditAuthorizationUrl->text());
            QTimer::singleShot(500, p, [this]() {
                p->next();
            });
        });
        buttonOpenAuthorizationUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open-remote")), i18n("Open URL"), p);
        gridLayout->addWidget(buttonOpenAuthorizationUrl, 3, 2, 1, 1);
        connect(buttonOpenAuthorizationUrl, &QPushButton::clicked, p, [this]() {
            openLink(lineEditAuthorizationUrl->text(), p);
            QTimer::singleShot(500, p, [this]() {
                p->next();
            });
        });

        QValidator *v{new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[a-zA-Z0-9]{12,32}$")), p)};
        validatedAPIkeyPage = new ValidatedLineEditPage(i18n("<qt><p>In your web browser, you should have reached a web page showing you a green text on green background like this:<br/><span style=\"color:#1e622d;background-color:#d8f2dd;border: 1px solid #c8edd0;\">zqB0KmelCOJcBvY40J</span></p><p>In your web browser, copy this green and enter it in the text field below.</p></qt>"), i18n("API key:"), v, p);
        validatedAPIkeyPage->setTitle(i18n("Enter API Key from Web Browser"));
        p->addPage(validatedAPIkeyPage);

        v = new QIntValidator(1000, 99999999, p);
        validatedUserIDpage = new ValidatedLineEditPage(i18n("<qt><p>Finally, please enter the numeric user ID for use in API calls.<br/>This ID can be found on your <a href=\"https://www.zotero.org/settings/security#applications\">personal settings page in Zotero, in tab 'Security', section 'Applications'</a>.</p></qt>"), i18n("User ID:"), v, p);
        validatedUserIDpage->setTitle(i18n("Enter User ID for API Calls"));
        p->addPage(validatedUserIDpage);

        p->setWindowTitle(i18nc("@title:window", "Zotero OAuth Key Exchange"));
    }
};


OAuthWizard::OAuthWizard(QWidget *parent)
        : QWizard(parent), d(new OAuthWizard::Private(this))
{
    /// nothing
}

OAuthWizard::~OAuthWizard()
{
    delete d;
}

int OAuthWizard::exec()
{
    const int result = QWizard::exec();
    if (result != Accepted || userId().isEmpty() || apiKey().isEmpty()) {
        d->validatedUserIDpage->clear();
        d->validatedAPIkeyPage->clear();
        return Rejected;
    } else
        return Accepted;
}

QString OAuthWizard::userId() const
{
    return d->validatedUserIDpage->isValid() ? d->validatedUserIDpage->enteredText() : QString();
}

QString OAuthWizard::apiKey() const
{
    return d->validatedAPIkeyPage->isValid() ? d->validatedAPIkeyPage->enteredText() : QString();
}
