/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchisbndb.h"

#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QCoreApplication>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate
{
private:
    static const QString accessKey;
    static const QString booksUrl, authorsUrl;

public:
    const XSLTransform xslt;
    QUrl queryUrl;

    OnlineSearchIsbnDBPrivate(OnlineSearchIsbnDB *)
            : xslt(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QCoreApplication::instance()->applicationName().remove(QStringLiteral("test")) + QStringLiteral("/isbndb2bibtex.xsl")))
    {
        /// nothing
    }

    QUrl buildBooksUrl(const QMap<QString, QString> &query) {
        queryUrl = QUrl(booksUrl);
        QUrlQuery q(queryUrl);
        q.addQueryItem(QStringLiteral("access_key"), accessKey);
        q.addQueryItem(QStringLiteral("results"), QStringLiteral("texts,authors"));

        QString index1, value1;
        if (query[queryKeyFreeText].isEmpty() && query[queryKeyAuthor].isEmpty() && !query[queryKeyTitle].isEmpty()) {
            /// only searching for title
            index1 = QStringLiteral("title");
            value1 = query[queryKeyTitle];
        } else {
            /// multiple different values given
            index1 = QStringLiteral("full");
            value1 = query[queryKeyFreeText] + QLatin1Char(' ') + query[queryKeyAuthor] + QLatin1Char(' ') + query[queryKeyTitle];
        }
        q.addQueryItem(QStringLiteral("index1"), index1);
        q.addQueryItem(QStringLiteral("value1"), value1);

        queryUrl.setQuery(q);
        return queryUrl;
    }
};

const QString OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate::accessKey = QStringLiteral("NBTD24WJ");
const QString OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate::booksUrl = QStringLiteral("http://isbndb.com/api/books.xml");
const QString OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate::authorsUrl = QStringLiteral("http://isbndb.com/api/authors.xml");

OnlineSearchIsbnDB::OnlineSearchIsbnDB(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIsbnDBPrivate(this))
{
    /// nothing
}

OnlineSearchIsbnDB::~OnlineSearchIsbnDB()
{
    delete d;
}

void OnlineSearchIsbnDB::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    emit progress(curStep = 0, numSteps = (numResults + 9) / 10);

    QNetworkRequest request(d->buildBooksUrl(query));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchIsbnDB::downloadDone);
}

QString OnlineSearchIsbnDB::label() const
{
    return i18n("ISBNdb.com");
}

QString OnlineSearchIsbnDB::favIconUrl() const
{
    return QStringLiteral("http://isbndb.com/favicon.ico");
}

QUrl OnlineSearchIsbnDB::homepage() const
{
    return QUrl(QStringLiteral("http://isbndb.com/"));
}

void OnlineSearchIsbnDB::downloadDone()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString xmlCode = QString::fromUtf8(reply->readAll().constData());

        /// use XSL transformation to get BibTeX document from XML result
        const QString bibTeXcode = d->xslt.transform(xmlCode).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")).replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        if (bibTeXcode.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << reply->url().toDisplayString();
            stopSearch(resultInvalidArguments);
        } else {
            FileImporterBibTeX importer(this);
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                    hasEntries |= publishEntry(entry);
                }
                delete bibtexFile;

                if (!hasEntries) {
                    stopSearch(resultNoError);
                } else if (curStep >= numSteps)
                    stopSearch(resultNoError);
                else {
                    QUrl nextUrl = d->queryUrl;
                    QUrlQuery query(nextUrl);
                    query.addQueryItem(QStringLiteral("page_number"), QString::number(curStep /** FIXME? + 1 */));
                    nextUrl.setQuery(query);
                    QNetworkRequest request(nextUrl);
                    QNetworkReply *nextReply = InternalNetworkAccessManager::instance().get(request);
                    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(nextReply);
                    connect(nextReply, &QNetworkReply::finished, this, &OnlineSearchIsbnDB::downloadDone);
                    return;
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toDisplayString();
                stopSearch(resultUnspecifiedError);
            }
        }
    }
}
