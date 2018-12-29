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

#include "onlinesearchieeexplore.h"

#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

#ifdef HAVE_KF5
#include <KLocalizedString>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#include "internalnetworkaccessmanager.h"
#include "xsltransform.h"
#include "encoderxml.h"
#include "fileimporterbibtex.h"
#include "logging_networking.h"

class OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate
{
private:
    static const QString xsltFilenameBase;

public:
    static const QUrl apiUrl;
    const XSLTransform xslt;

    OnlineSearchIEEEXplorePrivate(OnlineSearchIEEEXplore *)
            : xslt(XSLTransform::locateXSLTfile(xsltFilenameBase))
    {
        if (!xslt.isValid())
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to initialize XSL transformation based on file '" << xsltFilenameBase << "'";
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QUrl queryUrl = apiUrl;
        QUrlQuery q(queryUrl.query());

        /// Free text
        const QStringList freeTextFragments = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyFreeText]);
        if (!freeTextFragments.isEmpty())
            q.addQueryItem(QStringLiteral("querytext"), QStringLiteral("\"") + freeTextFragments.join(QStringLiteral("\"+\"")) + QStringLiteral("\""));

        /// Title
        const QStringList title = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyTitle]);
        if (!title.isEmpty())
            q.addQueryItem(QStringLiteral("article_title"), QStringLiteral("\"") + title.join(QStringLiteral("\"+\"")) + QStringLiteral("\""));

        /// Author
        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyAuthor]);
        if (!authors.isEmpty())
            q.addQueryItem(QStringLiteral("author"), QStringLiteral("\"") + authors.join(QStringLiteral("\"+\"")) + QStringLiteral("\""));

        /// Year
        if (!query[queryKeyYear].isEmpty()) {
            q.addQueryItem(QStringLiteral("start_year"), query[queryKeyYear]);
            q.addQueryItem(QStringLiteral("end_year"), query[queryKeyYear]);
        }

        /// Sort order of results: newest publications first
        q.addQueryItem(QStringLiteral("sort_field"), QStringLiteral("publication_year"));
        q.addQueryItem(QStringLiteral("sort_order"), QStringLiteral("desc"));
        /// Request numResults many entries
        q.addQueryItem(QStringLiteral("start_record"), QStringLiteral("1"));
        q.addQueryItem(QStringLiteral("max_records"), QString::number(numResults));

        queryUrl.setQuery(q);

        return queryUrl;
    }
};

const QString OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate::xsltFilenameBase = QStringLiteral("ieeexploreapiv1-to-bibtex.xsl");
const QUrl OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate::apiUrl(QStringLiteral("https://ieeexploreapi.ieee.org/api/v1/search/articles?format=xml&apikey=") + InternalNetworkAccessManager::reverseObfuscate("\x15\x65\x4b\x2a\x37\x5f\x78\x12\x44\x70\xf8\x8e\x85\xe0\xdb\xae\xb\x7a\x7e\x46\xab\x93\xbc\xc8\xdb\xa8\xa5\xd2\xee\x96\x7e\x7\x37\x54\xa3\xd4\x2b\x5e\x81\xe6\x6f\x17\xb3\xd6\x7b\x1f\x1a\x60"));

OnlineSearchIEEEXplore::OnlineSearchIEEEXplore(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate(this))
{
    /// nothing
}

OnlineSearchIEEEXplore::~OnlineSearchIEEEXplore()
{
    delete d;
}

void OnlineSearchIEEEXplore::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl(query, numResults));

    // FIXME 'ieeexploreapi.ieee.org' uses a SSL/TLS certificate only valid for 'mashery.com'
    // TODO re-enable certificate validation once problem has been fix (already reported)
    QSslConfiguration requestSslConfig = request.sslConfiguration();
    requestSslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(requestSslConfig);

    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchIEEEXplore::doneFetchingXML);

    refreshBusyProperty();
}

void OnlineSearchIEEEXplore::doneFetchingXML()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);

            // FIXME 'ieeexploreapi.ieee.org' uses a SSL/TLS certificate only valid for 'mashery.com'
            // TODO re-enable certificate validation once problem has been fix (already reported)
            QSslConfiguration requestSslConfig = request.sslConfiguration();
            requestSslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            request.setSslConfiguration(requestSslConfig);

            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchIEEEXplore::doneFetchingXML);
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString xmlCode = QString::fromUtf8(reply->readAll().constData());

            /// use XSL transformation to get BibTeX document from XML result
            const QString bibTeXcode = EncoderXML::instance().decode(d->xslt.transform(xmlCode));
            if (bibTeXcode.isEmpty()) {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                stopSearch(resultInvalidArguments);
            } else {
                FileImporterBibTeX importer(this);
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != nullptr) {
                    for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    stopSearch(resultNoError);

                    delete bibtexFile;
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                    stopSearch(resultUnspecifiedError);
                }
            }
        }
    }

    refreshBusyProperty();
}

QString OnlineSearchIEEEXplore::label() const
{
    return i18n("IEEEXplore");
}

QString OnlineSearchIEEEXplore::favIconUrl() const
{
    return QStringLiteral("http://ieeexplore.ieee.org/favicon.ico");
}

QUrl OnlineSearchIEEEXplore::homepage() const
{
    return QUrl(QStringLiteral("https://ieeexplore.ieee.org/"));
}
