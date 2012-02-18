/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QSpinBox>
#include <QLayout>
#include <QLabel>
#include <QFormLayout>
#include <QNetworkReply>
#include <QNetworkCookieJar>

#include <KLocale>
#include <KMessageBox>
#include <KDebug>
#include <KConfigGroup>
#include <KLineEdit>
#include <KIcon>

#include <fileimporterbibtex.h>
#include <internalnetworkaccessmanager.h>
#include "onlinesearchgooglescholar.h"


class OnlineSearchGoogleScholar::OnlineSearchGoogleScholarPrivate
{
private:
    OnlineSearchGoogleScholar *p;

public:
    int numResults;
    QStringList listBibTeXurls;
    QString queryFreetext, queryAuthor, queryYear;
    QString startPageUrl;
    QString advancedSearchPageUrl;
    QString configPageUrl;
    QString setConfigPageUrl;
    QString queryPageUrl;
    FileImporterBibTeX importer;
    int numSteps, curStep;

    OnlineSearchGoogleScholarPrivate(OnlineSearchGoogleScholar *parent)
            : p(parent) {
        startPageUrl = QLatin1String("http://scholar.google.com/");
        configPageUrl = QLatin1String("http://%1/scholar_preferences");
        setConfigPageUrl = QLatin1String("http://%1/scholar_setprefs");
        queryPageUrl = QLatin1String("http://%1/scholar");
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
    emit stoppedSearch(resultNoError);
}

void OnlineSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->numResults = numResults;
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = numResults + 4;

    QStringList queryFragments;
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyFreeText])) {
        queryFragments.append(encodeURL(queryFragment));
    }
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyTitle])) {
        queryFragments.append(encodeURL(queryFragment));
    }
    d->queryFreetext = queryFragments.join("+");
    queryFragments.clear();
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyAuthor])) {
        queryFragments.append(encodeURL(queryFragment));
    }
    d->queryAuthor = queryFragments.join("+");
    d->queryYear = encodeURL(query[queryKeyYear]);

    KUrl url(d->startPageUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));

    emit progress(0, d->numSteps);
}

void OnlineSearchGoogleScholar::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = formParameters(reply->readAll(), "<form ");
        inputMap["hl"] = "en";

        KUrl url(d->configPageUrl.arg(reply->url().host()));
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());

        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingConfigPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingConfigPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = formParameters(reply->readAll(), "<form ");
        inputMap["hl"] = "en";
        inputMap["scis"] = "yes";
        inputMap["scisf"] = "4";
        inputMap["num"] = QString::number(d->numResults);

        KUrl url(d->setConfigPageUrl.arg(reply->url().host()));
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());

        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSetConfigPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingSetConfigPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = formParameters(reply->readAll(), "<form ");
        QStringList dummyArguments = QStringList() << "as_epq" << "as_oq" << "as_eq" << "as_occt" << "as_publication" << "as_sdtf";
        foreach(QString dummyArgument, dummyArguments) {
            inputMap[dummyArgument] = "";
        }
        inputMap["hl"] = "en";
        inputMap["num"] = QString::number(d->numResults);

        KUrl url(QString(d->queryPageUrl).arg(reply->url().host()));
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());
        url.addEncodedQueryItem(QString("as_q").toAscii(), d->queryFreetext.toAscii());
        url.addEncodedQueryItem(QString("as_sauthors").toAscii(), d->queryAuthor.toAscii());
        url.addEncodedQueryItem(QString("as_ylo").toAscii(), d->queryYear.toAscii());
        url.addEncodedQueryItem(QString("as_yhi").toAscii(), d->queryYear.toAscii());
        url.addQueryItem("btnG", "Search Scholar");

        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingQueryPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingQueryPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString htmlText = reply->readAll();

        static const QRegExp linkToBib("/scholar.bib\\?[^\" >]+");
        int pos = 0;
        d->listBibTeXurls.clear();
        while ((pos = linkToBib.indexIn(htmlText, pos)) != -1) {
            d->listBibTeXurls << "http://" + reply->url().host() + linkToBib.cap(0).replace("&amp;", "&");
            pos += linkToBib.matchedLength();
        }

        if (!d->listBibTeXurls.isEmpty()) {
            QNetworkRequest request(d->listBibTeXurls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->listBibTeXurls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchGoogleScholar::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString rawText = reply->readAll();
        File *bibtexFile = d->importer.fromString(rawText);

        bool hasEntry = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (!entry.isNull()) {
                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                    entry->insert("x-fetchedfrom", v);
                    emit foundEntry(entry);
                    hasEntry = true;
                }
            }
            delete bibtexFile;
        }

        if (!hasEntry) {
            kWarning() << "Searching" << label() << "resulted in invalid BibTeX data:" << QString(reply->readAll());
            emit stoppedSearch(resultUnspecifiedError);
            return;
        }

        if (!d->listBibTeXurls.isEmpty()) {
            QNetworkRequest request(d->listBibTeXurls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->listBibTeXurls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

QString OnlineSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

QString OnlineSearchGoogleScholar::favIconUrl() const
{
    return QLatin1String("http://scholar.google.com/favicon.ico");
}

OnlineSearchQueryFormAbstract* OnlineSearchGoogleScholar::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchGoogleScholar::homepage() const
{
    return KUrl("http://scholar.google.com/");
}

void OnlineSearchGoogleScholar::cancel()
{
    OnlineSearchAbstract::cancel();
}
