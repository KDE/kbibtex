/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchieeexplore.h"

#include <QNetworkReply>

#include <KMessageBox>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KStandardDirs>
#include <KUrl>

#include "internalnetworkaccessmanager.h"
#include "xsltransform.h"
#include "fileimporterbibtex.h"

class OnlineSearchIEEEXplore::OnlineSearchIEEEXplorePrivate
{
private:
    OnlineSearchIEEEXplore *p;

public:
    const QString gatewayUrl;
    XSLTransform *xslt;
    int numSteps, curStep;

    OnlineSearchIEEEXplorePrivate(OnlineSearchIEEEXplore *parent)
            : p(parent), gatewayUrl(QLatin1String("http://ieeexplore.ieee.org/gateway/ipsSearch.jsp")) {
        xslt = XSLTransform::createXSLTransform(KStandardDirs::locate("data", "kbibtex/ieeexplore2bibtex.xsl"));
    }


    ~OnlineSearchIEEEXplorePrivate() {
        delete xslt;
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        KUrl queryUrl = KUrl(gatewayUrl);

        QStringList queryText;

        /// Free text
        QStringList freeTextFragments = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(const QString &freeTextFragment, freeTextFragments) {
            queryText << QString(QLatin1String("\"%1\"")).arg(freeTextFragment);
        }

        /// Title
        if (!query[queryKeyTitle].isEmpty())
            queryText << QString(QLatin1String("\"Document Title\":\"%1\"")).arg(query[queryKeyTitle]);

        /// Author
        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(const QString &author, authors) {
            queryText << QString(QLatin1String("Author:\"%1\"")).arg(author);
        }

        /// Year
        if (!query[queryKeyYear].isEmpty())
            queryText << QString(QLatin1String("\"Publication Year\":\"%1\"")).arg(query[queryKeyYear]);

        queryUrl.addQueryItem(QLatin1String("queryText"), queryText.join(QLatin1String(" AND ")));
        queryUrl.addQueryItem(QLatin1String("sortfield"), QLatin1String("py"));
        queryUrl.addQueryItem(QLatin1String("sortorder"), QLatin1String("desc"));
        queryUrl.addQueryItem(QLatin1String("hc"), QString::number(numResults));
        queryUrl.addQueryItem(QLatin1String("rs"), QLatin1String("1"));

        kDebug() << "buildQueryUrl=" << queryUrl.pathOrUrl();

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
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingXML()));

    emit progress(d->curStep, d->numSteps);
}

void OnlineSearchIEEEXplore::doneFetchingXML()
{
    ++d->curStep;
    emit progress(d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    KUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
            connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingXML()));
        } else {
            /// ensure proper treatment of UTF-8 characters
            const QString xmlCode = QString::fromUtf8(reply->readAll().data());

            /// use XSL transformation to get BibTeX document from XML result
            const QString bibTeXcode = d->xslt->transform(xmlCode);

            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    hasEntries |= publishEntry(entry);
                }

                if (!hasEntries)
                    kDebug() << "No hits found in" << reply->url().toString();
                emit stoppedSearch(resultNoError);
                emit progress(d->numSteps, d->numSteps);

                delete bibtexFile;
            } else {
                kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                emit stoppedSearch(resultUnspecifiedError);
            }
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

QString OnlineSearchIEEEXplore::label() const
{
    return i18n("IEEEXplore");
}

QString OnlineSearchIEEEXplore::favIconUrl() const
{
    return QLatin1String("http://ieeexplore.ieee.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIEEEXplore::customWidget(QWidget *)
{
    return NULL;
}

KUrl OnlineSearchIEEEXplore::homepage() const
{
    return KUrl("http://ieeexplore.ieee.org/");
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
    const QString ftXAuthor = QLatin1String("x-author");
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
