/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchgooglescholar.h"

#include <QNetworkReply>
#include <QIcon>
#include <QUrlQuery>
#include <QRegularExpression>

#ifdef HAVE_KF5
#include <KLocalizedString>
#endif // HAVE_KF5

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchGoogleScholar::OnlineSearchGoogleScholarPrivate
{
public:
    int numResults;
    QMap<QString, QString> listBibTeXurls;
    QString queryFreetext, queryAuthor, queryYear;
    QString startPageUrl;
    QString advancedSearchPageUrl;
    QString queryPageUrl;
    FileImporterBibTeX *importer;

    OnlineSearchGoogleScholarPrivate(OnlineSearchGoogleScholar *parent)
            : numResults(0)
    {
        importer = new FileImporterBibTeX(parent);
        startPageUrl = QStringLiteral("http://scholar.google.com/");
        queryPageUrl = QStringLiteral("http://%1/scholar");
    }

    ~OnlineSearchGoogleScholarPrivate() {
        delete importer;
    }

    QString documentUrlForBibTeXEntry(const QString &htmlText, int bibLinkPos) {
        /// Regular expression to detect text of a link to a document
        static const QRegularExpression documentLinkIndicator(QStringLiteral("\\[(PDF|HTML)\\]"), QRegularExpression::CaseInsensitiveOption);

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

OnlineSearchGoogleScholar::OnlineSearchGoogleScholar(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchGoogleScholar::OnlineSearchGoogleScholarPrivate(this))
{
    /// nothing
}

OnlineSearchGoogleScholar::~OnlineSearchGoogleScholar()
{
    delete d;
}

void OnlineSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->numResults = numResults;
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = numResults + 4);

    const auto respectingQuotationMarksFreeText = splitRespectingQuotationMarks(query[queryKeyFreeText]);
    const auto respectingQuotationMarksTitle = splitRespectingQuotationMarks(query[queryKeyTitle]);
    QStringList queryFragments;
    queryFragments.reserve(respectingQuotationMarksFreeText.size() + respectingQuotationMarksTitle.size());
    for (const QString &queryFragment : respectingQuotationMarksFreeText) {
        queryFragments.append(encodeURL(queryFragment));
    }
    for (const QString &queryFragment : respectingQuotationMarksTitle) {
        queryFragments.append(encodeURL(queryFragment));
    }
    d->queryFreetext = queryFragments.join(QStringLiteral("+"));
    const auto respectingQuotationMarksAuthor = splitRespectingQuotationMarks(query[queryKeyAuthor]);
    queryFragments.clear();
    queryFragments.reserve(respectingQuotationMarksAuthor.size());
    for (const QString &queryFragment : respectingQuotationMarksAuthor) {
        queryFragments.append(encodeURL(queryFragment));
    }
    d->queryAuthor = queryFragments.join(QStringLiteral("+"));
    d->queryYear = encodeURL(query[queryKeyYear]);

    QUrl url(d->startPageUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingStartPage);

    refreshBusyProperty();
}

void OnlineSearchGoogleScholar::doneFetchingStartPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl newDomainUrl;
    if (handleErrors(reply, newDomainUrl)) {
        if (newDomainUrl.isValid() && newDomainUrl != reply->url()) {
            /// following redirection to country-specific domain
            ++numSteps;
            QNetworkRequest request(newDomainUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingStartPage);
        } else {
            /// landed on country-specific domain
            static const QRegularExpression pathToSettingsPage(QStringLiteral(" href=\"(/scholar_settings[^ \"]*)"));
            const QString htmlCode = QString::fromUtf8(reply->readAll());
            // dumpToFile(QStringLiteral("01-doneFetchingStartPage.html"),htmlCode);
            const QRegularExpressionMatch pathToSettingsPageMatch = pathToSettingsPage.match(htmlCode);
            if (!pathToSettingsPageMatch.hasMatch() || pathToSettingsPageMatch.captured(1).isEmpty()) {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No link to Google Scholar settings found";
                stopSearch(resultNoError);
                return;
            }

            QUrl url = reply->url().resolved(QUrl(decodeURL(pathToSettingsPageMatch.captured(1))));
            QUrlQuery query(url);
            query.removeQueryItem(QStringLiteral("hl"));
            query.addQueryItem(QStringLiteral("hl"), QStringLiteral("en"));
            query.removeQueryItem(QStringLiteral("as_sdt"));
            query.addQueryItem(QStringLiteral("as_sdt"), QStringLiteral("0,5"));
            url.setQuery(query);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply->url());
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingConfigPage);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchGoogleScholar::doneFetchingConfigPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// Redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingConfigPage);
        } else {
            const QString htmlText = QString::fromUtf8(reply->readAll().constData());
            // dumpToFile(QStringLiteral("02-doneFetchingConfigPage.html"), htmlText);
            static const QRegularExpression formOpeningTag(QStringLiteral("<form [^>]+action=\"([^\"]*scholar_setprefs[^\"]*)"));
            const QRegularExpressionMatch formOpeningTagMatch = formOpeningTag.match(htmlText);
            const int formOpeningTagPos = formOpeningTagMatch.capturedStart(0);
            if (formOpeningTagPos < 0) {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not find opening tag for form:" << formOpeningTag.pattern();
                stopSearch(resultNoError);
                return;
            }

            QMap<QString, QString> inputMap = formParameters(htmlText, formOpeningTagPos);
            inputMap[QStringLiteral("hl")] = QStringLiteral("en");
            inputMap[QStringLiteral("scis")] = QStringLiteral("yes");
            inputMap[QStringLiteral("scisf")] = QStringLiteral("4");
            inputMap[QStringLiteral("num")] = QString::number(d->numResults);
            inputMap[QStringLiteral("submit")] = QStringLiteral("");

            QUrl url = reply->url().resolved(QUrl(decodeURL(formOpeningTagMatch.captured(1))));
            QUrlQuery query(url);
            for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it) {
                query.removeQueryItem(it.key());
                query.addQueryItem(it.key(), it.value());
            }
            url.setQuery(query);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingSetConfigPage);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchGoogleScholar::doneFetchingSetConfigPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// Redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingSetConfigPage);
        } else {
            // const QString htmlText = QString::fromUtf8(reply->readAll().constData());
            // dumpToFile(QStringLiteral("03-doneFetchingSetConfigPage.html"), htmlText);

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
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingQueryPage);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchGoogleScholar::doneFetchingQueryPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// Redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingQueryPage);
        } else {
            const QString htmlText = QString::fromUtf8(reply->readAll().constData());
            // dumpToFile(QStringLiteral("04-doneFetchingQueryPage.html"), htmlText);

            d->listBibTeXurls.clear();

#ifdef HAVE_KF5
            if (htmlText.contains(QStringLiteral("enable JavaScript")) || htmlText.contains(QStringLiteral("re not a robot"))) {
                sendVisualNotification(i18n("'Google Scholar' denied scrapping data because it thinks you are a robot."), label(), QStringLiteral("kbibtex"), 7 * 1000);
            } else {
#endif // HAVE_KF5
                static const QRegExp linkToBib("/scholar.bib\\?[^\" >]+");
                int pos = 0;
                while ((pos = linkToBib.indexIn(htmlText, pos)) != -1) {
                    /// Try to figure out [PDF] or [HTML] link associated with BibTeX entry
                    const QString documentUrl = d->documentUrlForBibTeXEntry(htmlText, pos);
                    /// Extract primary link associated with BibTeX entry
                    const QString primaryUrl = d->mainUrlForBibTeXEntry(htmlText, pos);

                    const QString bibtexUrl("https://" + reply->url().host() + linkToBib.cap(0).replace(QStringLiteral("&amp;"), QStringLiteral("&")));
                    d->listBibTeXurls.insert(bibtexUrl, primaryUrl + QLatin1Char('|') + documentUrl);
                    pos += linkToBib.matchedLength();
                }
#ifdef HAVE_KF5
            }
#endif // HAVE_KF5

            if (!d->listBibTeXurls.isEmpty()) {
                const QString bibtexUrl = d->listBibTeXurls.constBegin().key();
                const QStringList urls = d->listBibTeXurls.constBegin().value().split(QStringLiteral("|"), QString::KeepEmptyParts);
                const QString primaryUrl = urls.first();
                const QString documentUrl = urls.last();
                QNetworkRequest request(bibtexUrl);
                QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
                if (!primaryUrl.isEmpty()) {
                    /// Store primary URL as a property of the request/reply
                    newReply->setProperty("primaryurl", QVariant::fromValue<QString>(primaryUrl));
                }
                if (!documentUrl.isEmpty()) {
                    /// Store URL to document as a property of the request/reply
                    newReply->setProperty("documenturl", QVariant::fromValue<QString>(documentUrl));
                }
                InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
                connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingBibTeX);
                d->listBibTeXurls.erase(d->listBibTeXurls.begin());
            } else
                stopSearch(resultNoError);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchGoogleScholar::doneFetchingBibTeX()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    /// Extract previously stored URLs from reply
    const QString primaryUrl = reply->property("primaryurl").toString();
    const QString documentUrl = reply->property("documenturl").toString();

    QUrl newDomainUrl;
    if (handleErrors(reply, newDomainUrl)) {
        if (newDomainUrl.isValid() && newDomainUrl != reply->url()) {
            /// following redirection to country-specific domain
            ++numSteps;
            QNetworkRequest request(newDomainUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingBibTeX);
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString rawText = QString::fromUtf8(reply->readAll());
            // dumpToFile(QStringLiteral("05-doneFetchingBibTeX.bib"),rawText);
            File *bibtexFile = d->importer->fromString(rawText);

            bool hasEntry = false;
            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
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
                stopSearch(resultUnspecifiedError);
            } else if (!d->listBibTeXurls.isEmpty()) {
                const QString bibtexUrl = d->listBibTeXurls.constBegin().key();
                const QStringList urls = d->listBibTeXurls.constBegin().value().split(QStringLiteral("|"), QString::KeepEmptyParts);
                const QString primaryUrl = urls.first();
                const QString documentUrl = urls.last();
                QNetworkRequest request(bibtexUrl);
                QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
                InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
                if (!primaryUrl.isEmpty()) {
                    /// Store primary URL as a property of the request/reply
                    newReply->setProperty("primaryurl", QVariant::fromValue<QString>(primaryUrl));
                }
                if (!documentUrl.isEmpty()) {
                    /// Store URL to document as a property of the request/reply
                    newReply->setProperty("documenturl", QVariant::fromValue<QString>(documentUrl));
                }
                connect(newReply, &QNetworkReply::finished, this, &OnlineSearchGoogleScholar::doneFetchingBibTeX);
                d->listBibTeXurls.erase(d->listBibTeXurls.begin());
            } else
                stopSearch(resultNoError);
        }
    }

    refreshBusyProperty();
}

QString OnlineSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

QString OnlineSearchGoogleScholar::favIconUrl() const
{
    return QStringLiteral("https://scholar.google.com/favicon-png.ico");
}

QUrl OnlineSearchGoogleScholar::homepage() const
{
    return QUrl(QStringLiteral("https://scholar.google.com/"));
}
