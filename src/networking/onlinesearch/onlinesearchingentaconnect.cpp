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

#include "onlinesearchingentaconnect.h"

#include <QBuffer>
#include <QLabel>
#include <QNetworkReply>
#include <QFormLayout>
#include <QSpinBox>
#include <QIcon>
#include <QUrlQuery>

#include <KLocalizedString>
#include <KLineEdit>
#include <KConfigGroup>

#include "file.h"
#include "entry.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchIngentaConnect::OnlineSearchQueryFormIngentaConnect : public OnlineSearchQueryFormAbstract
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFullText->setText(configGroup.readEntry(QStringLiteral("fullText"), QString()));
        lineEditTitle->setText(configGroup.readEntry(QStringLiteral("title"), QString()));
        lineEditAuthor->setText(configGroup.readEntry(QStringLiteral("author"), QString()));
        lineEditAbstractKeywords->setText(configGroup.readEntry(QStringLiteral("abstractKeywords"), QString()));
        lineEditPublication->setText(configGroup.readEntry(QStringLiteral("publication"), QString()));
        lineEditISSNDOIISBN->setText(configGroup.readEntry(QStringLiteral("ISSNDOIISBN"), QString()));
        lineEditVolume->setText(configGroup.readEntry(QStringLiteral("volume"), QString()));
        lineEditIssue->setText(configGroup.readEntry(QStringLiteral("issue"), QString()));
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
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
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QStringLiteral("Search Engine IngentaConnect")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setMargin(0);

        lineEditFullText = new KLineEdit(this);
        lineEditFullText->setClearButtonEnabled(true);
        layout->addRow(i18n("Full text:"), lineEditFullText);
        connect(lineEditFullText, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditTitle = new KLineEdit(this);
        lineEditTitle->setClearButtonEnabled(true);
        layout->addRow(i18n("Title:"), lineEditTitle);
        connect(lineEditTitle, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditAuthor = new KLineEdit(this);
        lineEditAuthor->setClearButtonEnabled(true);
        layout->addRow(i18n("Author:"), lineEditAuthor);
        connect(lineEditAuthor, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditAbstractKeywords = new KLineEdit(this);
        lineEditAbstractKeywords->setClearButtonEnabled(true);
        layout->addRow(i18n("Abstract/Keywords:"), lineEditAbstractKeywords);
        connect(lineEditAbstractKeywords, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditPublication = new KLineEdit(this);
        lineEditPublication->setClearButtonEnabled(true);
        layout->addRow(i18n("Publication:"), lineEditPublication);
        connect(lineEditPublication, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditISSNDOIISBN = new KLineEdit(this);
        lineEditISSNDOIISBN->setClearButtonEnabled(true);
        layout->addRow(i18n("ISSN/ISBN/DOI:"), lineEditISSNDOIISBN);
        connect(lineEditISSNDOIISBN, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditVolume = new KLineEdit(this);
        lineEditVolume->setClearButtonEnabled(true);
        layout->addRow(i18n("Volume:"), lineEditVolume);
        connect(lineEditVolume, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditIssue = new KLineEdit(this);
        lineEditIssue->setClearButtonEnabled(true);
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
        lineEditAuthor->setText(authorLastNames(entry).join(QStringLiteral(" ")));
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
        lineEditAbstractKeywords->setText(QStringLiteral(""));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("fullText"), lineEditFullText->text());
        configGroup.writeEntry(QStringLiteral("title"), lineEditTitle->text());
        configGroup.writeEntry(QStringLiteral("author"), lineEditAuthor->text());
        configGroup.writeEntry(QStringLiteral("abstractKeywords"), lineEditAbstractKeywords->text());
        configGroup.writeEntry(QStringLiteral("publication"), lineEditPublication->text());
        configGroup.writeEntry(QStringLiteral("ISSNDOIISBN"), lineEditISSNDOIISBN->text());
        configGroup.writeEntry(QStringLiteral("volume"), lineEditVolume->text());
        configGroup.writeEntry(QStringLiteral("issue"), lineEditIssue->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
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
            : p(parent), ingentaConnectBaseUrl(QStringLiteral("http://www.ingentaconnect.com/search?format=bib")), form(NULL) {
        // nothing
    }

    QUrl buildQueryUrl() {
        if (form == NULL) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot build query url if no form is specified";
            return QUrl();
        }

        QUrl queryUrl(ingentaConnectBaseUrl);
        QUrlQuery query(queryUrl);

        int index = 1;
        QStringList chunks = p->splitRespectingQuotationMarks(form->lineEditFullText->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND")); ///< join search terms with an AND operation
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("fulltext"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditAuthor->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("author"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditTitle->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("title"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditPublication->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("journalbooktitle"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditIssue->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("issue"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditVolume->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("volume"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(form->lineEditAbstractKeywords->text());
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("tka"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        query.addQueryItem(QStringLiteral("pageSize"), QString::number(form->numResultsField->value()));

        query.addQueryItem(QStringLiteral("sortDescending"), QStringLiteral("true"));
        query.addQueryItem(QStringLiteral("subscribed"), QStringLiteral("false"));
        query.addQueryItem(QStringLiteral("sortField"), QStringLiteral("default"));

        queryUrl.setQuery(query);
        return queryUrl;
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QUrl queryUrl(ingentaConnectBaseUrl);
        QUrlQuery q(queryUrl);

        int index = 1;
        QStringList chunks = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("fulltext"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("author"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        chunks = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        foreach (const QString &chunk, chunks) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("title"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        /// Field "year" not supported in IngentaConnect's search

        q.addQueryItem(QStringLiteral("pageSize"), QString::number(numResults));

        q.addQueryItem(QStringLiteral("sortDescending"), QStringLiteral("true"));
        q.addQueryItem(QStringLiteral("subscribed"), QStringLiteral("false"));
        q.addQueryItem(QStringLiteral("sortField"), QStringLiteral("default"));

        queryUrl.setQuery(q);
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
    return QStringLiteral("http://www.ingentaconnect.com/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchIngentaConnect::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new OnlineSearchIngentaConnect::OnlineSearchQueryFormIngentaConnect(parent);
    return d->form;
}

QUrl OnlineSearchIngentaConnect::homepage() const
{
    return QUrl(QStringLiteral("http://www.ingentaconnect.com/"));
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
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    hasEntries |= publishEntry(entry);
                }

                emit stoppedSearch(resultNoError);

                delete bibtexFile;
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toString();
                emit stoppedSearch(resultUnspecifiedError);
            }
        } else {
            /// returned file is empty
            emit stoppedSearch(resultNoError);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

#include "onlinesearchingentaconnect.moc"
