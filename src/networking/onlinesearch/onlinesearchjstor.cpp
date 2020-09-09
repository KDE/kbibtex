/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifdef HAVE_WEBENGINEWIDGETS

#include "onlinesearchjstor.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QtWebEngineWidgets/QWebEnginePage>

#ifdef HAVE_KF5
#include <KLocalizedString>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchJStor::OnlineSearchJStorPrivate
{
public:
    int numExpectedResults;
    static const QString jstorBaseUrl;
    QUrl queryUrl;

    OnlineSearchJStorPrivate(OnlineSearchJStor *)
            : numExpectedResults(0)
    {
        /// nothing
    }
};

const QString OnlineSearchJStor::OnlineSearchJStorPrivate::jstorBaseUrl = QStringLiteral("https://www.jstor.org/");

OnlineSearchJStor::OnlineSearchJStor(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchJStorPrivate(this))
{
    /// nothing
}

OnlineSearchJStor::~OnlineSearchJStor()
{
    delete d;
}

void OnlineSearchJStor::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 2 + numResults);
    d->numExpectedResults = numResults;

    /// Build search URL, to be used in the second step
    /// after fetching the start page
    d->queryUrl = QUrl(OnlineSearchJStorPrivate::jstorBaseUrl);
    QUrlQuery q(d->queryUrl);
    d->queryUrl.setPath(QStringLiteral("/action/doAdvancedSearch"));
    q.addQueryItem(QStringLiteral("Search"), QStringLiteral("Search"));
    q.addQueryItem(QStringLiteral("acc"), QStringLiteral("off")); /// all content, not just what you can access
    q.addQueryItem(QStringLiteral("la"), QStringLiteral("")); /// no specific language
    q.addQueryItem(QStringLiteral("group"), QStringLiteral("none")); /// not sure what that means
    // TODO how to set number of results? 25 seems to be hard-coded standard
    // Unused query keys:
    //   'pt' for publication title
    //   'isbn' for ISBN
    int queryNumber = 0;
    const QStringList elementsTitle = splitRespectingQuotationMarks(query[QueryKey::Title]);
    for (const QString &element : elementsTitle) {
        if (queryNumber > 0) q.addQueryItem(QString(QStringLiteral("c%1")).arg(queryNumber), QStringLiteral("AND")); ///< join search terms with an AND operation
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(queryNumber), QStringLiteral("ti"));
        q.addQueryItem(QString(QStringLiteral("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    const QStringList elementsAuthor = splitRespectingQuotationMarks(query[QueryKey::Author]);
    for (const QString &element : elementsAuthor) {
        if (queryNumber > 0) q.addQueryItem(QString(QStringLiteral("c%1")).arg(queryNumber), QStringLiteral("AND")); ///< join search terms with an AND operation
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(queryNumber), QStringLiteral("au"));
        q.addQueryItem(QString(QStringLiteral("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    const QStringList elementsFreeText = splitRespectingQuotationMarks(query[QueryKey::FreeText]);
    for (const QString &element : elementsFreeText) {
        if (queryNumber > 0) q.addQueryItem(QString(QStringLiteral("c%1")).arg(queryNumber), QStringLiteral("AND")); ///< join search terms with an AND operation
        q.addQueryItem(QString(QStringLiteral("f%1")).arg(queryNumber), QStringLiteral("all"));
        q.addQueryItem(QString(QStringLiteral("q%1")).arg(queryNumber), element);
        ++queryNumber;
    }
    if (!query[QueryKey::Year].isEmpty()) {
        q.addQueryItem(QStringLiteral("sd"), query[QueryKey::Year]);
        q.addQueryItem(QStringLiteral("ed"), query[QueryKey::Year]);
    }
    d->queryUrl.setQuery(q);

    QNetworkRequest request(OnlineSearchJStorPrivate::jstorBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingStartPage);

    refreshBusyProperty();
}

QString OnlineSearchJStor::label() const
{
#ifdef HAVE_KF5
    return i18n("JSTOR");
#else // HAVE_KF5
    //= onlinesearch-jstor-label
    return QObject::tr("JSTOR");
#endif // HAVE_KF5
}

QUrl OnlineSearchJStor::homepage() const
{
    return QUrl(QStringLiteral("https://www.jstor.org/"));
}

void OnlineSearchJStor::doneFetchingStartPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            ++numSteps;

            /// redirection to another url
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply->url());
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingStartPage);
        } else {
            QNetworkRequest request(d->queryUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingResultPage);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchJStor::doneFetchingResultPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlText = QString::fromUtf8(reply->readAll().constData());
        QWebEnginePage *webPage = new QWebEnginePage(parent());
        connect(webPage, &QWebEnginePage::loadFinished, parent(), [this, webPage](bool ok) mutable {
            if (ok) {
                QTimer::singleShot(4096, parent(), [this, webPage]() {
                    QSharedPointer<QStringList> uniqueJStorIdentifiers(new QStringList);
                    webPage->toHtml([this, uniqueJStorIdentifiers](const QString & htmlText) mutable {
                        if (htmlText.length() == 0) return;

                        /// Extract all unique identifiers from HTML code
                        static const QRegularExpression jstorIdentifierRegExp(QStringLiteral("/stable/(pdf/|10[.][^/]+/)*([^\\\"?]+?)(\\.pdf)?[\\\"?]"));
                        QRegularExpressionMatchIterator jstorIdentifierRegExpMatchIt = jstorIdentifierRegExp.globalMatch(htmlText);
                        while (uniqueJStorIdentifiers->count() < d->numExpectedResults && jstorIdentifierRegExpMatchIt.hasNext()) {
                            const QRegularExpressionMatch jstorIdentifierRegExpMatch = jstorIdentifierRegExpMatchIt.next();
                            const QString jstorIdentifier = jstorIdentifierRegExpMatch.captured(2);
                            if (!uniqueJStorIdentifiers->contains(jstorIdentifier)) {
                                uniqueJStorIdentifiers->append(jstorIdentifier);
                            }
                        }
                    });

                    QTimer::singleShot(1024, parent(), [this, uniqueJStorIdentifiers, webPage]() {
                        if (uniqueJStorIdentifiers->isEmpty()) {
                            /// No results found
                            stopSearch(resultNoError);
                            refreshBusyProperty();
                        } else {
                            for (const QString &jstorIdentifier : const_cast<const QStringList &>(*uniqueJStorIdentifiers)) {
                                const QString url = QStringLiteral("https://www.jstor.org/citation/text/10.2307/") + jstorIdentifier;
                                QNetworkRequest request(QUrl::fromUserInput(url));
                                QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
                                InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
                                connect(newReply, &QNetworkReply::finished, this, &OnlineSearchJStor::doneFetchingBibTeXCode);
                            }
                        }
                        webPage->deleteLater();
                    });
                });
            } else {
                stopSearch(resultUnspecifiedError);
                webPage->deleteLater();
            }
        });
        webPage->setHtml(htmlText, reply->url());
    }

    refreshBusyProperty();
}

void OnlineSearchJStor::doneFetchingBibTeXCode()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        FileImporterBibTeX importer(this);
        File *bibtexFile = importer.fromString(bibTeXcode);
        int numFoundResults = 0;
        if (bibtexFile != nullptr) {
            for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                if (publishEntry(entry))
                    ++numFoundResults;
            }
            delete bibtexFile;
        }

        if (curStep == numSteps)
            stopSearch(numFoundResults > 0 ? resultNoError : resultUnspecifiedError);
    }

    refreshBusyProperty();
}

void OnlineSearchJStor::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

    const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(entry->id());
    if (doiRegExpMatch.hasMatch()) {
        /// entry ID is a DOI
        Value v;
        v.append(QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured(0))));
        entry->insert(Entry::ftDOI, v);
    }

    QString url = PlainTextValue::text(entry->value(Entry::ftUrl));
    if (url.startsWith(QStringLiteral("https://www.jstor.org/stable/"))) {
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

#endif // HAVE_WEBENGINEWIDGETS
