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

#include "onlinesearchgooglescholar.h"

#include <QSpinBox>
#include <QLayout>
#include <QLabel>
#include <QFormLayout>
#include <QNetworkReply>
#include <QIcon>
#include <QUrlQuery>

#include <KLocalizedString>
#include <KMessageBox>
#include <KConfigGroup>
#include <KLineEdit>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchGoogleScholar::OnlineSearchGoogleScholarPrivate
{
private:
    // UNUSED OnlineSearchGoogleScholar *p;

public:
    int numResults;
    QMap<QString, QString> listBibTeXurls;
    QString queryFreetext, queryAuthor, queryYear;
    QString startPageUrl;
    QString advancedSearchPageUrl;
    QString configPageUrl;
    QString setConfigPageUrl;
    QString queryPageUrl;
    FileImporterBibTeX importer;
    int numSteps, curStep;

    OnlineSearchGoogleScholarPrivate(OnlineSearchGoogleScholar */* UNUSED parent*/)
        : /* UNUSED p(parent), */ numResults(0), numSteps(0), curStep(0)
    {
        startPageUrl = QStringLiteral("http://scholar.google.com/");
        configPageUrl = QStringLiteral("http://%1/scholar_settings");
        setConfigPageUrl = QStringLiteral("http://%1/scholar_setprefs");
        queryPageUrl = QStringLiteral("http://%1/scholar");
    }

    QString documentUrlForBibTeXEntry(const QString &htmlText, int bibLinkPos) {
        /// Regular expression to detect text of a link to a document
        static const QRegExp documentLinkIndicator(QStringLiteral("\\[(PDF|HTML)\\]"), Qt::CaseSensitive);

        /// Text for link is *before* the BibTeX link in Google's HTML code
        int posDocumentLinkText = htmlText.lastIndexOf(documentLinkIndicator, bibLinkPos);
        /// Check position of previous BibTeX link to not extract the wrong document link
        int posPreviousBib = htmlText.lastIndexOf(QStringLiteral("/scholar.bib"), bibLinkPos - 3);
        if (posPreviousBib < 0) posPreviousBib = 0; /// no previous BibTeX entry?

        /// If all found position values look reasonable ...
        if (posDocumentLinkText > posPreviousBib) {
            /// There is a [PDF] or [HTML] link for this BibTeX entry, so find URL
            /// Variables p1 and p2 are used to close in to the document's URL
            int p1 = htmlText.lastIndexOf(QStringLiteral("<a "), posDocumentLinkText);
            if (p1 > 0) {
                p1 = htmlText.indexOf(QStringLiteral("href=\""), p1);
                if (p1 > 0) {
                    int p2 = htmlText.indexOf(QLatin1Char('"'), p1 + 7);
                    if (p2 > 0)
                        return htmlText.mid(p1 + 6, p2 - p1 - 6).replace(QStringLiteral("&amp;"), QStringLiteral("&"));
                }
            }
        }

        return QString();
    }

    QString mainUrlForBibTeXEntry(const QString &htmlText, int bibLinkPos) {
        /// Text for link is *before* the BibTeX link in Google's HTML code
        int posH3 = htmlText.lastIndexOf(QStringLiteral("<h3 "), bibLinkPos);
        /// Check position of previous BibTeX link to not extract the wrong document link
        int posPreviousBib = htmlText.lastIndexOf(QStringLiteral("/scholar.bib"), bibLinkPos - 3);
        if (posPreviousBib < 0) posPreviousBib = 0; /// no previous BibTeX entry?

        /// If all found position values look reasonable ...
        if (posH3 > posPreviousBib) {
            /// There is a h3 tag for this BibTeX entry, so find URL
            /// Variables p1 and p2 are used to close in to the document's URL
            int p1 = htmlText.indexOf(QStringLiteral("href=\""), posH3);
            if (p1 > 0) {
                int p2 = htmlText.indexOf(QLatin1Char('"'), p1 + 7);
                if (p2 > 0)
                    return htmlText.mid(p1 + 6, p2 - p1 - 6).replace(QStringLiteral("&amp;"), QStringLiteral("&"));
            }
        }

        return QString();
    }
};

OnlineSearchGoogleScholar::OnlineSearchGoogleScholar(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchGoogleScholar::OnlineSearchGoogleScholarPrivate(this))
{
    // nothing
}

OnlineSearchGoogleScholar::~OnlineSearchGoogleScholar()
{
    delete d;
}

void OnlineSearchGoogleScholar::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->numResults = numResults;
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = numResults + 4;

    QStringList queryFragments;
    foreach (const QString & queryFragment, splitRespectingQuotationMarks(query[queryKeyFreeText])) {
        queryFragments.append(encodeURL(queryFragment));
    }
    foreach (const QString & queryFragment, splitRespectingQuotationMarks(query[queryKeyTitle])) {
        queryFragments.append(encodeURL(queryFragment));
    }
    d->queryFreetext = queryFragments.join(QStringLiteral("+"));
    queryFragments.clear();
    foreach (const QString & queryFragment, splitRespectingQuotationMarks(query[queryKeyAuthor])) {
        queryFragments.append(encodeURL(queryFragment));
    }
    d->queryAuthor = queryFragments.join(QStringLiteral("+"));
    d->queryYear = encodeURL(query[queryKeyYear]);

    QUrl url(d->startPageUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));

    emit progress(0, d->numSteps);
}

void OnlineSearchGoogleScholar::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl newDomainUrl;
    if (handleErrors(reply, newDomainUrl)) {
        if (newDomainUrl.isValid() && newDomainUrl != reply->url()) {
            /// following redirection to country-specific domain
            ++d->numSteps;
            QNetworkRequest request(newDomainUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
        } else {
            /// landed on country-specific domain
            QUrl url(d->configPageUrl.arg(reply->url().host()));
            QUrlQuery query(url);
            query.addQueryItem(QStringLiteral("hl"), QStringLiteral("en"));
            query.addQueryItem(QStringLiteral("as_sdt"), QStringLiteral("0,5"));
            url.setQuery(query);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply->url());
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingConfigPage()));
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingConfigPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlText = QString::fromUtf8(reply->readAll().constData());
        QMap<QString, QString> inputMap = formParameters(htmlText, QStringLiteral("<form "));
        inputMap[QStringLiteral("hl")] = QStringLiteral("en");
        inputMap[QStringLiteral("scis")] = QStringLiteral("yes");
        inputMap[QStringLiteral("scisf")] = QStringLiteral("4");
        inputMap[QStringLiteral("num")] = QString::number(d->numResults);
        inputMap[QStringLiteral("submit")] = QStringLiteral("");

        QUrl url(d->setConfigPageUrl.arg(reply->url().host()));
        QUrlQuery query(url);
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            query.addQueryItem(it.key(), it.value());
        url.setQuery(query);

        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSetConfigPage()));
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingSetConfigPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QUrl url(QString(d->queryPageUrl).arg(reply->url().host()));
        QUrlQuery query(url);
        query.addQueryItem(QStringLiteral("as_q"), d->queryFreetext);
        query.addQueryItem(QStringLiteral("as_sauthors"), d->queryAuthor);
        query.addQueryItem(QStringLiteral("as_ylo"), d->queryYear);
        query.addQueryItem(QStringLiteral("as_yhi"), d->queryYear);
        query.addQueryItem(QStringLiteral("as_vis"), QStringLiteral("1")); ///< include citations
        query.addQueryItem(QStringLiteral("num"), QString::number(d->numResults));
        query.addQueryItem(QStringLiteral("btnG"), QStringLiteral("Search Scholar"));
        url.setQuery(query);

        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingQueryPage()));
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingQueryPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlText = QString::fromUtf8(reply->readAll().constData());

        static const QRegExp linkToBib("/scholar.bib\\?[^\" >]+");
        int pos = 0;
        d->listBibTeXurls.clear();
        while ((pos = linkToBib.indexIn(htmlText, pos)) != -1) {
            /// Try to figure out [PDF] or [HTML] link associated with BibTeX entry
            const QString documentUrl = d->documentUrlForBibTeXEntry(htmlText, pos);
            /// Extract primary link associated with BibTeX entry
            const QString primaryUrl = d->mainUrlForBibTeXEntry(htmlText, pos);

            const QString bibtexUrl("http://" + reply->url().host() + linkToBib.cap(0).replace(QStringLiteral("&amp;"), QStringLiteral("&")));
            d->listBibTeXurls.insert(bibtexUrl, primaryUrl + QLatin1Char('|') + documentUrl);
            pos += linkToBib.matchedLength();
        }

        if (!d->listBibTeXurls.isEmpty()) {
            const QString bibtexUrl = d->listBibTeXurls.constBegin().key();
            const QStringList urls = d->listBibTeXurls.constBegin().value().split(QStringLiteral("|"), QString::KeepEmptyParts);
            const QString primaryUrl = urls.first();
            const QString documentUrl = urls.last();
            QNetworkRequest request(bibtexUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            if (!primaryUrl.isEmpty()) {
                /// Store primary URL as a property of the request/reply
                newReply->setProperty("primaryurl", QVariant::fromValue<QString>(primaryUrl));
            }
            if (!documentUrl.isEmpty()) {
                /// Store URL to document as a property of the request/reply
                newReply->setProperty("documenturl", QVariant::fromValue<QString>(documentUrl));
            }
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->listBibTeXurls.erase(d->listBibTeXurls.begin());
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    /// Extract previously stored URLs from reply
    const QString primaryUrl = reply->property("primaryurl").toString();
    const QString documentUrl = reply->property("documenturl").toString();

    QUrl newDomainUrl;
    if (handleErrors(reply, newDomainUrl)) {
        if (newDomainUrl.isValid() && newDomainUrl != reply->url()) {
            /// following redirection to country-specific domain
            ++d->numSteps;
            QNetworkRequest request(newDomainUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
        } else {
            /// ensure proper treatment of UTF-8 characters
            QString rawText = QString::fromUtf8(reply->readAll().constData());
            File *bibtexFile = d->importer.fromString(rawText);

            bool hasEntry = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    if (!entry.isNull()) {
                        Value v;
                        v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                        entry->insert(QStringLiteral("x-fetchedfrom"), v);
                        if (!primaryUrl.isEmpty()) {
                            /// There is an external document associated with this BibTeX entry
                            Value urlValue = entry->value(Entry::ftUrl);
                            urlValue.append(QSharedPointer<VerbatimText>(new VerbatimText(primaryUrl)));
                            entry->insert(Entry::ftUrl, urlValue);
                        }
                        if (!documentUrl.isEmpty() &&
                                primaryUrl != documentUrl /** avoid duplicates */) {
                            /// There is a web page associated with this BibTeX entry
                            Value urlValue = entry->value(Entry::ftUrl);
                            urlValue.append(QSharedPointer<VerbatimText>(new VerbatimText(documentUrl)));
                            entry->insert(Entry::ftUrl, urlValue);
                        }
                        emit foundEntry(entry);
                        hasEntry = true;
                    }
                }
                delete bibtexFile;
            }

            if (!hasEntry) {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Searching" << label() << "resulted in invalid BibTeX data:" << rawText;
                emit stoppedSearch(resultUnspecifiedError);
                return;
            }

            if (!d->listBibTeXurls.isEmpty()) {
                const QString bibtexUrl = d->listBibTeXurls.constBegin().key();
                const QStringList urls = d->listBibTeXurls.constBegin().value().split(QStringLiteral("|"), QString::KeepEmptyParts);
                const QString primaryUrl = urls.first();
                const QString documentUrl = urls.last();
                QNetworkRequest request(bibtexUrl);
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
                if (!primaryUrl.isEmpty()) {
                    /// Store primary URL as a property of the request/reply
                    newReply->setProperty("primaryurl", QVariant::fromValue<QString>(primaryUrl));
                }
                if (!documentUrl.isEmpty()) {
                    /// Store URL to document as a property of the request/reply
                    newReply->setProperty("documenturl", QVariant::fromValue<QString>(documentUrl));
                }
                connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
                d->listBibTeXurls.erase(d->listBibTeXurls.begin());
            } else {
                emit stoppedSearch(resultNoError);
                emit progress(d->numSteps, d->numSteps);
            }
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

QString OnlineSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

QString OnlineSearchGoogleScholar::favIconUrl() const
{
    return QStringLiteral("http://scholar.google.com/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchGoogleScholar::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchGoogleScholar::homepage() const
{
    return QUrl(QStringLiteral("http://scholar.google.com/"));
}

void OnlineSearchGoogleScholar::cancel()
{
    OnlineSearchAbstract::cancel();
}
