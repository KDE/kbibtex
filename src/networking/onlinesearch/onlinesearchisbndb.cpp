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

#include "onlinesearchisbndb.h"

#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrlQuery>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate
{
private:
    // UNUSED OnlineSearchIsbnDB *p;
    static const QString accessKey;
    static const QString booksUrl, authorsUrl;

public:
    XSLTransform *xslt;
    QUrl queryUrl;
    int currentPage, maxPage;

    OnlineSearchIsbnDBPrivate(OnlineSearchIsbnDB */* UNUSED parent*/)
        : /* UNUSED p(parent),*/ xslt(), currentPage(0), maxPage(0) {
        const QString xsltFilename = QStringLiteral("kbibtex/isbndb2bibtex.xsl");
        xslt = XSLTransform::createXSLTransform(QStandardPaths::locate(QStandardPaths::GenericDataLocation, xsltFilename));
        if (xslt == NULL)
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not create XSLT transformation for" << xsltFilename;
    }

    ~OnlineSearchIsbnDBPrivate() {
        delete xslt;
    }

    QUrl buildBooksUrl(const QMap<QString, QString> &query, int numResults) {
        currentPage = 1;
        maxPage = (numResults + 9) / 10;

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
    // nothing
}

OnlineSearchIsbnDB::~OnlineSearchIsbnDB()
{
    delete d;
}

void OnlineSearchIsbnDB::startSearch(const QMap<QString, QString> &query, int numResults)
{
    if (d->xslt == NULL) {
        /// Don't allow searches if xslt is not defined
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot allow searching" << label() << "if XSL Transformation not available";
        delayedStoppedSearch(resultUnspecifiedError);
        return;
    }

    m_hasBeenCanceled = false;

    emit progress(0, d->maxPage);

    QNetworkRequest request(d->buildBooksUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));
}

void OnlineSearchIsbnDB::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

QString OnlineSearchIsbnDB::label() const
{
    return i18n("ISBNdb.com");
}

QString OnlineSearchIsbnDB::favIconUrl() const
{
    return QStringLiteral("https://isbndb.com/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIsbnDB::customWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return NULL;
}

QUrl OnlineSearchIsbnDB::homepage() const
{
    return QUrl(QStringLiteral("http://isbndb.com/"));
}

void OnlineSearchIsbnDB::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchIsbnDB::downloadDone()
{
    emit progress(d->currentPage, d->maxPage);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString xmlCode = QString::fromUtf8(reply->readAll().constData());

        /// use XSL transformation to get BibTeX document from XML result
        const QString bibtexCode = d->xslt->transform(xmlCode).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")).replace(QStringLiteral("&amp;"), QStringLiteral("&"));

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibtexCode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntries |= publishEntry(entry);
            }
            delete bibtexFile;

            if (!hasEntries) {
                emit stoppedSearch(resultNoError);
            } else if (d->currentPage >= d->maxPage)
                emit stoppedSearch(resultNoError);
            else {
                ++d->currentPage;
                QUrl nextUrl = d->queryUrl;
                QUrlQuery query(nextUrl);
                query.addQueryItem(QStringLiteral("page_number"), QString::number(d->currentPage));
                nextUrl.setQuery(query);
                QNetworkRequest request(nextUrl);
                QNetworkReply *nextReply = InternalNetworkAccessManager::self()->get(request);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(nextReply);
                connect(nextReply, SIGNAL(finished()), this, SLOT(downloadDone()));
                return;
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}
