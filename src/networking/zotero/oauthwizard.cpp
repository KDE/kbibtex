/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>

#include <KLocalizedString>
#include <KLineEdit>
#include <KRun>
#include <kio_version.h>

#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

#include <QtOAuth>

using namespace Zotero;

/**
 * Specialization of a regular line edit, as only a specified number
 * of hexadecimal characters is accepted as valid input.
 * In contrast to using a validator like QRegExpValidator, this approach
 * still accepts paste operations from clipboard.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class HexInputLineEdit: public KLineEdit
{
    Q_OBJECT

private:
    const QRegularExpression correctInput;

public:
    HexInputLineEdit(int expectedLength, QWidget *parent)
            : KLineEdit(parent), correctInput(QString(QStringLiteral("^[a-f0-9]{%1}$")).arg(expectedLength)) {
        setMaxLength(expectedLength);
    }

    bool hasAcceptableInput() const {
        return correctInput.match(text()).hasMatch();
    }
};

class VerificationCodePage: public QWizardPage
{
    Q_OBJECT

private:
    // UNUSED Zotero::OAuthWizard *p;

public:
    HexInputLineEdit *lineEditVerificationCode;

    VerificationCodePage(Zotero::OAuthWizard *parent)
            : QWizardPage(parent) { /* UNUSED, p(parent)*/
        QBoxLayout *layout = new QVBoxLayout(this);
        setTitle(i18n("Verification Code"));
        setSubTitle(i18n("Copy the code from the web page and paste it in the input field below."));
        QLabel *label = new QLabel(i18n("<qt><p>Once you have granted permissions to KBibTeX on Zotero's web page, you should see a light-green field with a black hexadecimal code (example: <span style=\"color:black;background:#cfc;\">48b67422661cc282cdea</span>).</p></qt>"), this);
        label->setWordWrap(true);
        layout->addWidget(label);
        layout->addSpacing(8);
        QFormLayout *formLayout = new QFormLayout();
        layout->addLayout(formLayout);
        lineEditVerificationCode = new HexInputLineEdit(20, this);
        formLayout->addRow(i18n("Verification Code:"), lineEditVerificationCode);
        connect(lineEditVerificationCode, &HexInputLineEdit::textEdited, this, &VerificationCodePage::completeChanged);
    }

    bool isComplete() const override {
        return lineEditVerificationCode->hasAcceptableInput();
    }

protected:
    void initializePage() override {
        lineEditVerificationCode->clear();
    }
};

class Zotero::OAuthWizard::Private
{
private:
    Zotero::OAuthWizard *p;

    QOAuth::Interface *qOAuth;
    QByteArray token;
    QByteArray tokenSecret;

public:
    KLineEdit *lineEditAuthorizationUrl;
    VerificationCodePage *verificationCodePage;

    int pageIdInstructions, pageIdAuthorizationUrl, pageIdVerificationCode;

    int userId;
    QString apiKey, username;

    Private(Zotero::OAuthWizard *parent)
            : p(parent), userId(-1) {
        qOAuth = new QOAuth::Interface(p);
        /// Set the consumer key and secret
        qOAuth->setConsumerKey(InternalNetworkAccessManager::reverseObfuscate("\xb7\xd4\x7d\x48\xc0\xf9\x3f\x9\x3c\x5d\x10\x26\x36\x53\xf6\xcf\x54\x65\xee\xd6\x35\x50\xdb\xea\xb8\x8e\xf4\xcd\x14\x75\xbe\x8a\x55\x33\x4d\x28\xbe\xdb\x60\x51").toLatin1());
        qOAuth->setConsumerSecret(InternalNetworkAccessManager::reverseObfuscate("\x95\xa1\xa9\xcb\xc8\xfd\xd8\xbc\x43\x70\x54\x64\x5f\x68\x2b\x1e\xb3\xd1\x29\x1b\x39\xf\x89\xbb\x55\x66\x20\x42\xa7\xc5\x1a\x28\x4e\x7c\xf9\x9b\x9\x6b\x75\x4d").toLatin1());
        /// Set a timeout for requests (in msecs)
        qOAuth->setRequestTimeout(10000);

        setupGUI();
    }

    ~Private() {
        delete qOAuth;
    }

    // TODO separate GUI code from functional code
    QUrl oAuthAuthorizationUrl() {
        /// Send a request for an unauthorized token
        QOAuth::ParamMap params;
        params.insert("oauth_callback", "oob");
        QOAuth::ParamMap reply = qOAuth->requestToken(QStringLiteral("https://www.zotero.org/oauth/request"), QOAuth::POST, QOAuth::HMAC_SHA1, params);

        /// If no error occurred, read the received token and token secret
        if (qOAuth->error() == QOAuth::NoError) {
            token = reply.value(QOAuth::tokenParameterName());
            tokenSecret = reply.value(QOAuth::tokenSecretParameterName());

            QUrl oauthAuthorizationUrl = QUrl(QStringLiteral("https://www.zotero.org/oauth/authorize"));
            QUrlQuery query(oauthAuthorizationUrl);
            query.addQueryItem(QStringLiteral("oauth_token"), token);
            query.addQueryItem(QStringLiteral("library_access"), QStringLiteral("1"));
            query.addQueryItem(QStringLiteral("notes_access"), QStringLiteral("0"));
            query.addQueryItem(QStringLiteral("write_access"), QStringLiteral("0"));
            query.addQueryItem(QStringLiteral("all_groups"), QStringLiteral("read"));
            oauthAuthorizationUrl.setQuery(query);
            return oauthAuthorizationUrl;
        } else
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Error getting token" << qOAuth->error();

        return QUrl();
    }

    void setupGUI() {
        QWizardPage *page = new QWizardPage(p);
        pageIdInstructions = p->addPage(page);
        QBoxLayout *layout = new QVBoxLayout(page);
        page->setTitle(i18n("Instructions"));
        page->setSubTitle(i18n("<qt>To allow <strong>KBibTeX</strong> access your <strong>Zotero bibliography</strong>, this KBibTeX instance has to be authorized.</qt>"));
        QLabel *label = new QLabel(i18n("<qt><p>The process of authorization involves multiple steps:</p><ol><li>Open the URL as shown on the next page.</li><li>Follow the instructions as shown on Zotero's webpage.<br/>Configure permissions for KBibTeX.</li><li>Eventually, you will get a hexadecimal string (black on light-green background) which you have to copy and paste into this wizard's last page</li></ol></qt>"), page);
        label->setWordWrap(true);
        layout->addWidget(label);

        page = new QWizardPage(p);
        pageIdAuthorizationUrl = p->addPage(page);
        QGridLayout *gridLayout = new QGridLayout(page);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 0);
        page->setTitle(i18n("Authorization URL"));
        page->setSubTitle(i18n("Open the shown URL in your favorite browser."));
        lineEditAuthorizationUrl = new KLineEdit(page);
        lineEditAuthorizationUrl->setReadOnly(true);
        gridLayout->addWidget(lineEditAuthorizationUrl, 0, 0, 1, 3);
        QPushButton *buttonCopyAuthorizationUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy URL"), page);
        gridLayout->addWidget(buttonCopyAuthorizationUrl, 1, 1, 1, 1);
        connect(buttonCopyAuthorizationUrl, &QPushButton::clicked, p, &Zotero::OAuthWizard::copyAuthorizationUrl);
        connect(buttonCopyAuthorizationUrl, &QPushButton::clicked, p, &Zotero::OAuthWizard::next);
        QPushButton *buttonOpenAuthorizationUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open-remote")), i18n("Open URL"), page);
        gridLayout->addWidget(buttonOpenAuthorizationUrl, 1, 2, 1, 1);
        connect(buttonOpenAuthorizationUrl, &QPushButton::clicked, p, &Zotero::OAuthWizard::openAuthorizationUrl);
        connect(buttonOpenAuthorizationUrl, &QPushButton::clicked, p, &Zotero::OAuthWizard::next);
        gridLayout->setRowMinimumHeight(2, 8);
        label = new QLabel(i18n("You will be asked to login into Zotero and select and confirm the permissions for KBibTeX."), page);
        label->setWordWrap(true);
        gridLayout->addWidget(label, 3, 0, 1, 3);

        verificationCodePage = new VerificationCodePage(p);
        pageIdVerificationCode = p->addPage(verificationCodePage);

        p->setWindowTitle(i18n("Zotero OAuth Key Exchange"));
    }

    // TODO separate GUI code from functional code
    void setOAuthVerifier(const QString &verifier) {
        QOAuth::ParamMap oAuthVerifierParams;
        oAuthVerifierParams.insert("oauth_verifier", verifier.toUtf8());
        QOAuth::ParamMap oAuthVerifierRequest = qOAuth->accessToken(QStringLiteral("https://www.zotero.org/oauth/access"), QOAuth::GET, token, tokenSecret, QOAuth::HMAC_SHA1, oAuthVerifierParams);
        if (qOAuth->error() == QOAuth::NoError) {
            token = oAuthVerifierRequest.value(QOAuth::tokenParameterName());
            tokenSecret = oAuthVerifierRequest.value(QOAuth::tokenSecretParameterName());

            if (!token.isEmpty() && !tokenSecret.isEmpty()) {
                bool ok = false;
                userId = oAuthVerifierRequest.value("userID").toInt(&ok);
                if (!ok) {
                    userId = -1;
                    apiKey.clear();
                    username.clear();
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Returned user id is not a valid number:" << oAuthVerifierRequest.value("userID");
                } else {
                    apiKey = oAuthVerifierRequest.value("oauth_token");
                    username = oAuthVerifierRequest.value("username");
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "QOAuth error: token or tokenSecret empty";
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "QOAuth error:" << qOAuth->error();
        }
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
    return d->userId >= 0 ? result : Rejected;
}

int OAuthWizard::userId() const
{
    return d->userId;
}

QString OAuthWizard::apiKey() const
{
    return d->apiKey;
}

QString OAuthWizard::username() const
{
    return d->username;
}

void OAuthWizard::initializePage(int id)
{
    if (id == d->pageIdInstructions) {
        /// nothing
    } else if (id == d->pageIdAuthorizationUrl) {
        const QCursor currentCursor = cursor();
        setCursor(Qt::WaitCursor);
        d->lineEditAuthorizationUrl->setText(d->oAuthAuthorizationUrl().url(QUrl::PreferLocalFile));
        d->lineEditAuthorizationUrl->setCursorPosition(0);
        setCursor(currentCursor);
    } else if (id == d->pageIdVerificationCode) {
        /// This page initializes itself, see VerificationCodePage::initializePage
    }
}

void OAuthWizard::accept()
{
    const QCursor currentCursor = cursor();
    setCursor(Qt::WaitCursor);
    d->setOAuthVerifier(d->verificationCodePage->lineEditVerificationCode->text());
    setCursor(currentCursor);
    QWizard::accept();
}

void OAuthWizard::copyAuthorizationUrl()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(d->lineEditAuthorizationUrl->text());
}

void OAuthWizard::openAuthorizationUrl()
{
#if KIO_VERSION < 0x051f00 // < 5.31.0
    KRun::runUrl(QUrl(d->lineEditAuthorizationUrl->text()), QStringLiteral("text/html"), this, false, false);
#else // KIO_VERSION < 0x051f00 // >= 5.31.0
    KRun::runUrl(QUrl(d->lineEditAuthorizationUrl->text()), QStringLiteral("text/html"), this, KRun::RunFlags());
#endif // KIO_VERSION < 0x051f00
}

#include "oauthwizard.moc"
