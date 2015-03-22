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

#include "onlinesearchoclcworldcat.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>

#include <KLocale>
#include <KStandardDirs>

#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"

class OnlineSearchOCLCWorldCat::Private
{
private:
    static const QString APIkey;

    OnlineSearchOCLCWorldCat *p;
public:
    static const int countPerStep;
    int maxSteps, curStep;

    QUrl baseUrl;

    XSLTransform *xslt;

    Private(OnlineSearchOCLCWorldCat *parent)
            : p(parent), maxSteps(0), curStep(0) {
        const QString xsltFilename = QLatin1String("kbibtex/worldcatdc2bibtex.xsl");
        xslt = XSLTransform::createXSLTransform(QStandardPaths::locate(QStandardPaths::GenericDataLocation, xsltFilename));
        if (xslt == NULL)
            qWarning() << "Could not create XSLT transformation for" << xsltFilename;
    }

    ~Private() {
        delete xslt;
    }

    void setBaseUrl(const QMap<QString, QString> &query) {
        QStringList queryFragments;

        /// Add words from "free text" field
        /// WorldCat's Open Search does not support "free" search,
        /// instead, title, keyword, subject etc are searched OR-connected
        const QStringList freeTextWords = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        for (QStringList::ConstIterator it = freeTextWords.constBegin(); it != freeTextWords.constEnd(); ++it) {
            static const QString freeWorldTemplate = QLatin1String("(+srw.ti+all+\"%1\"+or+srw.kw+all+\"%1\"+or+srw.au+all+\"%1\"+or+srw.bn+all+\"%1\"+or+srw.su+all+\"%1\"+)");
            queryFragments.append(freeWorldTemplate.arg(*it));
        }
        /// Add words from "title" field
        const QStringList titleWords = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        for (QStringList::ConstIterator it = titleWords.constBegin(); it != titleWords.constEnd(); ++it) {
            static const QString titleTemplate = QLatin1String("srw.ti+all+\"%1\"");
            queryFragments.append(titleTemplate.arg(*it));
        }
        /// Add words from "author" field
        const QStringList authorWords = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (QStringList::ConstIterator it = authorWords.constBegin(); it != authorWords.constEnd(); ++it) {
            static const QString authorTemplate = QLatin1String("srw.au+all+\"%1\"");
            queryFragments.append(authorTemplate.arg(*it));
        }

        /// Field year cannot stand alone, therefore if no query fragments
        /// have been assembled yet, no valid search will be possible.
        /// Invalidate base URL and return in this case.
        if (queryFragments.isEmpty()) {
            qWarning() << "For WorldCat OCLC search to work, either a title or an author has get specified; free text or year only is not sufficient";
            baseUrl.clear();
            return;
        }

        /// Add words from "year" field
        const QStringList yearWords = p->splitRespectingQuotationMarks(query[queryKeyYear]);
        for (QStringList::ConstIterator it = yearWords.constBegin(); it != yearWords.constEnd(); ++it) {
            static const QString yearTemplate = QLatin1String("srw.yr+any+\"%1\"");
            queryFragments.append(yearTemplate.arg(*it));
        }

        const QString queryString = queryFragments.join(QLatin1String("+and+"));
        baseUrl = QUrl(QString(QLatin1String("http://www.worldcat.org/webservices/catalog/search/worldcat/sru?query=%1&recordSchema=%3&wskey=%2")).arg(queryString).arg(OnlineSearchOCLCWorldCat::Private::APIkey).arg(QLatin1String("info%3Asrw%2Fschema%2F1%2Fdc")));
        baseUrl.addQueryItem(QLatin1String("maximumRecords"), QString::number(OnlineSearchOCLCWorldCat::Private::countPerStep));
    }
};

const int OnlineSearchOCLCWorldCat::Private::countPerStep = 11; /// pseudo-randomly chosen prime number between 10 and 20
const QString OnlineSearchOCLCWorldCat::Private::APIkey = QLatin1String("Bt6h4KIHrfbSXEahwUzpFQD6SNjQZfQUG3W2LN9oNEB5tROFGeRiDVntycEEyBe0aH17sH4wrNlnVANH");

OnlineSearchOCLCWorldCat::OnlineSearchOCLCWorldCat(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchOCLCWorldCat::Private(this)) {
    // TODO
}

OnlineSearchOCLCWorldCat::~OnlineSearchOCLCWorldCat() {
    delete d;
}

void OnlineSearchOCLCWorldCat::startSearch() {
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchOCLCWorldCat::startSearch(const QMap<QString, QString> &query, int numResults) {
    if (d->xslt == NULL) {
        /// Don't allow searches if xslt is not defined
        qWarning() << "Cannot allow searching" << label() << "if XSL Transformation not available";
        delayedStoppedSearch(resultUnspecifiedError);
        return;
    }

    m_hasBeenCanceled = false;

    d->maxSteps = (numResults + OnlineSearchOCLCWorldCat::Private::countPerStep - 1) / OnlineSearchOCLCWorldCat::Private::countPerStep;
    d->curStep = 0;
    emit progress(d->curStep, d->maxSteps);

    d->setBaseUrl(query);
    if (d->baseUrl.isEmpty()) {
        /// No base URL set for some reason, no search possible
        m_hasBeenCanceled = false;
        delayedStoppedSearch(resultInvalidArguments);
        return;
    }
    QUrl startUrl = d->baseUrl;
    QNetworkRequest request(startUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));
}

QString OnlineSearchOCLCWorldCat::label() const {
    return i18n("OCLC WorldCat");
}

OnlineSearchQueryFormAbstract *OnlineSearchOCLCWorldCat::customWidget(QWidget *) {
    return NULL;
}

QUrl OnlineSearchOCLCWorldCat::homepage() const {
    return QUrl(QLatin1String("http://www.worldcat.org/"));
}

void OnlineSearchOCLCWorldCat::cancel() {
    OnlineSearchAbstract::cancel();
}

QString OnlineSearchOCLCWorldCat::favIconUrl() const {
    return QLatin1String("http://www.worldcat.org/favicon.ico");
}

void OnlineSearchOCLCWorldCat::downloadDone() {
    emit progress(++d->curStep, d->maxSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// Ensure proper treatment of UTF-8 characters
        const QString atomCode = QString::fromUtf8(reply->readAll().data()).remove(QLatin1String("xmlns=\"http://www.w3.org/2005/Atom\"")).remove(QLatin1String(" xmlns=\"http://www.loc.gov/zing/srw/\"")); // FIXME fix worldcatdc2bibtex.xsl to handle namespace

        /// Use XSL transformation to get BibTeX document from XML result
        const QString bibTeXcode = d->xslt->transform(atomCode).remove(QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntries |= publishEntry(entry);
            }
            delete bibtexFile;

            if (!hasEntries) {
                emit progress(d->maxSteps, d->maxSteps);
                emit stoppedSearch(resultNoError);
            } else if (d->curStep < d->maxSteps) {
                QUrl nextUrl = d->baseUrl;
                nextUrl.addQueryItem(QLatin1String("start"), QString::number(d->curStep * OnlineSearchOCLCWorldCat::Private::countPerStep + 1));
                QNetworkRequest request(nextUrl);
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
                connect(newReply, SIGNAL(finished()), this, SLOT(downloadDone()));
            } else {
                emit progress(d->maxSteps, d->maxSteps);
                emit stoppedSearch(resultNoError);
            }
        } else {
            qWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        qWarning() << "url was" << reply->url().toString();
}
