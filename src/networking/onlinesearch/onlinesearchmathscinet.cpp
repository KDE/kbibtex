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

#include "onlinesearchmathscinet.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QMap>

#include <KLocale>
#include <KDebug>

#include "fileimporterbibtex.h"
#include "kbibtexnamespace.h"
#include "internalnetworkaccessmanager.h"

class OnlineSearchMathSciNet::OnlineSearchMathSciNetPrivate
{
private:
    // UNUSED OnlineSearchMathSciNet *p;

public:
    QMap<QString, QString> queryParameters;
    int numResults;

    static const QString queryFormUrl;
    static const QString queryUrlStem;

    OnlineSearchMathSciNetPrivate(OnlineSearchMathSciNet */* UNUSED parent*/)
    // : UNUSED p(parent)
    {
        /// nothing
    }
};

const QString OnlineSearchMathSciNet::OnlineSearchMathSciNetPrivate::queryFormUrl = QLatin1String("http://www.ams.org/mathscinet/");
const QString OnlineSearchMathSciNet::OnlineSearchMathSciNetPrivate::queryUrlStem = QLatin1String("http://www.ams.org/mathscinet/search/publications.html?client=KBibTeX");

OnlineSearchMathSciNet::OnlineSearchMathSciNet(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchMathSciNetPrivate(this))
{
    /// nothing
}

OnlineSearchMathSciNet::~OnlineSearchMathSciNet()
{
    delete d;
}

void OnlineSearchMathSciNet::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    d->queryParameters.clear();
    d->numResults = qMin(50, numResults); /// limit query to max 50 elements
    int index = 1;

    const QString freeText = query[queryKeyFreeText];
    QStringList elements = splitRespectingQuotationMarks(freeText);
    foreach(const QString &element, elements) {
        d->queryParameters.insert(QString(QLatin1String("pg%1")).arg(index), QLatin1String("ALLF"));
        d->queryParameters.insert(QString(QLatin1String("s%1")).arg(index), element);
        ++index;
    }

    const QString title = query[queryKeyTitle];
    elements = splitRespectingQuotationMarks(title);
    foreach(const QString &element, elements) {
        d->queryParameters.insert(QString(QLatin1String("pg%1")).arg(index), QLatin1String("TI"));
        d->queryParameters.insert(QString(QLatin1String("s%1")).arg(index), element);
        ++index;
    }

    const QString authors = query[queryKeyAuthor];
    elements = splitRespectingQuotationMarks(authors);
    foreach(const QString &element, elements) {
        d->queryParameters.insert(QString(QLatin1String("pg%1")).arg(index), QLatin1String("ICN"));
        d->queryParameters.insert(QString(QLatin1String("s%1")).arg(index), element);
        ++index;
    }

    const QString year = query[queryKeyYear];
    if (year.isEmpty()) {
        d->queryParameters.insert(QLatin1String("dr"), QLatin1String("all"));
    } else {
        d->queryParameters.insert(QLatin1String("dr"), QLatin1String("pubyear"));
        d->queryParameters.insert(QLatin1String("yrop"), QLatin1String("eq"));
        d->queryParameters.insert(QLatin1String("arg3"), year);
    }

    emit progress(0, 3);

    /// issue request for start page
    QNetworkRequest request(d->queryFormUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingQueryForm()));
}

void OnlineSearchMathSciNet::startSearch()
{
    d->queryParameters.clear();
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

QString OnlineSearchMathSciNet::label() const
{
    return i18n("MathSciNet");
}

QString OnlineSearchMathSciNet::favIconUrl() const
{
    return QLatin1String("http://www.ams.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchMathSciNet::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchMathSciNet::homepage() const
{
    return QUrl("http://www.ams.org/mathscinet/help/about.html");
}

void OnlineSearchMathSciNet::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchMathSciNet::doneFetchingQueryForm()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(1, 3);

    if (handleErrors(reply)) {
        // UNUSED const QString htmlText = QString::fromUtf8(reply->readAll().data());

        /// extract form's parameters ...
        QMap<QString, QString> formParams;
        /// ... and overwrite them with the query's parameters
        for (QMap<QString, QString>::ConstIterator it = d->queryParameters.constBegin(); it != d->queryParameters.constEnd(); ++it)
            formParams.insert(it.key(), it.value());

        /// build url by appending parameters
        QUrl url(d->queryUrlStem);
        for (QMap<QString, QString>::ConstIterator it = formParams.constBegin(); it != formParams.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());
        for (int i = 1; i <= d->queryParameters.count(); ++i)
            url.addQueryItem(QString(QLatin1String("co%1")).arg(i), QLatin1String("AND")); ///< join search terms with an AND operation

        /// issue request for result page
        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchMathSciNet::doneFetchingResultPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(2, 3);

    if (handleErrors(reply)) {
        const QString htmlText = QString::fromUtf8(reply->readAll().data());

        /// extract form's parameters ...
        QMap<QString, QString> formParams = formParameters(htmlText, QLatin1String("<form name=\"batchDownload\" action="));

        /// build url by appending parameters
        QUrl url(d->queryUrlStem);
        QStringList copyParameters = QStringList() << QLatin1String("foo") << QLatin1String("reqargs") << QLatin1String("batch_title");
        foreach(const QString &param, copyParameters)
        url.addQueryItem(param, formParams[param]);
        url.addQueryItem(QLatin1String("fmt"), QLatin1String("bibtex"));

        int p = -1, count = 0;
        static const QRegExp checkBoxRegExp(QLatin1String("<input class=\"hlCheckBox\" type=\"checkbox\" name=\"b\" value=\"(\\d+)\""));
        while (count < d->numResults && (p = checkBoxRegExp.indexIn(htmlText, p + 1)) >= 0) {
            url.addQueryItem(QLatin1String("b"), checkBoxRegExp.cap(1));
            ++count;
        }

        if (count > 0) {
            /// issue request for bibtex code
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeXcode()));
        } else {
            /// nothing found
            emit progress(3, 3);
            emit stoppedSearch(resultNoError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchMathSciNet::doneFetchingBibTeXcode()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(3, 3);

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlCode = QString::fromUtf8(reply->readAll().data());

        QString bibtexCode;
        int p1 = -1, p2 = -1;
        while ((p1 = htmlCode.indexOf(QLatin1String("<pre>"), p2 + 1)) >= 0 && (p2 = htmlCode.indexOf(QLatin1String("</pre>"), p1 + 1)) >= 0) {
            bibtexCode += htmlCode.mid(p1 + 5, p2 - p1 - 5) + QChar('\n');
        }

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibtexCode);

        bool hasEntry = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntry |= publishEntry(entry);

            }
            delete bibtexFile;
        }

        emit stoppedSearch(hasEntry ? resultNoError : resultUnspecifiedError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchMathSciNet::sanitizeEntry(QSharedPointer<Entry> entry)
{
    const QString ftFJournal = QLatin1String("fjournal");
    if (entry->contains(ftFJournal)) {
        Value v = entry->value(ftFJournal);
        entry->remove(Entry::ftJournal);
        entry->remove(ftFJournal);
        entry->insert(Entry::ftJournal, v);
    }
}
