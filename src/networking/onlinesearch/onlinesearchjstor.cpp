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

#include <KLocale>
#include <KDebug>

#include "internalnetworkaccessmanager.h"
#include "iocommon.h"
#include "fileimporterbibtex.h"

class OnlineSearchJStor::OnlineSearchJStorPrivate
{
private:
    // UNUSED OnlineSearchJStor *p;

public:
    int numFoundResults, curStep, numSteps;
    static const QString jstorBaseUrl;
    KUrl queryUrl;

    OnlineSearchJStorPrivate(OnlineSearchJStor */* UNUSED parent*/)
    // : UNUSED p(parent)
    {
        /// nothing
    }
};

const QString OnlineSearchJStor::OnlineSearchJStorPrivate::jstorBaseUrl = QLatin1String("http://www.jstor.org/");

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
    d->numFoundResults = 0;

    /// Build search URL, to be used in the second step
    /// after fetching the start page
    d->queryUrl = KUrl(d->jstorBaseUrl);
    d->queryUrl.setPath("/action/doAdvancedSearch");
    d->queryUrl.addQueryItem("Search", "Search");
    d->queryUrl.addQueryItem("wc", "on"); /// include external references, too
    d->queryUrl.addQueryItem("la", ""); /// no specific language
    d->queryUrl.addQueryItem("jo", ""); /// no specific journal
    d->queryUrl.addQueryItem("Search", "Search");
    d->queryUrl.addQueryItem("hp", QString::number(numResults)); /// hits per page
    int queryNumber = 0;
    QStringList elements = splitRespectingQuotationMarks(query[queryKeyTitle]);
    foreach(const QString &element, elements) {
        if (queryNumber > 0) d->queryUrl.addQueryItem(QString(QLatin1String("c%1")).arg(queryNumber), "AND"); ///< join search terms with an AND operation
        d->queryUrl.addQueryItem(QString(QLatin1String("f%1")).arg(queryNumber), "ti");
        d->queryUrl.addQueryItem(QString(QLatin1String("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    elements = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    foreach(const QString &element, elements) {
        if (queryNumber > 0) d->queryUrl.addQueryItem(QString(QLatin1String("c%1")).arg(queryNumber), "AND"); ///< join search terms with an AND operation
        d->queryUrl.addQueryItem(QString(QLatin1String("f%1")).arg(queryNumber), "au");
        d->queryUrl.addQueryItem(QString(QLatin1String("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    elements = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    foreach(const QString &element, elements) {
        if (queryNumber > 0) d->queryUrl.addQueryItem(QString(QLatin1String("c%1")).arg(queryNumber), "AND"); ///< join search terms with an AND operation
        d->queryUrl.addQueryItem(QString(QLatin1String("f%1")).arg(queryNumber), "all");
        d->queryUrl.addQueryItem(QString(QLatin1String("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    if (!query[queryKeyYear].isEmpty()) {
        d->queryUrl.addQueryItem("sd", query[queryKeyYear]);
        d->queryUrl.addQueryItem("ed", query[queryKeyYear]);
    }

    QNetworkRequest request(d->jstorBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
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
    return QLatin1String("http://www.jstor.org/assets/search_20151218T0921/files/search/images/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchJStor::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchJStor::homepage() const
{
    return KUrl("http://www.jstor.org/");
}

void OnlineSearchJStor::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchJStor::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    KUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            ++d->numSteps;

            /// redirection to another url
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply->url());
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
        } else {
            QNetworkRequest request(d->queryUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchJStor::doneFetchingResultPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlText = QString::fromUtf8(reply->readAll().constData());

        /// extract all unique DOI from HTML code
        QSet<QString> uniqueDOIs;
        int p = -1;
        while ((p = KBibTeX::doiRegExp.indexIn(htmlText, p + 1)) >= 0)
            uniqueDOIs.insert(KBibTeX::doiRegExp.cap(0));

        if (uniqueDOIs.isEmpty()) {
            /// no results found
            emit progress(d->numSteps, d->numSteps);
            emit stoppedSearch(resultNoError);
        } else {
            /// Build search URL, to be used in the second step
            /// after fetching the start page
            KUrl bibTeXUrl = KUrl(d->jstorBaseUrl);
            bibTeXUrl.setPath("/action/downloadCitation");
            bibTeXUrl.addQueryItem("userAction", "export");
            bibTeXUrl.addQueryItem("format", "bibtex"); /// request BibTeX format
            bibTeXUrl.addQueryItem("include", "abs"); /// include abstracts
            foreach(const QString &doi, uniqueDOIs) {
                bibTeXUrl.addQueryItem("doi", doi);
            }

            QNetworkRequest request(bibTeXUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeXCode()));
        }
    } else
        kDebug() << "url was" << reply->url().toString();
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

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (publishEntry(entry))
                    ++d->numFoundResults;

            }
            delete bibtexFile;
        }

        emit progress(d->numSteps, d->numSteps);
        emit stoppedSearch(d->numFoundResults > 0 ? resultNoError : resultUnspecifiedError);
    } else
        kDebug() << "url was" << reply->url().toString();
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
    if (url.startsWith(QLatin1String("http://www.jstor.org/stable/"))) {
        /// use JSTOR's own stable ID for entry ID
        entry->setId("jstor" + url.mid(28).replace(QLatin1Char(','), QString()));
        /// store JSTOR's own stable ID
        Value v;
        v.append(QSharedPointer<VerbatimText>(new VerbatimText(url.mid(28))));
        entry->insert(QLatin1String("jstor_id"), v);
    }

    const QString formattedDateKey = QLatin1String("jstor_formatteddate");
    const QString formattedDate = PlainTextValue::text(entry->value(formattedDateKey));

    /// first, try to guess month by inspecting the beginning
    /// the jstor_formatteddate field
    const QString formattedDateLower = formattedDate.toLower();
    int i;
    for (i = 0; i < 12; ++i)
        if (formattedDateLower.startsWith(MonthsTriple[i])) break;
    entry->remove(formattedDateKey);
    if (i < 12) {
        Value v;
        v.append(QSharedPointer<MacroKey>(new MacroKey(MonthsTriple[i])));
        entry->insert(Entry::ftMonth, v);
    }
    /// guessing failed, therefore extract first part if it exists
    else if ((i = formattedDate.indexOf(",")) >= 0) {
        /// text may be a season ("Winter")
        Value v;
        v.append(QSharedPointer<PlainText>(new PlainText(formattedDate.left(i))));
        entry->insert(Entry::ftMonth, v);
    } else {
        /// this case happens if the field only contains a year
        kDebug() << "Cannot extract month/season from date" << formattedDate;
    }

    /// page field may start with "pp. ", remove that
    QString pages = PlainTextValue::text(entry->value(Entry::ftPages)).toLower();
    if (pages.startsWith(QLatin1String("pp. "))) {
        pages = pages.mid(4);
        entry->remove(Entry::ftPages);
        Value v;
        v.append(QSharedPointer<PlainText>(new PlainText(pages)));
        entry->insert(Entry::ftPages, v);
    }

    for (QMap<QString, Value>::Iterator it = entry->begin(); it != entry->end();) {
        if (PlainTextValue::text(it.value()).isEmpty())
            it = entry->erase(it);
        else
            ++it;
    }
}

