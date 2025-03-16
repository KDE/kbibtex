/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2016-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchbiorxiv.h"

#ifdef HAVE_QTWIDGETS
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#endif // HAVE_QTWIDGETS
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QQueue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#ifdef HAVE_KF
#include <KConfigGroup>
#include <KLocalizedString>
#endif // HAVE_KF

#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "onlinesearchabstract_p.h"
#include "logging_networking.h"


#ifdef HAVE_QTWIDGETS
class OnlineSearchBioRxiv::Form : public OnlineSearchAbstract::Form
{
    Q_OBJECT

private:
    QString configGroupName;
    OnlineSearchBioRxiv::Rxiv rxiv;

    void loadState() {
        KConfigGroup configGroup(d->config, configGroupName);
        lineEditDoiNumber->setText(configGroup.readEntry(QStringLiteral("doiNumber"), QString()));
    }

public:
    QLineEdit *lineEditDoiNumber;

    Form(OnlineSearchBioRxiv::Rxiv _rxiv, QWidget *widget)
            : OnlineSearchAbstract::Form(widget), configGroupName(_rxiv == Rxiv::bioRxiv ? QStringLiteral("Search Engine bioRxiv") : (_rxiv == Rxiv::medRxiv ? QStringLiteral("Search Engine medRxiv") : QStringLiteral("Search Engine DOI"))), rxiv(_rxiv)
    {
        QGridLayout *layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        QLabel *label = new QLabel(i18n("DOI:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditDoiNumber = new QLineEdit(this);
        layout->addWidget(lineEditDoiNumber, 0, 1, 1, 1);
        lineEditDoiNumber->setClearButtonEnabled(true);
        connect(lineEditDoiNumber, &QLineEdit::returnPressed, this, &OnlineSearchBioRxiv::Form::returnPressed);

        layout->setRowStretch(1, 100);
        lineEditDoiNumber->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const override {
        return !lineEditDoiNumber->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) override {
        lineEditDoiNumber->setText(PlainTextValue::text(entry[Entry::ftDOI]));
    }

    void saveState() {
        KConfigGroup configGroup(d->config, configGroupName);
        configGroup.writeEntry(QStringLiteral("doiNumber"), lineEditDoiNumber->text());
        d->config->sync();
    }

};
#endif // HAVE_QTWIDGETS


class OnlineSearchBioRxiv::Private
{
public:
    OnlineSearchBioRxiv::Rxiv rxiv;
#ifdef HAVE_QTWIDGETS
    OnlineSearchBioRxiv::Form *form;
#endif // HAVE_QTWIDGETS

    explicit Private(OnlineSearchBioRxiv::Rxiv _rxiv, OnlineSearchBioRxiv *)
            : rxiv(_rxiv)
#ifdef HAVE_QTWIDGETS
        , form(nullptr)
#endif // HAVE_QTWIDGETS
    {
        /// nothing
    }

#ifdef HAVE_QTWIDGETS
    QUrl buildQueryUrl() {
        if (form == nullptr) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot build query url if no form is specified";
            return QUrl();
        }

        const QString server{rxiv == OnlineSearchBioRxiv::Rxiv::medRxiv ? QStringLiteral("medrxiv") : QStringLiteral("biorxiv")};
        const QString doi{form->lineEditDoiNumber->text()};
        return QUrl(QString(QStringLiteral("https://api.biorxiv.org/details/%1/%2/na/json")).arg(server, doi));
    }
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QueryKey, QString> &query) {
        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(query[QueryKey::FreeText]);
        if (doiRegExpMatch.hasMatch()) {
            const QString server{rxiv == OnlineSearchBioRxiv::Rxiv::medRxiv ? QStringLiteral("medrxiv") : QStringLiteral("biorxiv")};
            const QString doi{doiRegExpMatch.captured(QStringLiteral("doi"))};
            return QUrl(QString(QStringLiteral("https://api.biorxiv.org/details/%1/%2/na/json")).arg(server, doi));
        }

        return QUrl();
    }

    QVector<QSharedPointer<Entry >> parseJSON(const QByteArray &jsonData, bool *ok) {
        QVector<QSharedPointer<Entry >> result;

        // Source code generated by Python script 'onlinesearch-parser-generator.py'
        // using information from configuration file 'onlinesearchbiorxiv-parser.in.cpp'
#include "onlinesearch/onlinesearchbiorxiv-parser.generated.cpp"

        return result;
    }
};

OnlineSearchBioRxiv::OnlineSearchBioRxiv(OnlineSearchBioRxiv::Rxiv rxiv, QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchBioRxiv::Private(rxiv, this))
{
    /// nothing
}

OnlineSearchBioRxiv::~OnlineSearchBioRxiv() {
    delete d;
}

OnlineSearchBioRxiv::Rxiv OnlineSearchBioRxiv::rxiv() const
{
    return d->rxiv;
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchBioRxiv::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl();
    if (url.isValid()) {
        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::downloadDone);

        d->form->saveState();
    } else
        delayedStoppedSearch(resultNoError);

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchBioRxiv::startSearch(const QMap<QueryKey, QString> &query, int) {
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl(query);
    if (url.isValid()) {
        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::downloadDone);

        refreshBusyProperty();
    } else
        delayedStoppedSearch(resultNoError);
}

QString OnlineSearchBioRxiv::label() const {
    return d->rxiv == Rxiv::bioRxiv ? i18n("bioRxiv") : (d->rxiv == Rxiv::medRxiv ? i18n("medRxiv") : QString());
}

#ifdef HAVE_QTWIDGETS
OnlineSearchAbstract::Form *OnlineSearchBioRxiv::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchBioRxiv::Form(d->rxiv, parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchBioRxiv::homepage() const {
    return QUrl(d->rxiv == Rxiv::bioRxiv ? QStringLiteral("https://www.biorxiv.org/") : (d->rxiv == Rxiv::medRxiv ? QStringLiteral("https://www.medrxiv.org/") : QString()));
}

#ifdef BUILD_TESTING
QVector<QSharedPointer<Entry>> OnlineSearchBioRxiv::parseBioRxivJSON(const QByteArray &jsonData, bool *ok)
{
    return d->parseJSON(jsonData, ok);
}
#endif // BUILD_TESTING

void OnlineSearchBioRxiv::downloadDone()
{
    Q_EMIT progress(++curStep, numSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        Q_ASSERT(!redirUrl.isValid());
        const QByteArray jsonCode{reply->readAll()};

        if (!jsonCode.isEmpty()) {
            bool ok{false};
            QVector<QSharedPointer<Entry >> entries{d->parseJSON(jsonCode, &ok)};

            if (ok && !entries.isEmpty()) {
                publishEntry(entries.first());
                stopSearch(resultNoError);
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid JSON data returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                stopSearch(resultUnspecifiedError);
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Entry data returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultNoError);
        }
    }
}
#include "onlinesearchbiorxiv.moc"
