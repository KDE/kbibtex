/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                 2018 Alexander Dunlap <alexander.dunlap@gmail.com>      *
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

#include "onlinesearchmathscinet.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QMap>
#include <QUrlQuery>
#include <QRegularExpression>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "kbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchMathSciNet::OnlineSearchMathSciNetPrivate
{
public:
    QMap<QString, QString> queryParameters;
    int numResults;

    static const QString queryFormUrl;
    static const QString queryUrlStem;

    OnlineSearchMathSciNetPrivate(OnlineSearchMathSciNet *parent)
            : numResults(0)
    {
        Q_UNUSED(parent)
    }
};

const QString OnlineSearchMathSciNet::OnlineSearchMathSciNetPrivate::queryFormUrl = QStringLiteral("https://mathscinet.ams.org/mathscinet/");
const QString OnlineSearchMathSciNet::OnlineSearchMathSciNetPrivate::queryUrlStem = QStringLiteral("https://mathscinet.ams.org/mathscinet/search/publications.html?client=KBibTeX");

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
    emit progress(curStep = 0, numSteps = 3);

    d->queryParameters.clear();
    d->numResults = qMin(50, numResults); /// limit query to max 50 elements
    int index = 1;

    const QString freeText = query[queryKeyFreeText];
    const QStringList elementsFreeText = splitRespectingQuotationMarks(freeText);
    for (const QString &element : elementsFreeText) {
        d->queryParameters.insert(QString(QStringLiteral("pg%1")).arg(index), QStringLiteral("ALLF"));
        d->queryParameters.insert(QString(QStringLiteral("s%1")).arg(index), element);
        ++index;
    }

    const QString title = query[queryKeyTitle];
    const QStringList elementsTitle = splitRespectingQuotationMarks(title);
    for (const QString &element : elementsTitle) {
        d->queryParameters.insert(QString(QStringLiteral("pg%1")).arg(index), QStringLiteral("TI"));
        d->queryParameters.insert(QString(QStringLiteral("s%1")).arg(index), element);
        ++index;
    }

    const QString authors = query[queryKeyAuthor];
    const QStringList elementsAuthor = splitRespectingQuotationMarks(authors);
    for (const QString &element : elementsAuthor) {
        d->queryParameters.insert(QString(QStringLiteral("pg%1")).arg(index), QStringLiteral("ICN"));
        d->queryParameters.insert(QString(QStringLiteral("s%1")).arg(index), element);
        ++index;
    }

    const QString year = query[queryKeyYear];
    if (year.isEmpty()) {
        d->queryParameters.insert(QStringLiteral("dr"), QStringLiteral("all"));
    } else {
        d->queryParameters.insert(QStringLiteral("dr"), QStringLiteral("pubyear"));
        d->queryParameters.insert(QStringLiteral("yrop"), QStringLiteral("eq"));
        d->queryParameters.insert(QStringLiteral("arg3"), year);
    }

    /// issue request for start page
    QNetworkRequest request(OnlineSearchMathSciNetPrivate::queryFormUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchMathSciNet::doneFetchingQueryForm);

    refreshBusyProperty();
}

QString OnlineSearchMathSciNet::label() const
{
    return i18n("MathSciNet");
}

QString OnlineSearchMathSciNet::favIconUrl() const
{
    return QStringLiteral("http://www.ams.org/favicon.ico");
}

QUrl OnlineSearchMathSciNet::homepage() const
{
    return QUrl(QStringLiteral("https://mathscinet.ams.org/mathscinet/help/about.html"));
}

void OnlineSearchMathSciNet::doneFetchingQueryForm()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(++curStep, numSteps);

    if (handleErrors(reply)) {
        // UNUSED const QString htmlText = QString::fromUtf8(reply->readAll().constData());

        /// extract form's parameters ...
        QMap<QString, QString> formParams;
        /// ... and overwrite them with the query's parameters
        for (QMap<QString, QString>::ConstIterator it = d->queryParameters.constBegin(); it != d->queryParameters.constEnd(); ++it)
            formParams.insert(it.key(), it.value());

        /// build url by appending parameters
        QUrl url(OnlineSearchMathSciNetPrivate::queryUrlStem);
        QUrlQuery query(url);
        for (QMap<QString, QString>::ConstIterator it = formParams.constBegin(); it != formParams.constEnd(); ++it)
            query.addQueryItem(it.key(), it.value());
        for (int i = 1; i <= d->queryParameters.count(); ++i)
            query.addQueryItem(QString(QStringLiteral("co%1")).arg(i), QStringLiteral("AND")); ///< join search terms with an AND operation
        url.setQuery(query);

        /// issue request for result page
        QNetworkRequest request(url);
        QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
        connect(newReply, &QNetworkReply::finished, this, &OnlineSearchMathSciNet::doneFetchingResultPage);
    }

    refreshBusyProperty();
}

void OnlineSearchMathSciNet::doneFetchingResultPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(++curStep, numSteps);

    if (handleErrors(reply)) {
        const QString htmlText = QString::fromUtf8(reply->readAll().constData());

        /// extract form's parameters ...
        QMap<QString, QString> formParams = formParameters(htmlText, htmlText.indexOf(QStringLiteral("<form name=\"batchDownload\" action="), Qt::CaseInsensitive));

        /// build url by appending parameters
        QUrl url(OnlineSearchMathSciNetPrivate::queryUrlStem);
        QUrlQuery query(url);
        static const QStringList copyParameters {QStringLiteral("foo"), QStringLiteral("bdl"), QStringLiteral("reqargs"), QStringLiteral("batch_title")};
        for (const QString &param : copyParameters) {
            query.addQueryItem(param, formParams[param]);
        }
        query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("bibtex"));

        int count = 0;
        static const QRegularExpression checkBoxRegExp(QStringLiteral("<input class=\"hlCheckBox\" type=\"checkbox\" name=\"b\" value=\"(\\d+)\""));
        QRegularExpressionMatchIterator checkBoxRegExpMatchIt = checkBoxRegExp.globalMatch(htmlText);
        while (count < d->numResults && checkBoxRegExpMatchIt.hasNext()) {
            const QRegularExpressionMatch checkBoxRegExpMatch = checkBoxRegExpMatchIt.next();
            query.addQueryItem(QStringLiteral("b"), checkBoxRegExpMatch.captured(1));
            ++count;
        }

        url.setQuery(query);
        if (count > 0) {
            /// issue request for bibtex code
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchMathSciNet::doneFetchingBibTeXcode);
        } else {
            /// nothing found
            stopSearch(resultNoError);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchMathSciNet::doneFetchingBibTeXcode()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    emit progress(curStep = numSteps, numSteps);

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString htmlCode = QString::fromUtf8(reply->readAll().constData());

        QString bibtexCode;
        int p1 = -1, p2 = -1;
        while ((p1 = htmlCode.indexOf(QStringLiteral("<pre>"), p2 + 1)) >= 0 && (p2 = htmlCode.indexOf(QStringLiteral("</pre>"), p1 + 1)) >= 0) {
            bibtexCode += htmlCode.midRef(p1 + 5, p2 - p1 - 5);
            bibtexCode += QLatin1Char('\n');
        }

        FileImporterBibTeX importer(this);
        const File *bibtexFile = importer.fromString(bibtexCode);

        bool hasEntry = false;
        if (bibtexFile != nullptr) {
            for (const auto &element : *bibtexFile) {
                QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                hasEntry |= publishEntry(entry);

            }
            delete bibtexFile;
        }

        stopSearch(hasEntry ? resultNoError : resultUnspecifiedError);
    }

    refreshBusyProperty();
}

void OnlineSearchMathSciNet::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

    const QString ftFJournal = QStringLiteral("fjournal");
    if (entry->contains(ftFJournal)) {
        Value v = entry->value(ftFJournal);
        entry->remove(Entry::ftJournal);
        entry->remove(ftFJournal);
        entry->insert(Entry::ftJournal, v);
    }
}
