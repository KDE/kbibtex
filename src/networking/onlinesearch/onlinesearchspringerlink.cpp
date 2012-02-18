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

#include <QFile>
#include <QFormLayout>
#include <QSpinBox>
#include <QLabel>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <KLocale>
#include <KDebug>
#include <KLineEdit>
#include <KConfigGroup>
#include <KStandardDirs>

#include <internalnetworkaccessmanager.h>
#include "encoderlatex.h"
#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "onlinesearchspringerlink.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class OnlineSearchSpringerLink::OnlineSearchQueryFormSpringerLink : public OnlineSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QLatin1String("free"), QString()));
        lineEditTitle->setText(configGroup.readEntry(QLatin1String("title"), QString()));
        lineEditBookTitle->setText(configGroup.readEntry(QLatin1String("bookTitle"), QString()));
        lineEditAuthorEditor->setText(configGroup.readEntry(QLatin1String("authorEditor"), QString()));
        lineEditYear->setText(configGroup.readEntry(QLatin1String("year"), QString()));
        numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
    }

public:
    KLineEdit *lineEditFreeText, *lineEditTitle, *lineEditBookTitle, *lineEditAuthorEditor, *lineEditYear;
    QSpinBox *numResultsField;

    OnlineSearchQueryFormSpringerLink(QWidget *parent)
            : OnlineSearchQueryFormAbstract(parent), configGroupName(QLatin1String("Search Engine SpringerLink")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setMargin(0);

        lineEditFreeText = new KLineEdit(this);
        lineEditFreeText->setClearButtonShown(true);
        QLabel *label = new QLabel(i18n("Free Text:"), this);
        label->setBuddy(lineEditFreeText);
        layout->addRow(label, lineEditFreeText);
        connect(lineEditFreeText, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditTitle = new KLineEdit(this);
        lineEditTitle->setClearButtonShown(true);
        label = new QLabel(i18n("Title:"), this);
        label->setBuddy(lineEditTitle);
        layout->addRow(label, lineEditTitle);
        connect(lineEditTitle, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditBookTitle = new KLineEdit(this);
        lineEditBookTitle->setClearButtonShown(true);
        label = new QLabel(i18n("Book/Journal title:"), this);
        label->setBuddy(lineEditBookTitle);
        layout->addRow(label, lineEditBookTitle);
        connect(lineEditBookTitle, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditAuthorEditor = new KLineEdit(this);
        lineEditAuthorEditor->setClearButtonShown(true);
        label = new QLabel(i18n("Author or Editor:"), this);
        label->setBuddy(lineEditAuthorEditor);
        layout->addRow(label, lineEditAuthorEditor);
        connect(lineEditAuthorEditor, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditYear = new KLineEdit(this);
        lineEditYear->setClearButtonShown(true);
        label = new QLabel(i18n("Year:"), this);
        label->setBuddy(lineEditYear);
        layout->addRow(label, lineEditYear);
        connect(lineEditYear, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        numResultsField = new QSpinBox(this);
        label = new QLabel(i18n("Number of Results:"), this);
        label->setBuddy(numResultsField);
        layout->addRow(label, numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);

        lineEditFreeText->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const {
        return !(lineEditFreeText->text().isEmpty() && lineEditTitle->text().isEmpty() && lineEditBookTitle->text().isEmpty() && lineEditAuthorEditor->text().isEmpty());
    }

    void copyFromEntry(const Entry &entry) {
        lineEditTitle->setText(PlainTextValue::text(entry[Entry::ftTitle]));
        QString bookTitle = PlainTextValue::text(entry[Entry::ftBookTitle]);
        if (bookTitle.isEmpty())
            bookTitle = PlainTextValue::text(entry[Entry::ftJournal]);
        lineEditBookTitle->setText(bookTitle);
        lineEditAuthorEditor->setText(authorLastNames(entry).join(" "));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("free"), lineEditFreeText->text());
        configGroup.writeEntry(QLatin1String("title"), lineEditTitle->text());
        configGroup.writeEntry(QLatin1String("bookTitle"), lineEditBookTitle->text());
        configGroup.writeEntry(QLatin1String("authorEditor"), lineEditAuthorEditor->text());
        configGroup.writeEntry(QLatin1String("year"), lineEditYear->text());
        configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
        config->sync();
    }
};

class OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate
{
private:
    OnlineSearchSpringerLink *p;

public:
    const QString springerMetadataKey;
    XSLTransform xslt;
    OnlineSearchQueryFormSpringerLink *form;

    OnlineSearchSpringerLinkPrivate(OnlineSearchSpringerLink *parent)
            : p(parent), springerMetadataKey(QLatin1String("7pphfmtb9rtwt3dw3e4hm7av")), xslt(KStandardDirs::locate("data", "kbibtex/pam2bibtex.xsl")), form(NULL) {
        // nothing
    }

    KUrl buildQueryUrl() {
        if (form == NULL) return KUrl();

        KUrl queryUrl = KUrl(QString("http://api.springer.com/metadata/pam/?api_key=").append(springerMetadataKey));

        QString queryString = form->lineEditFreeText->text();

        QStringList titleChunks = p->splitRespectingQuotationMarks(form->lineEditTitle->text());
        foreach(const QString &titleChunk, titleChunks) {
            queryString += QString(QLatin1String(" title:%1")).arg(EncoderLaTeX::instance()->convertToPlainAscii(titleChunk));
        }

        titleChunks = p->splitRespectingQuotationMarks(form->lineEditBookTitle->text());
        foreach(const QString &titleChunk, titleChunks) {
            queryString += QString(QLatin1String(" ( journal:%1 OR book:%1 )")).arg(EncoderLaTeX::instance()->convertToPlainAscii(titleChunk));
        }

        QStringList authors = p->splitRespectingQuotationMarks(form->lineEditAuthorEditor->text());
        foreach(const QString &author, authors) {
            queryString += QString(QLatin1String(" name:%1")).arg(EncoderLaTeX::instance()->convertToPlainAscii(author));
        }

        const QString year = form->lineEditYear->text();
        if (!year.isEmpty())
            queryString += QString(QLatin1String(" year:%1")).arg(year);

        queryString = queryString.simplified();
        queryUrl.addQueryItem(QLatin1String("q"), queryString);

        return queryUrl;
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query) {
        KUrl queryUrl = KUrl(QString("http://api.springer.com/metadata/pam/?api_key=").append(springerMetadataKey));

        QString queryString = query[queryKeyFreeText] + ' ' + query[queryKeyTitle];

        QStringList titleChunks = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        foreach(const QString &titleChunk, titleChunks) {
            queryString += QString(QLatin1String(" title:%1")).arg(EncoderLaTeX::instance()->convertToPlainAscii(titleChunk));
        }

        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(const QString &author, authors) {
            queryString += QString(QLatin1String(" name:%1")).arg(EncoderLaTeX::instance()->convertToPlainAscii(author));
        }

        QString year = query[queryKeyYear];
        if (!year.isEmpty()) {
            static const QRegExp yearRegExp("\\b(18|19|20)[0-9]{2}\\b");
            if (yearRegExp.indexIn(year) >= 0) {
                year = yearRegExp.cap(0);
                queryString += QString(QLatin1String(" year:%1")).arg(year);
            }
        }

        queryString = queryString.simplified();
        queryUrl.addQueryItem(QLatin1String("q"), queryString);

        return queryUrl;
    }
};


OnlineSearchSpringerLink::OnlineSearchSpringerLink(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate(this))
{
    // nothing
}

OnlineSearchSpringerLink::~OnlineSearchSpringerLink()
{
    delete d;
}

void OnlineSearchSpringerLink::startSearch()
{
    m_hasBeenCanceled = false;

    KUrl springerLinkSearchUrl = d->buildQueryUrl();

    kDebug() << "springerLinkSearchUrl=" << springerLinkSearchUrl.pathOrUrl();
    emit progress(0, 1);
    QNetworkRequest request(springerLinkSearchUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingPAM()));

    if (d->form != NULL) d->form->saveState();
}

void OnlineSearchSpringerLink::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    KUrl springerLinkSearchUrl = d->buildQueryUrl(query);
    springerLinkSearchUrl.addQueryItem(QLatin1String("p"), QString::number(numResults));

    kDebug() << "springerLinkSearchUrl=" << springerLinkSearchUrl.pathOrUrl();
    emit progress(0, 1);
    QNetworkRequest request(springerLinkSearchUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingPAM()));
}

QString OnlineSearchSpringerLink::label() const
{
    return i18n("SpringerLink");
}

QString OnlineSearchSpringerLink::favIconUrl() const
{
    return QLatin1String("http://www.springerlink.com/images/favicon.ico");
}

OnlineSearchQueryFormAbstract* OnlineSearchSpringerLink::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new OnlineSearchQueryFormSpringerLink(parent);
    return d->form;
}

KUrl OnlineSearchSpringerLink::homepage() const
{
    return KUrl("http://www.springerlink.com/");
}

void OnlineSearchSpringerLink::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchSpringerLink::doneFetchingPAM()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        const QString xmlSource = ts.readAll();

        QString bibTeXcode = d->xslt.transform(xmlSource);
        bibTeXcode = bibTeXcode.replace(QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), QString());

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            foreach(const QSharedPointer<Element> &element, *bibtexFile) {
                QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
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
            emit progress(1, 1);

            delete bibtexFile;
        } else {
            kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

