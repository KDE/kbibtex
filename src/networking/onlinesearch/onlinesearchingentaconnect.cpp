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

#include "onlinesearchingentaconnect.h"

#include <QBuffer>
#ifdef HAVE_QTWIDGETS
#include <QLabel>
#include <QFormLayout>
#include <QSpinBox>
#include <QIcon>
#endif // HAVE_QTWIDGETS
#include <QNetworkReply>
#include <QUrlQuery>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KConfigGroup>
#endif // HAVE_KF5
#ifdef HAVE_QTWIDGETS
#include <KLineEdit>
#endif // HAVE_QTWIDGETS

#include "file.h"
#include "entry.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
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
        connect(lineEditFullText, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditTitle = new KLineEdit(this);
        lineEditTitle->setClearButtonEnabled(true);
        layout->addRow(i18n("Title:"), lineEditTitle);
        connect(lineEditTitle, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditAuthor = new KLineEdit(this);
        lineEditAuthor->setClearButtonEnabled(true);
        layout->addRow(i18n("Author:"), lineEditAuthor);
        connect(lineEditAuthor, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditAbstractKeywords = new KLineEdit(this);
        lineEditAbstractKeywords->setClearButtonEnabled(true);
        layout->addRow(i18n("Abstract/Keywords:"), lineEditAbstractKeywords);
        connect(lineEditAbstractKeywords, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditPublication = new KLineEdit(this);
        lineEditPublication->setClearButtonEnabled(true);
        layout->addRow(i18n("Publication:"), lineEditPublication);
        connect(lineEditPublication, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditISSNDOIISBN = new KLineEdit(this);
        lineEditISSNDOIISBN->setClearButtonEnabled(true);
        layout->addRow(i18n("ISSN/ISBN/DOI:"), lineEditISSNDOIISBN);
        connect(lineEditISSNDOIISBN, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditVolume = new KLineEdit(this);
        lineEditVolume->setClearButtonEnabled(true);
        layout->addRow(i18n("Volume:"), lineEditVolume);
        connect(lineEditVolume, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        lineEditIssue = new KLineEdit(this);
        lineEditIssue->setClearButtonEnabled(true);
        layout->addRow(i18n("Issue/Number:"), lineEditIssue);
        connect(lineEditIssue, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormIngentaConnect::returnPressed);

        numResultsField = new QSpinBox(this);
        layout->addRow(i18n("Number of Results:"), numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(10);
    }

    bool readyToStart() const override {
        return !(lineEditFullText->text().isEmpty() && lineEditTitle->text().isEmpty() && lineEditAuthor->text().isEmpty() && lineEditAbstractKeywords->text().isEmpty() && lineEditPublication->text().isEmpty() && lineEditISSNDOIISBN->text().isEmpty() && lineEditVolume->text().isEmpty() && lineEditIssue->text().isEmpty());
    }

    void copyFromEntry(const Entry &entry) override {
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
#endif // HAVE_QTWIDGETS

class OnlineSearchIngentaConnect::OnlineSearchIngentaConnectPrivate
{
private:
    const QString ingentaConnectBaseUrl;

public:
#ifdef HAVE_QTWIDGETS
    OnlineSearchQueryFormIngentaConnect *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchIngentaConnectPrivate(OnlineSearchIngentaConnect *)
            : ingentaConnectBaseUrl(QStringLiteral("http://www.ingentaconnect.com/search?format=bib"))
#ifdef HAVE_QTWIDGETS
        , form(nullptr)
#endif // HAVE_QTWIDGETS
    {
        /// nothing
    }

#ifdef HAVE_QTWIDGETS
    QUrl buildQueryUrl() {
        if (form == nullptr) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot build query url if no form is specified";
            return QUrl();
        }

        QUrl queryUrl(ingentaConnectBaseUrl);
        QUrlQuery query(queryUrl);

        int index = 1;
        const QStringList chunksFullText = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditFullText->text());
        for (const QString &chunk : chunksFullText) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND")); ///< join search terms with an AND operation
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("fulltext"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksAuthor = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditAuthor->text());
        for (const QString &chunk : chunksAuthor) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("author"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksTitle = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditTitle->text());
        for (const QString &chunk : chunksTitle) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("title"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksPublication = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditPublication->text());
        for (const QString &chunk : chunksPublication) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("journalbooktitle"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksIssue = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditIssue->text());
        for (const QString &chunk : chunksIssue) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("issue"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksVolume = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditVolume->text());
        for (const QString &chunk : chunksVolume) {
            if (index > 1)
                query.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            query.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("volume"));
            query.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksKeywords = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditAbstractKeywords->text());
        for (const QString &chunk : chunksKeywords) {
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
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QUrl queryUrl(ingentaConnectBaseUrl);
        QUrlQuery q(queryUrl);

        int index = 1;
        const QStringList chunksFreeText = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyFreeText]);
        for (const QString &chunk : chunksFreeText) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("fulltext"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksAuthor = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (const QString &chunk : chunksAuthor) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("author"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksTitle = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyTitle]);
        for (const QString &chunk : chunksTitle) {
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

OnlineSearchIngentaConnect::OnlineSearchIngentaConnect(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchIngentaConnectPrivate(this))
{
    /// nothing
}

OnlineSearchIngentaConnect::~OnlineSearchIngentaConnect()
{
    delete d;
}

void OnlineSearchIngentaConnect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchIngentaConnect::downloadDone);

    refreshBusyProperty();
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchIngentaConnect::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchIngentaConnect::downloadDone);

    d->form->saveState();

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

QString OnlineSearchIngentaConnect::label() const
{
    return i18n("IngentaConnect");
}

QString OnlineSearchIngentaConnect::favIconUrl() const
{
    return QStringLiteral("http://www.ingentaconnect.com/favicon.ico");
}

#ifdef HAVE_QTWIDGETS
OnlineSearchQueryFormAbstract *OnlineSearchIngentaConnect::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchIngentaConnect::OnlineSearchQueryFormIngentaConnect(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchIngentaConnect::homepage() const
{
    return QUrl(QStringLiteral("http://www.ingentaconnect.com/"));
}

void OnlineSearchIngentaConnect::downloadDone()
{
    emit progress(curStep = numSteps, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (!bibTeXcode.isEmpty()) {
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
        } else {
            /// returned file is empty
            stopSearch(resultNoError);
        }
    }

    refreshBusyProperty();
}

#include "onlinesearchingentaconnect.moc"
