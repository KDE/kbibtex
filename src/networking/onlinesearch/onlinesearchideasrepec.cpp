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

#include "onlinesearchideasrepec.h"

#include <QSet>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <KLocale>
#include <KDebug>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"

class OnlineSearchIDEASRePEc::OnlineSearchIDEASRePEcPrivate
{
public:
    int numSteps, curStep;
    QSet<QString> publicationLinks;

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QString urlBase = QLatin1String("https://ideas.repec.org/cgi-bin/htsearch?cmd=Search%21&form=extended&m=all&fmt=url&wm=wrd&sp=1&sy=1&dt=range");

        bool hasFreeText = !query[queryKeyFreeText].isEmpty();
        bool hasTitle = !query[queryKeyTitle].isEmpty();
        bool hasAuthor = !query[queryKeyAuthor].isEmpty();
        bool hasYear = QRegExp(QLatin1String("^(19|20)[0-9]{2}$")).indexIn(query[queryKeyYear]) == 0;

        QString fieldWF = QLatin1String("4BFF"); ///< search whole record by default
        QString fieldQ, fieldDB, fieldDE;
        if (hasAuthor && !hasFreeText && !hasTitle) {
            /// If only the author field is used, search explictly for author
            fieldWF = QLatin1String("000F");
            fieldQ = query[queryKeyAuthor];
        } else if (!hasAuthor && !hasFreeText && hasTitle) {
            /// If only the title field is used, search explictly for title
            fieldWF = QLatin1String("00F0");
            fieldQ = query[queryKeyTitle];
        } else {
            fieldQ = query[queryKeyFreeText] + QLatin1Char(' ') + query[queryKeyTitle] + QLatin1Char(' ') + query[queryKeyAuthor] + QLatin1Char(' ');
        }
        if (hasYear) {
            fieldDB = QLatin1String("01/01/") + query[queryKeyYear];
            fieldDE = QLatin1String("31/12/") + query[queryKeyYear];
        }

        KUrl url(urlBase);
        url.addQueryItem(QLatin1String("ps"), QString::number(numResults));
        url.addQueryItem(QLatin1String("db"), fieldDB);
        url.addQueryItem(QLatin1String("de"), fieldDE);
        url.addQueryItem(QLatin1String("q"), fieldQ);
        url.addQueryItem(QLatin1String("wf"), fieldWF);

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

void OnlineSearchIDEASRePEc::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchIDEASRePEc::startSearch(const QMap<QString, QString> &query, int numResults)
{
    const KUrl url = d->buildQueryUrl(query, numResults);
    d->curStep = 0;
    d->numSteps = 2 * numResults + 1;
    m_hasBeenCanceled = false;

    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadListDone()));

    emit progress(0, d->numSteps);
}


QString OnlineSearchIDEASRePEc::label() const
{
    return i18n("IDEAS (RePEc)");
}

QString OnlineSearchIDEASRePEc::favIconUrl() const
{
    return QLatin1String("https://ideas.repec.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIDEASRePEc::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchIDEASRePEc::homepage() const
{
    return KUrl("https://ideas.repec.org/");
}

void OnlineSearchIDEASRePEc::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchIDEASRePEc::downloadListDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    KUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(downloadListDone()));
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString htmlCode = QString::fromUtf8(reply->readAll().constData());

            static const QRegExp publicationLinkRegExp(QLatin1String("http[s]?://ideas.repec.org/[a-z]/\\S{,8}/\\S{2,24}/\\S{,64}.html"));
            d->publicationLinks.clear();
            int p = -1;
            while ((p = publicationLinkRegExp.indexIn(htmlCode, p + 1)) >= 0) {
                QString c = publicationLinkRegExp.cap(0);
                /// Rewrite URL to be https instead of http, avoids HTTP redirection
                c = c.replace(QLatin1String("http://"), QLatin1String("https://"));
                d->publicationLinks.insert(c);
            }
            d->numSteps += 2 * d->publicationLinks.count(); ///< update number of steps

            if (d->publicationLinks.isEmpty()) {
                kDebug() << "No publication links found";
                emit stoppedSearch(resultNoError);
                emit progress(1, 1);
            } else {
                QSet<QString>::Iterator it = d->publicationLinks.begin();
                const QString publicationLink = *it;
                d->publicationLinks.erase(it);
                QNetworkRequest request = QNetworkRequest(QUrl(publicationLink));
                reply = InternalNetworkAccessManager::self()->get(request, reply);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
                connect(reply, SIGNAL(finished()), this, SLOT(downloadPublicationDone()));
            }
        }
    } else
        kDebug() << "url was" << reply->url().toString();

}

void OnlineSearchIDEASRePEc::downloadPublicationDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString htmlCode = QString::fromUtf8(reply->readAll().constData());

        QString downloadUrl;
        static const QString downloadFormStart = QLatin1String("<FORM METHOD=GET ACTION=\"/cgi-bin/get_doc.pl\"");
        if (htmlCode.contains(downloadFormStart)) {
            QMap<QString, QString> form = formParameters(htmlCode, htmlCode.indexOf(downloadFormStart, Qt::CaseInsensitive));
            downloadUrl = form[QLatin1String("url")];
        }

        QMap<QString, QString> form = formParameters(htmlCode, htmlCode.indexOf(QLatin1String("<form method=\"post\" action=\"/cgi-bin/refs.cgi\""), Qt::CaseInsensitive));
        form[QLatin1String("output")] = QLatin1String("2"); ///< enforce BibTeX output

        QString body;
        QMap<QString, QString>::ConstIterator it = form.constBegin();
        while (it != form.constEnd()) {
            if (!body.isEmpty()) body += QLatin1Char('&');
            body += it.key() + QLatin1Char('=') + QString(QUrl::toPercentEncoding(it.value()));
            ++it;
        }

        const KUrl url = KUrl(QLatin1String("https://ideas.repec.org/cgi-bin/refs.cgi"));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        reply = InternalNetworkAccessManager::self()->post(request, body.toUtf8());
        reply->setProperty("downloadurl", QVariant::fromValue<QString>(downloadUrl));
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(downloadBibTeXDone()));

    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchIDEASRePEc::downloadBibTeXDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    const QString downloadUrl = reply->property("downloadurl").toString();

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    if (!entry.isNull()) {
                        if (!downloadUrl.isEmpty()) {
                            /// There is an external document associated with this BibTeX entry
                            Value urlValue = entry->value(Entry::ftUrl);
                            urlValue.append(QSharedPointer<VerbatimText>(new VerbatimText(downloadUrl)));
                            entry->insert(Entry::ftUrl, urlValue);
                        }

                        Value v;
                        v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                        entry->insert("x-fetchedfrom", v);
                        emit foundEntry(entry);
                        hasEntries = true;
                    }

                }

                if (!hasEntries)
                    kDebug() << "No hits found in" << reply->url().toString() << "(was" << downloadUrl << ")";

                delete bibtexFile;
            }
        }

        if (d->publicationLinks.isEmpty()) {
            emit stoppedSearch(resultNoError);
            emit progress(1, 1);
        } else {
            QSet<QString>::Iterator it = d->publicationLinks.begin();
            const QString publicationLink = *it;
            d->publicationLinks.erase(it);
            QNetworkRequest request = QNetworkRequest(QUrl(publicationLink));
            reply = InternalNetworkAccessManager::self()->get(request, reply);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, SIGNAL(finished()), this, SLOT(downloadPublicationDone()));
        }
    } else
        kDebug() << "url was" << reply->url().toString() << "(was" << downloadUrl << ")";
}
