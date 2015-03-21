/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifdef HAVE_QTOAUTH // krazy:exclude=cpp

#include "oauthwizard.h"

#include <QLabel>
#include <QLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>

#include <KLocale>
#include <KLineEdit>
#include <KPushButton>
#include <KUrl>
#include <KDebug>
#include <KRun>

#include "internalnetworkaccessmanager.h"

#include <QtOAuth/QtOAuth>

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
private:
    const QRegExp correctInput;

public:
    HexInputLineEdit(int expectedLength, QWidget *parent)
            : KLineEdit(parent), correctInput(QString(QLatin1String("^[a-f0-9]{%1}$")).arg(expectedLength)) {
        setMaxLength(expectedLength);
    }

    bool hasAcceptableInput() const {
        return correctInput.exactMatch(text());
    }
};

class VerificationCodePage: public QWizardPage
{
private:
    Zotero::OAuthWizard *p;

public:
    HexInputLineEdit *lineEditVerificationCode;

    VerificationCodePage(Zotero::OAuthWizard *parent)
            : QWizardPage(parent), p(parent) {
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
        connect(lineEditVerificationCode, SIGNAL(textEdited(QString)), this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const {
        return lineEditVerificationCode->hasAcceptableInput();
    }

protected:
    virtual void initializePage() {
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
        qOAuth->setConsumerKey("1eef4a961e819e6a695c");
        qOAuth->setConsumerSecret("8bb22bb3262b5703d5b4");
        /// Set a timeout for requests (in msecs)
        qOAuth->setRequestTimeout(10000);
    }

    ~Private() {
        delete qOAuth;
    }

    // TODO separate GUI code from functional code
    QUrl oAuthAuthorizationUrl() {
        /// Send a request for an unauthorized token
        QOAuth::ParamMap params;
        params.insert("oauth_callback", "oob");
        QOAuth::ParamMap reply = qOAuth->requestToken("https://www.zotero.org/oauth/request", QOAuth::POST, QOAuth::HMAC_SHA1, params);

        /// If no error occurred, read the received token and token secret
        if (qOAuth->error() == QOAuth::NoError) {
            kDebug() << "Correctly retrieved authorization URL parameters";
            token = reply.value(QOAuth::tokenParameterName());
            tokenSecret = reply.value(QOAuth::tokenSecretParameterName());

            QUrl oauthAuthorizationUrl = QUrl(QLatin1String("https://www.zotero.org/oauth/authorize"));
            oauthAuthorizationUrl.addQueryItem("oauth_token", token);
            oauthAuthorizationUrl.addQueryItem("library_access", "1");
            oauthAuthorizationUrl.addQueryItem("notes_access", "0");
            oauthAuthorizationUrl.addQueryItem("write_access", "0");
            oauthAuthorizationUrl.addQueryItem("all_groups", "read");
            return oauthAuthorizationUrl;
        } else
            kWarning() << "Error getting token" << qOAuth->error();

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
        KPushButton *buttonCopyAuthorizationUrl = new KPushButton(KIcon("edit-copy"), i18n("Copy URL"), page);
        gridLayout->addWidget(buttonCopyAuthorizationUrl, 1, 1, 1, 1);
        connect(buttonCopyAuthorizationUrl, SIGNAL(clicked()), p, SLOT(copyAuthorizationUrl()));
        connect(buttonCopyAuthorizationUrl, SIGNAL(clicked()), p, SLOT(next()));
        KPushButton *buttonOpenAuthorizationUrl = new KPushButton(KIcon("document-open-remote"), i18n("Open URL"), page);
        gridLayout->addWidget(buttonOpenAuthorizationUrl, 1, 2, 1, 1);
        connect(buttonOpenAuthorizationUrl, SIGNAL(clicked()), p, SLOT(openAuthorizationUrl()));
        connect(buttonOpenAuthorizationUrl, SIGNAL(clicked()), p, SLOT(next()));
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
        QOAuth::ParamMap oAuthVerifierRequest = qOAuth->accessToken("https://www.zotero.org/oauth/access", QOAuth::GET, token, tokenSecret, QOAuth::HMAC_SHA1, oAuthVerifierParams);
        if (qOAuth->error() == QOAuth::NoError) {
            token = oAuthVerifierRequest.value(QOAuth::tokenParameterName());
            tokenSecret = oAuthVerifierRequest.value(QOAuth::tokenSecretParameterName());

            if (!token.isEmpty() && !tokenSecret.isEmpty()) {
                kDebug() << "KBibTeX is authorized successfully";
                bool ok = false;
                userId = oAuthVerifierRequest.value("userID").toInt(&ok);
                if (!ok) {
                    userId = -1;
                    apiKey.clear();
                    username.clear();
                    kWarning() << "Returned user id is not a valid number:" << oAuthVerifierRequest.value("userID");
                } else {
                    apiKey = oAuthVerifierRequest.value("oauth_token");
                    username = oAuthVerifierRequest.value("username");
                }
            } else {
                kWarning() << "QOAuth error: token or tokenSecret empty";
            }
        } else {
            kWarning() << "QOAuth error:" << qOAuth->error();
        }
    }
};


OAuthWizard::OAuthWizard(QWidget *parent)
        : QWizard(parent), d(new OAuthWizard::Private(this))
{
    d->setupGUI();
}

OAuthWizard::~OAuthWizard()
{
    delete d;
}

bool OAuthWizard::exec()
{
    const bool result = QWizard::exec() == Accepted && d->userId >= 0;
    return result;
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
        d->lineEditAuthorizationUrl->setText(d->oAuthAuthorizationUrl().pathOrUrl());
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
    KRun::runUrl(QUrl(d->lineEditAuthorizationUrl->text()), QLatin1String("text/html"), this);
}

#endif // HAVE_QTOAUTH
