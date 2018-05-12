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

#include "onlinesearchideasrepec.h"

#include <QSet>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchIDEASRePEc::OnlineSearchIDEASRePEcPrivate
{
public:
    QSet<const QUrl> publicationLinks;

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QString urlBase = QStringLiteral("https://ideas.repec.org/cgi-bin/htsearch?cmd=Search%21&form=extended&m=all&fmt=url&wm=wrd&sp=1&sy=1&dt=range");

        bool hasFreeText = !query[queryKeyFreeText].isEmpty();
        bool hasTitle = !query[queryKeyTitle].isEmpty();
        bool hasAuthor = !query[queryKeyAuthor].isEmpty();
        bool hasYear = QRegularExpression(QStringLiteral("^(19|20)[0-9]{2}$")).match(query[queryKeyYear]).hasMatch();

        QString fieldWF = QStringLiteral("4BFF"); ///< search whole record by default
        QString fieldQ, fieldDB, fieldDE;
        if (hasAuthor && !hasFreeText && !hasTitle) {
            /// If only the author field is used, search explictly for author
            fieldWF = QStringLiteral("000F");
            fieldQ = query[queryKeyAuthor];
        } else if (!hasAuthor && !hasFreeText && hasTitle) {
            /// If only the title field is used, search explictly for title
            fieldWF = QStringLiteral("00F0");
            fieldQ = query[queryKeyTitle];
        } else {
            fieldQ = query[queryKeyFreeText] + QLatin1Char(' ') + query[queryKeyTitle] + QLatin1Char(' ') + query[queryKeyAuthor] + QLatin1Char(' ');
        }
        if (hasYear) {
            fieldDB = QStringLiteral("01/01/") + query[queryKeyYear];
            fieldDE = QStringLiteral("31/12/") + query[queryKeyYear];
        }

        QUrl url(urlBase);
        QUrlQuery q(url);
        q.addQueryItem(QStringLiteral("ps"), QString::number(numResults));
        q.addQueryItem(QStringLiteral("db"), fieldDB);
        q.addQueryItem(QStringLiteral("de"), fieldDE);
        q.addQueryItem(QStringLiteral("q"), fieldQ);
        q.addQueryItem(QStringLiteral("wf"), fieldWF);
        url.setQuery(q);

        return url;
    }

};

OnlineSearchIDEASRePEc::OnlineSearchIDEASRePEc(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIDEASRePEc::OnlineSearchIDEASRePEcPrivate())
{
    /// nothing
}

OnlineSearchIDEASRePEc::~OnlineSearchIDEASRePEc()
{
    delete d;
}

void OnlineSearchIDEASRePEc::startSearch(const QMap<QString, QString> &query, int numResults)
{
    const QUrl url = d->buildQueryUrl(query, numResults);
    emit progress(curStep = 0, numSteps = 2 * numResults + 1);
    m_hasBeenCanceled = false;

    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchIDEASRePEc::downloadListDone);

    refreshBusyProperty();
}


QString OnlineSearchIDEASRePEc::label() const
{
    return i18n("IDEAS (RePEc)");
}

QString OnlineSearchIDEASRePEc::favIconUrl() const
{
    return QStringLiteral("https://ideas.repec.org/favicon.ico");
}

QUrl OnlineSearchIDEASRePEc::homepage() const
{
    return QUrl(QStringLiteral("https://ideas.repec.org/"));
}

void OnlineSearchIDEASRePEc::downloadListDone()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchIDEASRePEc::downloadListDone);
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString htmlCode = QString::fromUtf8(reply->readAll().constData());

            const int ol1 = htmlCode.indexOf(QStringLiteral(" results for "));
            const int ol2 = htmlCode.indexOf(QStringLiteral("</ol>"), ol1 + 2);
            d->publicationLinks.clear();
            if (ol1 > 0 && ol2 > ol1) {
                const QString olHtmlCode = htmlCode.mid(ol1, ol2 - ol1 + 5);
                static const QRegularExpression publicationLinkRegExp(QStringLiteral("\"/[a-z](/[^\"]+){1,6}[.]html"));
                QRegularExpressionMatchIterator publicationLinkRegExpMatchIt = publicationLinkRegExp.globalMatch(olHtmlCode);
                while (publicationLinkRegExpMatchIt.hasNext()) {
                    const QRegularExpressionMatch publicationLinkRegExpMatch = publicationLinkRegExpMatchIt.next();
                    const QUrl c = reply->url().resolved(QUrl(publicationLinkRegExpMatch.captured().mid(1)));
                    d->publicationLinks.insert(c);
                }
                numSteps += 2 * d->publicationLinks.count(); ///< update number of steps
            }

            if (d->publicationLinks.isEmpty())
                stopSearch(resultNoError);
            else {
                QSet<const QUrl>::Iterator it = d->publicationLinks.begin();
                const QUrl publicationLink = *it;
                d->publicationLinks.erase(it);
                QNetworkRequest request = QNetworkRequest(publicationLink);
                reply = InternalNetworkAccessManager::instance().get(request, reply);
                InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
                connect(reply, &QNetworkReply::finished, this, &OnlineSearchIDEASRePEc::downloadPublicationDone);
            }
        }
    }

    refreshBusyProperty();
}

void OnlineSearchIDEASRePEc::downloadPublicationDone()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString htmlCode = QString::fromUtf8(reply->readAll().constData());

        QString downloadUrl;
        static const QString downloadFormStart = QStringLiteral("<FORM METHOD=GET ACTION=\"/cgi-bin/get_doc.pl\"");
        if (htmlCode.contains(downloadFormStart)) {
            QMap<QString, QString> form = formParameters(htmlCode, htmlCode.indexOf(downloadFormStart, Qt::CaseInsensitive));
            downloadUrl = form[QStringLiteral("url")];
        }

        QMap<QString, QString> form = formParameters(htmlCode, htmlCode.indexOf(QStringLiteral("<form method=\"post\" action=\"/cgi-bin/refs.cgi\""), Qt::CaseInsensitive));
        form[QStringLiteral("output")] = QStringLiteral("2"); ///< enforce BibTeX output

        QString body;
        QMap<QString, QString>::ConstIterator it = form.constBegin();
        while (it != form.constEnd()) {
            if (!body.isEmpty()) body += QLatin1Char('&');
            body += it.key() + QLatin1Char('=') + QString(QUrl::toPercentEncoding(it.value()));
            ++it;
        }

        const QUrl url = QUrl(QStringLiteral("https://ideas.repec.org/cgi-bin/refs.cgi"));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        reply = InternalNetworkAccessManager::instance().post(request, body.toUtf8());
        reply->setProperty("downloadurl", QVariant::fromValue<QString>(downloadUrl));
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchIDEASRePEc::downloadBibTeXDone);
    }

    refreshBusyProperty();
}

void OnlineSearchIDEASRePEc::downloadBibTeXDone()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    const QString downloadUrl = reply->property("downloadurl").toString();

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer(this);
            File *bibtexFile = importer.fromString(bibTeXcode);

            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                    if (!entry.isNull()) {
                        if (!downloadUrl.isEmpty()) {
                            /// There is an external document associated with this BibTeX entry
                            Value urlValue = entry->value(Entry::ftUrl);
                            urlValue.append(QSharedPointer<VerbatimText>(new VerbatimText(downloadUrl)));
                            entry->insert(Entry::ftUrl, urlValue);
                        }

                        Value v;
                        v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                        entry->insert(QStringLiteral("x-fetchedfrom"), v);
                        emit foundEntry(entry);
                    }

                }

                delete bibtexFile;
            }
        }

        if (d->publicationLinks.isEmpty())
            stopSearch(resultNoError);
        else {
            QSet<const QUrl>::Iterator it = d->publicationLinks.begin();
            const QUrl publicationLink = *it;
            d->publicationLinks.erase(it);
            QNetworkRequest request = QNetworkRequest(publicationLink);
            reply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchIDEASRePEc::downloadPublicationDone);
        }
    }

    refreshBusyProperty();
}
