/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchunpaywall.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>

#ifdef HAVE_KF
#include <KLocalizedString>
#else // HAVE_KF
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF

#include <KBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchUnpaywall::Private
{
public:
    Private(OnlineSearchUnpaywall *_parent)
    {
        Q_UNUSED(_parent)
    }

    QUrl buildQueryUrl(const QMap<QueryKey, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(query[QueryKey::FreeText]);
        if (doiRegExpMatch.hasMatch())
            return QUrl::fromUserInput(QStringLiteral("https://api.unpaywall.org/v2/") + doiRegExpMatch.captured(0) + QStringLiteral("?email=") + InternalNetworkAccessManager::reverseObfuscate("\xb7\xd2\xaf\xcb\x25\xb\xa8\xc4\x16\x7d\x6c\x41\xda\xb3\x7a\x14\x25\x50\x27\x9\x49\x2e\x58\x39\x3b\x16\xdc\xa4\x8d\xe4\xbb\xd5\x6f\x1a\xb9\xf9\xb1\xe9\x44\x21\xaa\xfe\x4b\x29\x1a\x73\x20\x62\x88\xc3\x7f\x54\x62\x10\x7c\x19\x53\x3b\xc8\xab\x80\xf3\x43\x2a\x4d\x2b"));
        else if (query[QueryKey::FreeText].length() > 2)
            return QUrl::fromUserInput(QStringLiteral("https://api.unpaywall.org/v2/search/?query=") + query[QueryKey::FreeText] + QStringLiteral("&email=") + InternalNetworkAccessManager::reverseObfuscate("\xb7\xd2\xaf\xcb\x25\xb\xa8\xc4\x16\x7d\x6c\x41\xda\xb3\x7a\x14\x25\x50\x27\x9\x49\x2e\x58\x39\x3b\x16\xdc\xa4\x8d\xe4\xbb\xd5\x6f\x1a\xb9\xf9\xb1\xe9\x44\x21\xaa\xfe\x4b\x29\x1a\x73\x20\x62\x88\xc3\x7f\x54\x62\x10\x7c\x19\x53\x3b\xc8\xab\x80\xf3\x43\x2a\x4d\x2b"));

        return QUrl();
    }

    Entry *entryFromJsonObject(const QJsonObject &object) const {
        const QString title = object.value(QStringLiteral("title")).toString();
        const QString doi = object.value(QStringLiteral("doi")).toString();
        const int year = object.value(QStringLiteral("year")).toInt(-1);
        /// Basic sanity check
        if (title.isEmpty() || doi.length() < 3 || year < 1700)
            return nullptr;

        QString entryType{Entry::etMisc};
        if (object.value(QStringLiteral("genre")).toString() == QStringLiteral("journal-article"))
            entryType = Entry::etArticle;
        Entry *entry = new Entry(entryType, QString(QStringLiteral("Unpaywall:") + doi).replace(QStringLiteral(","), QStringLiteral("_")));
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(title)));
        entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QString::number(year))));
        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(doi);
        if (doiRegExpMatch.hasMatch())
            entry->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured())));
        const QString journal = object.value(QStringLiteral("journal_name")).toString();
        if (!journal.isEmpty())
            entry->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(journal)));
        const QString issn = object.value(QStringLiteral("journal_issn_l")).toString();
        if (!issn.isEmpty())
            entry->insert(Entry::ftISSN, Value() << QSharedPointer<PlainText>(new PlainText(issn)));
        const QString publisher = object.value(QStringLiteral("publisher")).toString();
        if (!publisher.isEmpty())
            entry->insert(Entry::ftPublisher, Value() << QSharedPointer<PlainText>(new PlainText(publisher)));

        const QJsonArray authorArray = object.value(QStringLiteral("z_authors")).toArray();
        Value authors;
        for (const QJsonValue &author : authorArray) {
            const QString familyName = author.toObject().value(QStringLiteral("family")).toString();
            const QString givenName = author.toObject().value(QStringLiteral("given")).toString();
            if (!familyName.isEmpty()) {
                QSharedPointer<Person> person{new Person(givenName, familyName)};
                authors.append(person);
            }
        }
        if (!authors.isEmpty())
            entry->insert(Entry::ftAuthor, authors);

        return entry;
    }
};

OnlineSearchUnpaywall::OnlineSearchUnpaywall(QObject *parent)
    : OnlineSearchAbstract(parent), d(new OnlineSearchUnpaywall::Private(this))
{
    // nothing
}

OnlineSearchUnpaywall::~OnlineSearchUnpaywall()
{
    delete d;
}

void OnlineSearchUnpaywall::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl(query, numResults);
    if (url.isValid()) {
        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchUnpaywall::downloadDone);
    } else
        delayedStoppedSearch(resultNoError);

    refreshBusyProperty();
}

QString OnlineSearchUnpaywall::label() const
{
    return i18n("Unpaywall");
}

QUrl OnlineSearchUnpaywall::homepage() const
{
    return QUrl(QStringLiteral("https://unpaywall.org/"));
}

void OnlineSearchUnpaywall::downloadDone()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            // Redirection to another url
            ++numSteps;
            Q_EMIT progress(++curStep, numSteps);

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchUnpaywall::downloadDone);
        } else {
            Q_EMIT progress(++curStep, numSteps);

            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (document.isObject()) {
                    Entry *entry = d->entryFromJsonObject(document.object());
                    if (entry != nullptr) {
                        publishEntry(QSharedPointer<Entry>(entry));
                        stopSearch(resultNoError);
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Unpaywall: Data could not be interpreted as a bibliographic entry";
                        stopSearch(resultUnspecifiedError);
                    }
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Unpaywall: Document is not an object";
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Unpaywall: " << parseError.errorString();
                stopSearch(resultUnspecifiedError);
            }
        }
    }

    refreshBusyProperty();
}
