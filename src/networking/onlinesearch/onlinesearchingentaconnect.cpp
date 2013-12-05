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

#include <QBuffer>
#include <QLabel>
#include <QNetworkReply>
#include <QFormLayout>
#include <QSpinBox>

#include <KLocale>
#include <KDebug>
#include <KIcon>
#include <KLineEdit>
#include <KConfigGroup>

#include "file.h"
#include "entry.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "onlinesearchingentaconnect.h"

class OnlineSearchIngentaConnect::OnlineSearchQueryFormIngentaConnect : public OnlineSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFullText->setText(configGroup.readEntry(QLatin1String("fullText"), QString()));
        lineEditTitle->setText(configGroup.readEntry(QLatin1String("title"), QString()));
        lineEditAuthor->setText(configGroup.readEntry(QLatin1String("author"), QString()));
        lineEditAbstractKeywords->setText(configGroup.readEntry(QLatin1String("abstractKeywords"), QString()));
        lineEditPublication->setText(configGroup.readEntry(QLatin1String("publication"), QString()));
        lineEditISSNDOIISBN->setText(configGroup.readEntry(QLatin1String("ISSNDOIISBN"), QString()));
        lineEditVolume->setText(configGroup.readEntry(QLatin1String("volume"), QString()));
        lineEditIssue->setText(configGroup.readEntry(QLatin1String("issue"), QString()));
        numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
    }

public:
    KLineEdit *lineEditFullText;
    KLineEdit *lineEditTitle;
    KLineEdit *lineEditAuthor;
    KLineEdit *lineEditAbstractKeywords;
    KLineEdit *lineEditPublication;
    KLineEdit *lineEditISSNDOIISBN;
    KLineEdit *lineEditVolume;
    KLineEdit *lineEditIssue;
    QSpinBox *numResultsField;

    OnlineSearchQueryFormIngentaConnect(QWidget *widget)
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QLatin1String("Search Engine IngentaConnect")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setMargin(0);

        lineEditFullText = new KLineEdit(this);
        lineEditFullText->setClearButtonShown(true);
        layout->addRow(i18n("Full text:"), lineEditFullText);
        connect(lineEditFullText, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditTitle = new KLineEdit(this);
        lineEditTitle->setClearButtonShown(true);
        layout->addRow(i18n("Title:"), lineEditTitle);
        connect(lineEditTitle, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditAuthor = new KLineEdit(this);
        lineEditAuthor->setClearButtonShown(true);
        layout->addRow(i18n("Author:"), lineEditAuthor);
        connect(lineEditAuthor, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditAbstractKeywords = new KLineEdit(this);
        lineEditAbstractKeywords->setClearButtonShown(true);
        layout->addRow(i18n("Abstract/Keywords:"), lineEditAbstractKeywords);
        connect(lineEditAbstractKeywords, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditPublication = new KLineEdit(this);
        lineEditPublication->setClearButtonShown(true);
        layout->addRow(i18n("Publication:"), lineEditPublication);
        connect(lineEditPublication, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditISSNDOIISBN = new KLineEdit(this);
        lineEditISSNDOIISBN->setClearButtonShown(true);
        layout->addRow(i18n("ISSN/ISBN/DOI:"), lineEditISSNDOIISBN);
        connect(lineEditISSNDOIISBN, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditVolume = new KLineEdit(this);
        lineEditVolume->setClearButtonShown(true);
        layout->addRow(i18n("Volume:"), lineEditVolume);
        connect(lineEditVolume, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditIssue = new KLineEdit(this);
        lineEditIssue->setClearButtonShown(true);
        layout->addRow(i18n("Issue/Number:"), lineEditIssue);
        connect(lineEditIssue, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        numResultsField = new QSpinBox(this);
        layout->addRow(i18n("Number of Results:"), numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(10);
    }

    bool readyToStart() const {
        return !(lineEditFullText->text().isEmpty() && lineEditTitle->text().isEmpty() && lineEditAuthor->text().isEmpty() && lineEditAbstractKeywords->text().isEmpty() && lineEditPublication->text().isEmpty() && lineEditISSNDOIISBN->text().isEmpty() && lineEditVolume->text().isEmpty() && lineEditIssue->text().isEmpty());
    }

    void copyFromEntry(const Entry &entry) {
        lineEditTitle->setText(PlainTextValue::text(entry[Entry::ftTitle]));
        lineEditAuthor->setText(authorLastNames(entry).join(" "));
        lineEditVolume->setText(PlainTextValue::text(entry[Entry::ftVolume]));
        lineEditIssue->setText(PlainTextValue::text(entry[Entry::ftNumber]));
        QString issnDoiIsbn = PlainTextValue::text(entry[Entry::ftDOI]);
        if (issnDoiIsbn.isEmpty())
            issnDoiIsbn = PlainTextValue::text(entry[Entry::ftISBN]);
        if (issnDoiIsbn.isEmpty())
            issnDoiIsbn = PlainTextValue::text(entry[Entry::ftISSN]);
        lineEditISSNDOIISBN->setText(issnDoiIsbn);
        QString publication = PlainTextValue::text(entry[Entry::ftJournal]);
        if (publication.isEmpty())
            publication = PlainTextValue::text(entry[Entry::ftBookTitle]);
        lineEditPublication->setText(publication);

        // TODO
        lineEditAbstractKeywords->setText(QLatin1String(""));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("fullText"), lineEditFullText->text());
        configGroup.writeEntry(QLatin1String("title"), lineEditTitle->text());
        configGroup.writeEntry(QLatin1String("author"), lineEditAuthor->text());
        configGroup.writeEntry(QLatin1String("abstractKeywords"), lineEditAbstractKeywords->text());
        configGroup.writeEntry(QLatin1String("publication"), lineEditPublication->text());
        configGroup.writeEntry(QLatin1String("ISSNDOIISBN"), lineEditISSNDOIISBN->text());
        configGroup.writeEntry(QLatin1String("volume"), lineEditVolume->text());
        configGroup.writeEntry(QLatin1String("issue"), lineEditIssue->text());
        configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
        config->sync();
    }
};

class OnlineSearchIngentaConnect::OnlineSearchIngentaConnectPrivate
{
private:
    OnlineSearchIngentaConnect *p;
    const QString ingentaConnectBaseUrl;

public:
    OnlineSearchQueryFormIngentaConnect *form;

    OnlineSearchIngentaConnectPrivate(OnlineSearchIngentaConnect *parent)
            : p(parent), ingentaConnectBaseUrl(QLatin1String("http://www.ingentaconnect.com/search?format=bib")), form(NULL) {
        // nothing
    }

    KUrl buildQueryUrl() {
        if (form == NULL) {
            kWarning() << "Cannot build query url if no form is specified";
            return KUrl();
        }

        KUrl queryUrl(ingentaConnectBaseUrl);


        int index = 1;
        QStringList chunks = p->splitRespectingQuotationMarks(form->lineEditFullText->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND")); ///< join search terms with an AND operation
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("fulltext"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditAuthor->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("author"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditTitle->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("title"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditPublication->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("journalbooktitle"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditIssue->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("issue"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditVolume->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("volume"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditAbstractKeywords->text());
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("tka"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        queryUrl.addQueryItem(QLatin1String("pageSize"), QString::number(form->numResultsField->value()));

        queryUrl.addQueryItem(QLatin1String("sortDescending"), QLatin1String("true"));
        queryUrl.addQueryItem(QLatin1String("subscribed"), QLatin1String("false"));
        queryUrl.addQueryItem(QLatin1String("sortField"), QLatin1String("default"));
        return queryUrl;
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        KUrl queryUrl(ingentaConnectBaseUrl);

        int index = 1;
        QStringList chunks = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("fulltext"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("author"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        foreach(const QString &chunk, chunks) {
            if (index > 1)
                queryUrl.addQueryItem(QString("operator%1").arg(index), QLatin1String("AND"));
            queryUrl.addQueryItem(QString("option%1").arg(index), QLatin1String("title"));
            queryUrl.addQueryItem(QString("value%1").arg(index), chunk);
            ++index;
        }

        /// Field "year" not supported in IngentaConnect's search

        queryUrl.addQueryItem(QLatin1String("pageSize"), QString::number(numResults));

        queryUrl.addQueryItem(QLatin1String("sortDescending"), QLatin1String("true"));
        queryUrl.addQueryItem(QLatin1String("subscribed"), QLatin1String("false"));
        queryUrl.addQueryItem(QLatin1String("sortField"), QLatin1String("default"));
        return queryUrl;
    }
};

OnlineSearchIngentaConnect::OnlineSearchIngentaConnect(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIngentaConnectPrivate(this))
{
    // nothing
}

OnlineSearchIngentaConnect::~OnlineSearchIngentaConnect()
{
    delete d;
}

void OnlineSearchIngentaConnect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, 1);
}

void OnlineSearchIngentaConnect::startSearch()
{
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, 1);

    d->form->saveState();
}

QString OnlineSearchIngentaConnect::label() const
{
    return i18n("IngentaConnect");
}

QString OnlineSearchIngentaConnect::favIconUrl() const
{
    return QLatin1String("http://www.ingentaconnect.com/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIngentaConnect::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new OnlineSearchIngentaConnect::OnlineSearchQueryFormIngentaConnect(parent);
    return d->form;
}

KUrl OnlineSearchIngentaConnect::homepage() const
{
    return KUrl("http://www.ingentaconnect.com/");
}

void OnlineSearchIngentaConnect::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchIngentaConnect::downloadDone()
{
    emit progress(1, 1);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().data());

        if (!bibTeXcode.isEmpty()) {
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

                delete bibtexFile;
            } else {
                kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                emit stoppedSearch(resultUnspecifiedError);
            }
        } else {
            /// returned file is empty
            kDebug() << "No hits found in" << reply->url().toString();
            emit stoppedSearch(resultNoError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
