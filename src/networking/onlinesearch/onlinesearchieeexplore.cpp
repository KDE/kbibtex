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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "onlinesearchieeexplore.h"

#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

#include <KMessageBox>
#include <KConfigGroup>
#include <KLocalizedString>

#include "internalnetworkaccessmanager.h"
#include "xsltransform.h"
#include "fileimporterbibtex.h"
#include "logging_networking.h"

class OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate
{
private:
    OnlineSearchIEEEXplore *p;

public:
    const QString gatewayUrl;
    XSLTransform xslt;
    int numSteps, curStep;

    OnlineSearchIEEEXplorePrivate(OnlineSearchIEEEXplore *parent)
            : p(parent), gatewayUrl(QStringLiteral("https://ieeexplore.ieee.org/gateway/ipsSearch.jsp")),
          xslt(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kbibtex/ieeexplore2bibtex.xsl"))),
          numSteps(0), curStep(0)
    {
        /// nothing
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QUrl queryUrl = QUrl(gatewayUrl);

        QStringList queryText;

        /// Free text
        QStringList freeTextFragments = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach (const QString &freeTextFragment, freeTextFragments) {
            queryText << QString(QStringLiteral("\"%1\"")).arg(freeTextFragment);
        }

        /// Title
        if (!query[queryKeyTitle].isEmpty())
            queryText << QString(QStringLiteral("\"Document Title\":\"%1\"")).arg(query[queryKeyTitle]);

        /// Author
        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach (const QString &author, authors) {
            queryText << QString(QStringLiteral("Author:\"%1\"")).arg(author);
        }

        /// Year
        if (!query[queryKeyYear].isEmpty())
            queryText << QString(QStringLiteral("\"Publication Year\":\"%1\"")).arg(query[queryKeyYear]);

        QUrlQuery q(queryUrl);
        q.addQueryItem(QStringLiteral("queryText"), queryText.join(QStringLiteral(" AND ")));
        q.addQueryItem(QStringLiteral("sortfield"), QStringLiteral("py"));
        q.addQueryItem(QStringLiteral("sortorder"), QStringLiteral("desc"));
        q.addQueryItem(QStringLiteral("hc"), QString::number(numResults));
        q.addQueryItem(QStringLiteral("rs"), QStringLiteral("1"));
        queryUrl.setQuery(q);

        return queryUrl;
    }
};

OnlineSearchIEEEXplore::OnlineSearchIEEEXplore(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate(this))
{
    // nothing
}

OnlineSearchIEEEXplore::~OnlineSearchIEEEXplore()
{
    delete d;
}

void OnlineSearchIEEEXplore::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchIEEEXplore::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 2;

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchIEEEXplore::doneFetchingXML);

    emit progress(d->curStep, d->numSteps);
}

void OnlineSearchIEEEXplore::doneFetchingXML()
{
    ++d->curStep;
    emit progress(d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, &QNetworkReply::finished, this, &OnlineSearchIEEEXplore::doneFetchingXML);
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString xmlCode = QString::fromUtf8(reply->readAll().constData());

            /// use XSL transformation to get BibTeX document from XML result
            const QString bibTeXcode = d->xslt.transform(xmlCode);
            if (bibTeXcode.isEmpty()) {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << reply->url().toDisplayString();
                emit stoppedSearch(resultInvalidArguments);
            } else {
                FileImporterBibTeX importer;
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != NULL) {
                    for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                        QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    emit stoppedSearch(resultNoError);
                    emit progress(d->numSteps, d->numSteps);

                    delete bibtexFile;
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toDisplayString();
                    emit stoppedSearch(resultUnspecifiedError);
                }
            }
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

QString OnlineSearchIEEEXplore::label() const
{
    return i18n("IEEEXplore");
}

QString OnlineSearchIEEEXplore::favIconUrl() const
{
    return QStringLiteral("https://ieeexplore.ieee.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIEEEXplore::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchIEEEXplore::homepage() const
{
    return QUrl(QStringLiteral("https://ieeexplore.ieee.org/"));
}

void OnlineSearchIEEEXplore::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchIEEEXplore::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

    /// XSL file cannot yet replace semicolon-separate author list
    /// by "and"-separated author list, so do it manually
    const QString ftXAuthor = QStringLiteral("x-author");
    if (!entry->contains(Entry::ftAuthor) && entry->contains(ftXAuthor)) {
        const Value xAuthorValue = entry->value(ftXAuthor);
        Value authorValue;
        for (Value::ConstIterator it = xAuthorValue.constBegin(); it != xAuthorValue.constEnd(); ++it) {
            QSharedPointer<PlainText> pt = it->dynamicCast<PlainText>();
            if (!pt.isNull()) {
                const QList<QSharedPointer<Person> > personList = FileImporterBibTeX::splitNames(pt->text());
                for (QList<QSharedPointer<Person> >::ConstIterator pit = personList.constBegin(); pit != personList.constEnd(); ++pit)
                    authorValue << *pit;
            }
        }

        entry->insert(Entry::ftAuthor, authorValue);
        entry->remove(ftXAuthor);
    }
}
