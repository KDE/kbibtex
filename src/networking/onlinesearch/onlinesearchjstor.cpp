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

#include "onlinesearchjstor.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <KLocalizedString>

#include "internalnetworkaccessmanager.h"
#include "fileimporterbibtex.h"
#include "logging_networking.h"

class OnlineSearchJStor::OnlineSearchJStorPrivate
{
private:
    // UNUSED OnlineSearchJStor *p;

public:
    int numExpectedResults, curStep, numSteps;
    static const QString jstorBaseUrl;
    QUrl queryUrl;

    OnlineSearchJStorPrivate(OnlineSearchJStor */* UNUSED parent*/)
        : /* UNUSED p(parent), */ numExpectedResults(0), curStep(0), numSteps(0)
    {
        /// nothing
    }
};

const QString OnlineSearchJStor::OnlineSearchJStorPrivate::jstorBaseUrl = QStringLiteral("http://www.jstor.org/");

OnlineSearchJStor::OnlineSearchJStor(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchJStorPrivate(this))
{
    /// nothing
}

OnlineSearchJStor::~OnlineSearchJStor()
{
    delete d;
}

void OnlineSearchJStor::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 3;
    d->numExpectedResults = numResults;

    /// Build search URL, to be used in the second step
    /// after fetching the start page
    d->queryUrl = QUrl(OnlineSearchJStorPrivate::jstorBaseUrl);
    QUrlQuery q(d->queryUrl);
    d->queryUrl.setPath(QStringLiteral("/action/doAdvancedSearch"));
    q.addQueryItem(QStringLiteral("Search"), QStringLiteral("Search"));
    q.addQueryItem(QStringLiteral("wc"), QStringLiteral("on")); /// include external references, too
    q.addQueryItem(QStringLiteral("la"), QStringLiteral("")); /// no specific language
    q.addQueryItem(QStringLiteral("jo"), QStringLiteral("")); /// no specific journal
    q.addQueryItem(QStringLiteral("hp"), QString::number(numResults)); /// hits per page
    int queryNumber = 0;
    QStringList elements = splitRespectingQuotationMarks(query[queryKeyTitle]);
    foreach (const QString &element, elements) {
        if (queryNumber > 0) q.addQueryItem(QString(QStringLiteral("c%1")).arg(queryNumber), QStringLiteral("AND")); ///< join search terms with an AND operation
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(queryNumber), QStringLiteral("ti"));
        q.addQueryItem(QString(QStringLiteral("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    elements = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    foreach (const QString &element, elements) {
        if (queryNumber > 0) q.addQueryItem(QString(QStringLiteral("c%1")).arg(queryNumber), QStringLiteral("AND")); ///< join search terms with an AND operation
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(queryNumber), QStringLiteral("au"));
        q.addQueryItem(QString(QStringLiteral("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    elements = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    foreach (const QString &element, elements) {
        if (queryNumber > 0) q.addQueryItem(QString(QStringLiteral("c%1")).arg(queryNumber), QStringLiteral("AND")); ///< join search terms with an AND operation
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(queryNumber), QStringLiteral("all"));
        q.addQueryItem(QString(QStringLiteral("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    if (!query[queryKeyYear].isEmpty()) {
        q.addQueryItem(QStringLiteral("sd"), query[queryKeyYear]);
        q.addQueryItem(QStringLiteral("ed"), query[queryKeyYear]);
    }
    d->queryUrl.setQuery(q);

    QNetworkRequest request(OnlineSearchJStorPrivate::jstorBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingStartPage);
    emit progress(d->curStep, d->numSteps);
}

void OnlineSearchJStor::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

QString OnlineSearchJStor::label() const
{
    return i18n("JSTOR");
}

QString OnlineSearchJStor::favIconUrl() const
{
    return QStringLiteral("http://www.jstor.org/assets/search_20151218T0921/files/search/images/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchJStor::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchJStor::homepage() const
{
    return QUrl(QStringLiteral("http://www.jstor.org/"));
}

void OnlineSearchJStor::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchJStor::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            ++d->numSteps;

            /// redirection to another url
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply->url());
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingStartPage);
        } else {
            QNetworkRequest request(d->queryUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingResultPage);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

void OnlineSearchJStor::doneFetchingResultPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlText = QString::fromUtf8(reply->readAll().constData());

        /// extract all unique DOI from HTML code
        QStringList uniqueDOIs;
        int p = -6;
        while ((p = KBibTeX::doiRegExp.indexIn(htmlText, p + 6)) >= 0) {
            QString doi = KBibTeX::doiRegExp.cap(0);
            // FIXME DOI RegExp accepts DOIs with question marks, causes problems here
            const int p = doi.indexOf(QLatin1Char('?'));
            if (p > 0) doi = doi.left(p);
            if (!uniqueDOIs.contains(doi))
                uniqueDOIs.append(doi);
        }

        if (uniqueDOIs.isEmpty()) {
            /// No results found
            emit progress(d->numSteps, d->numSteps);
            emit stoppedSearch(resultNoError);
        } else {
            /// Build POST request that should return a BibTeX document
            QString body;
            int numResults = 0;
            for (QStringList::ConstIterator it = uniqueDOIs.constBegin(); numResults < d->numExpectedResults && it != uniqueDOIs.constEnd(); ++it, ++numResults) {
                if (!body.isEmpty()) body.append(QStringLiteral("&"));
                body.append(QStringLiteral("citations=") + encodeURL(*it));
            }
            QUrl bibTeXUrl = QUrl(OnlineSearchJStorPrivate::jstorBaseUrl);
            bibTeXUrl.setPath(QStringLiteral("/citation/bulk/text"));
            QNetworkRequest request(bibTeXUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->post(request, body.toUtf8());
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingBibTeXCode);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

void OnlineSearchJStor::doneFetchingBibTeXCode()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);
        int numFoundResults = 0;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (publishEntry(entry))
                    ++numFoundResults;

            }
            delete bibtexFile;
        }

        emit progress(d->numSteps, d->numSteps);
        emit stoppedSearch(numFoundResults > 0 ? resultNoError : resultUnspecifiedError);
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

void OnlineSearchJStor::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

    if (KBibTeX::doiRegExp.indexIn(entry->id()) == 0) {
        /// entry ID is a DOI
        Value v;
        v.append(QSharedPointer<VerbatimText>(new VerbatimText(KBibTeX::doiRegExp.cap(0))));
        entry->insert(Entry::ftDOI, v);
    }

    QString url = PlainTextValue::text(entry->value(Entry::ftUrl));
    if (url.startsWith(QStringLiteral("http://www.jstor.org/stable/"))) {
        /// use JSTOR's own stable ID for entry ID
        entry->setId("jstor" + url.mid(28).replace(QLatin1Char(','), QString()));
        /// store JSTOR's own stable ID
        Value v;
        v.append(QSharedPointer<VerbatimText>(new VerbatimText(url.mid(28))));
        entry->insert(QStringLiteral("jstor_id"), v);
    }

    const QString formattedDateKey = QStringLiteral("jstor_formatteddate");
    const QString formattedDate = PlainTextValue::text(entry->value(formattedDateKey));

    /// first, try to guess month by inspecting the beginning
    /// the jstor_formatteddate field
    const QString formattedDateLower = formattedDate.toLower();
    int i;
    for (i = 0; i < 12; ++i)
        if (formattedDateLower.startsWith(KBibTeX::MonthsTriple[i])) break;
    entry->remove(formattedDateKey);
    if (i < 12) {
        Value v;
        v.append(QSharedPointer<MacroKey>(new MacroKey(KBibTeX::MonthsTriple[i])));
        entry->insert(Entry::ftMonth, v);
    }
    /// guessing failed, therefore extract first part if it exists
    else if ((i = formattedDate.indexOf(QStringLiteral(","))) >= 0) {
        /// text may be a season ("Winter")
        Value v;
        v.append(QSharedPointer<PlainText>(new PlainText(formattedDate.left(i))));
        entry->insert(Entry::ftMonth, v);
    } else {
        /// this case happens if the field only contains a year
        //qCDebug(LOG_KBIBTEX_NETWORKING) << "Cannot extract month/season from date" << formattedDate;
    }

    /// page field may start with "pp. ", remove that
    QString pages = PlainTextValue::text(entry->value(Entry::ftPages)).toLower();
    if (pages.startsWith(QStringLiteral("pp. "))) {
        pages = pages.mid(4);
        entry->remove(Entry::ftPages);
        Value v;
        v.append(QSharedPointer<PlainText>(new PlainText(pages)));
        entry->insert(Entry::ftPages, v);
    }

    /// Some entries may have no 'author' field, but a 'bookauthor' field instead
    if (!entry->contains(Entry::ftAuthor) && entry->contains(QStringLiteral("bookauthor"))) {
        Value v = entry->value(QStringLiteral("bookauthor"));
        entry->remove(QStringLiteral("bookauthor"));
        entry->insert(Entry::ftAuthor, v);
    }

    for (QMap<QString, Value>::Iterator it = entry->begin(); it != entry->end();) {
        if (PlainTextValue::text(it.value()).isEmpty())
            it = entry->erase(it);
        else
            ++it;
    }
}
