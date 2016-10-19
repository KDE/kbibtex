/***************************************************************************
 *   Copyright (C) 2016 by Thomas Fischer <fischer@unix-ag.uni-kl.de>      *
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

#include <QNetworkRequest>
#include <QNetworkReply>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchBioRxiv::Private
{
public:
    QSet<QUrl> resultPageUrls;
    int totalSteps, currentStep;

    explicit Private(OnlineSearchBioRxiv *)
            : totalSteps(0), currentStep(0)
    {
        /// nothing
    }
};

OnlineSearchBioRxiv::OnlineSearchBioRxiv(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchBioRxiv::Private(this))
{
    /// nothing
}

OnlineSearchBioRxiv::~OnlineSearchBioRxiv() {
    /// nothing
}

void OnlineSearchBioRxiv::startSearch() {
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchBioRxiv::startSearch(const QMap<QString, QString> &query, int numResults) {
    m_hasBeenCanceled = false;
    d->totalSteps = numResults * 2 + 1;
    d->currentStep = 0;
    emit progress(d->currentStep++, d->totalSteps);

    QString urlText(QString(QStringLiteral("http://biorxiv.org/search/numresults:%1 sort:relevance-rank title_flags:match-phrase format_result:standard ")).arg(numResults));
    urlText.append(query[queryKeyFreeText]);

    bool ok = false;
    int year = query[queryKeyYear].toInt(&ok);
    if (ok && year >= 1800 && year < 2100)
        urlText.append(QString(QStringLiteral(" limit_from:%1-01-01 limit_to:%1-12-31")).arg(year));

    const QStringList authors = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    int authorIndex = 1;
    for (QStringList::ConstIterator it = authors.constBegin(); it != authors.constEnd(); ++it, ++authorIndex)
        urlText.append(QString(QStringLiteral(" author%1:%2")).arg(authorIndex).arg(QString(*it).replace(QStringLiteral(" "), QStringLiteral("+"))));

    const QString title = QString(query[queryKeyTitle]).replace(QStringLiteral(" "), QStringLiteral("+"));
    if (!title.isEmpty())
        urlText.append(QString(QStringLiteral(" title:%1")).arg(title));

    QNetworkRequest request(urlText);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::resultsPageDone);
}

QString OnlineSearchBioRxiv::label() const {
    return i18n("bioRxiv");
}

OnlineSearchQueryFormAbstract *OnlineSearchBioRxiv::customWidget(QWidget *parent) {
    Q_UNUSED(parent)
    return NULL;
}

QUrl OnlineSearchBioRxiv::homepage() const {
    return QUrl(QStringLiteral("http://biorxiv.org/"));
}

QString OnlineSearchBioRxiv::favIconUrl() const {
    return QStringLiteral("http://d2538ggaoe6cji.cloudfront.net/sites/default/files/images/favicon.ico");
}

void OnlineSearchBioRxiv::resultsPageDone() {
    emit progress(d->currentStep++, d->totalSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString htmlCode = QString::fromUtf8(reply->readAll().constData());

        static const QRegExp contentRegExp(QStringLiteral("/content/early/[12]\\d{3}/[01]\\d/\\d{2}/\\d+"));
        int p = -1;
        while ((p = contentRegExp.indexIn(htmlCode, p + 1)) > 0) {
            const QUrl url = QUrl(QStringLiteral("http://biorxiv.org") + contentRegExp.cap(0));
            d->resultPageUrls.insert(url);
        }

        if (d->resultPageUrls.isEmpty())
            emit stoppedSearch(resultNoError);
        else {
            const QUrl firstUrl = *d->resultPageUrls.constBegin();
            d->resultPageUrls.remove(firstUrl);
            QNetworkRequest request(firstUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::resultPageDone);
        }
    }
}

void OnlineSearchBioRxiv::resultPageDone() {
    emit progress(d->currentStep++, d->totalSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString htmlCode = QString::fromUtf8(reply->readAll().constData());

        static const QRegExp highwireRegExp(QStringLiteral("/highwire/citation/\\d+/bibtex"));
        if (highwireRegExp.indexIn(htmlCode) > 0) {
            const QUrl url = QUrl(QStringLiteral("http://biorxiv.org") + highwireRegExp.cap(0));
            QNetworkRequest request(url);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::bibTeXDownloadDone);
        } else if (!d->resultPageUrls.isEmpty()) {
            const QUrl firstUrl = *d->resultPageUrls.constBegin();
            d->resultPageUrls.remove(firstUrl);
            QNetworkRequest request(firstUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::resultPageDone);
        } else
            emit stoppedSearch(resultNoError);
    }
}


void OnlineSearchBioRxiv::bibTeXDownloadDone() {
    emit progress(d->currentStep++, d->totalSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    publishEntry(entry);
                }

                delete bibtexFile;
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toDisplayString();
            }
        }
    }

    if (d->resultPageUrls.isEmpty())
        emit stoppedSearch(resultNoError);
    else {
        const QUrl firstUrl = *d->resultPageUrls.constBegin();
        d->resultPageUrls.remove(firstUrl);
        QNetworkRequest request(firstUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchBioRxiv::resultPageDone);
    }
}
