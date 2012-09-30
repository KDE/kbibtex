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

#include <QNetworkReply>
#include <QTextStream>

#include <KStandardDirs>
#include <KLocale>
#include <KDebug>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "onlinesearchisbndb.h"
#include "xsltransform.h"

class OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate
{
private:
    OnlineSearchIsbnDB *p;
    static const QString accessKey;
    static const QString booksUrl, authorsUrl;

    QString cachedPersonId;

public:
    XSLTransform xslt;

    OnlineSearchIsbnDBPrivate(OnlineSearchIsbnDB *parent)
            : p(parent), xslt(KStandardDirs::locate("data", "kbibtex/isbndb2bibtex.xsl")) {
        // nothing
    }

    KUrl buildBooksUrl(const QMap<QString, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        KUrl queryUrl(booksUrl);
        queryUrl.addQueryItem(QLatin1String("access_key"),accessKey);
        queryUrl.addQueryItem(QLatin1String("results"),QLatin1String("details,texts"));

        QString index1, value1;
                if (query[queryKeyFreeText].isEmpty()&&query[queryKeyAuthor].isEmpty()&&!query[queryKeyTitle].isEmpty()){
                    /// only searching for title
                    index1=QLatin1String("title");
                    value1=query[queryKeyTitle];
                }else if (!cachedPersonId.isEmpty()&&query[queryKeyFreeText].isEmpty()&&!query[queryKeyAuthor].isEmpty()&&query[queryKeyTitle].isEmpty()){
                    /// only searching for author
                    index1=QLatin1String("person_id");
                    value1=cachedPersonId;
                 }else{
                    /// multiple different values given
                    index1=QLatin1String("full");
                    value1=query[queryKeyFreeText]+QChar(' ')+query[queryKeyAuthor]+QChar(' ')+query[queryKeyTitle];
                }
                queryUrl.addQueryItem(QLatin1String("index1"),index1);
                queryUrl.addQueryItem(QLatin1String("value1"),value1);

        return queryUrl;
    }
};

const QString OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate::accessKey=QLatin1String("NBTD24WJ");
const QString OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate::booksUrl=QLatin1String("http://isbndb.com/api/books.xml");
const QString OnlineSearchIsbnDB::OnlineSearchIsbnDBPrivate::authorsUrl=QLatin1String("http://isbndb.com/api/authors.xml");

OnlineSearchIsbnDB::OnlineSearchIsbnDB(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIsbnDBPrivate(this))
{
    // nothing
}

OnlineSearchIsbnDB::~OnlineSearchIsbnDB()
{
    delete d;
}

void OnlineSearchIsbnDB::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildBooksUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, 2);
}

void OnlineSearchIsbnDB::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

QString OnlineSearchIsbnDB::label() const
{
    return i18n("ISBNdb.com");
}

QString OnlineSearchIsbnDB::favIconUrl() const
{
    return QLatin1String("http://isbndb.com/images/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIsbnDB::customWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return NULL;
}

KUrl OnlineSearchIsbnDB::homepage() const
{
    return KUrl("http://isbndb.com/");
}

void OnlineSearchIsbnDB::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchIsbnDB::downloadDone()
{
    emit progress(1, 2);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        ts.setCodec("utf-8");
        QString xmlCode = ts.readAll();

        dumpToFile("xml.xml",xmlCode);

        /// use XSL transformation to get BibTeX document from XML result
        QString bibtexCode = d->xslt.transform(xmlCode).replace(QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), QString());

        dumpToFile("bib.bib",bibtexCode);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibtexCode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (!entry.isNull()) {
                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                    entry->insert("x-fetchedfrom", v);
                    emit foundEntry(entry);
                    hasEntries = true;
                }

            }

            if (!hasEntries)
                kDebug() << "No hits found in" << reply->url().toString();
            emit stoppedSearch(resultNoError);

            delete bibtexFile;
        } else {
            kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();

    emit progress(2, 2);
}
