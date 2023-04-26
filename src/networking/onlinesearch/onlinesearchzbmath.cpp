/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchzbmath.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QSet>

#ifdef HAVE_KF5
#include <KLocalizedString>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#include <XSLTransform>
#include <EncoderXML>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchZbMath::Private
{
private:
    static const QString xsltFilenameBase;
    OnlineSearchZbMath *parent;

public:
    static const QUrl helperFilterUrl;
    int numAwaitedResults;
    // resumptionCounter is not (yet) used as resumption token are not evaluated
    int resumptionCounter;
    const XSLTransform xslt;
    QSet<QString> freeTextFragments, titleFragments;

    Private(OnlineSearchZbMath *_parent)
            : parent(_parent), numAwaitedResults(0), resumptionCounter(0), xslt(XSLTransform::locateXSLTfile(xsltFilenameBase))
    {
        if (!xslt.isValid())
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to initialize XSL transformation based on file '" << xsltFilenameBase << "'";
    }

    QString filterString(const QMap<QueryKey, QString> &query)
    {
        QStringList result;

        /// Free text
        freeTextFragments.clear();
        const QStringList _freeTextFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::FreeText]);
        for (const QString &freeTextFragment : _freeTextFragments)
            freeTextFragments.insert(freeTextFragment.toLower());

        /// Title
        titleFragments.clear();
        const QStringList _titleFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Title]);
        for (const QString &titleFragment : _titleFragments)
            titleFragments.insert(titleFragment.toLower());

        /// Authors
        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Author]);
        for (const QString &author : authors)
            result.append(QString(QStringLiteral("author:%1")).arg(author));
        const bool hasAuthors = !authors.isEmpty();

        /// Year
        static const QRegularExpression yearRegExp(QStringLiteral("\\b(18|19|20)[0-9]{2}\\b"));
        const QRegularExpressionMatch yearRegExpMatch = yearRegExp.match(query[QueryKey::Year]);
        if (yearRegExpMatch.hasMatch())
            result.append(QString(QStringLiteral("publication_year:%1")).arg(yearRegExpMatch.captured()));
        const bool hasYear = !authors.isEmpty();

        if (!hasAuthors || !hasYear) {
            const QString text = !hasAuthors && hasYear ? i18n("The query to be sent to 'zbMATH Open' lacks author information, which may result in few or no matching results.") : (hasAuthors && !hasYear ? i18n("The query to be sent to 'zbMATH Open' lacks 'year' information, which may degrade result quality.") : i18n("The query to be sent to 'zbMATH Open' lacks both author and 'year' information.\nThis will most likely result in few or no matching results."));
            parent->sendVisualNotification(text, parent->label(), 20);
        }
        return result.join(QStringLiteral("&"));
    }
};

const QString OnlineSearchZbMath::Private::xsltFilenameBase = QStringLiteral("oaizbpreview-to-bibtex.xsl");
const QUrl OnlineSearchZbMath::Private::helperFilterUrl(QStringLiteral("https://oai.zbmath.org/v1/helper/filter"));


OnlineSearchZbMath::OnlineSearchZbMath(QObject *parent)
    : OnlineSearchAbstract(parent), d(new OnlineSearchZbMath::Private(this))
{
    /// nothing
}

OnlineSearchZbMath::~OnlineSearchZbMath()
{
    delete d;
}

void OnlineSearchZbMath::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    Q_EMIT progress(curStep = 0, numSteps = 1);

    /// Remember number of expected results, but ensure that it is within a reasonable range
    d->numAwaitedResults = qMin(1024, qMax(1, numResults));
    /// To track how often a resumption token was followed
    d->resumptionCounter = 0;

    QUrl u(Private::helperFilterUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("metadataPrefix"), QStringLiteral("oai_zb_preview"));
    const QString filterString = d->filterString(query);
    if (!filterString.isEmpty())
        urlQuery.addQueryItem(QStringLiteral("filter"), filterString);
    u.setQuery(urlQuery);

    QNetworkRequest request(u);
    request.setRawHeader(QByteArray("Accept"), QByteArray("text/xml"));

    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchZbMath::doneFetchingOAI);

    refreshBusyProperty();
}

QString OnlineSearchZbMath::label() const
{
    return i18n("zbMATH Open");
}

QUrl OnlineSearchZbMath::homepage() const
{
    return QUrl(QStringLiteral("https://zbmath.org/"));
}

void OnlineSearchZbMath::doneFetchingOAI()
{
    Q_EMIT progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        const QString xmlCode = QString::fromUtf8(reply->readAll().constData());
        const QString bibTeXcode = EncoderXML::instance().decode(d->xslt.transform(xmlCode));

        if (bibTeXcode.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultInvalidArguments);
        } else {
            FileImporterBibTeX importer(this);
            QScopedPointer<File> bibtexFile(importer.fromString(bibTeXcode));

            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                    if (!entry.isNull()) {
                        /// Now check whether the entry contains the user-provided title fragments or free-text fragments
                        const QString title = entry->contains(Entry::ftTitle) ? PlainTextValue::text(entry->value(Entry::ftTitle)).toLower() : QString();
                        bool allTitleFragmentsContained = true;
                        for (const QString &titleFragment : const_cast<const QSet<QString> &>(d->titleFragments))
                            allTitleFragmentsContained &= title.contains(titleFragment);
                        const QString freeText = title;
                        bool allFreeTextFragmentsContained = true;
                        for (const QString &freeTextFragment : const_cast<const QSet<QString> &>(d->freeTextFragments))
                            allFreeTextFragmentsContained &= freeText.contains(freeTextFragment);

                        if (allTitleFragmentsContained && allFreeTextFragmentsContained) {
                            publishEntry(entry);
                            --d->numAwaitedResults;
                            if (d->numAwaitedResults <= 0)
                                break;
                        }
                    }
                }

                if (d->resumptionCounter < 16 && d->numAwaitedResults > 0) {
                    QString resumptionToken;
                    int rts = xmlCode.indexOf(QStringLiteral("<resumptionToken"));
                    if (rts > 0) {
                        rts = xmlCode.indexOf(QStringLiteral(">"), rts + 10);
                        if (rts > 0) {
                            int rte = xmlCode.indexOf(QStringLiteral("<"), rts);
                            if (rte > 0)
                                resumptionToken = xmlCode.mid(rts + 1, rte - rts - 1);
                        }
                    }

                    if (!resumptionToken.isEmpty()) {
                        // TODO resume to get more results
                        stopSearch(resultNoError);//< replace with proper code once resumption gets implemented
                    } else
                        stopSearch(resultNoError);
                } else
                    stopSearch(resultNoError);
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                stopSearch(resultUnspecifiedError);
            }
        }
    }

    refreshBusyProperty();
}
