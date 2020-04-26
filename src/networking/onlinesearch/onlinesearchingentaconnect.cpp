/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QLineEdit>
#include <QIcon>
#endif // HAVE_QTWIDGETS
#include <QNetworkReply>
#include <QUrlQuery>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KConfigGroup>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#include <File>
#include <Entry>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "onlinesearchabstract_p.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
class OnlineSearchIngentaConnect::Form : public OnlineSearchAbstract::Form
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(d->config, configGroupName);
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
    QLineEdit *lineEditFullText;
    QLineEdit *lineEditTitle;
    QLineEdit *lineEditAuthor;
    QLineEdit *lineEditAbstractKeywords;
    QLineEdit *lineEditPublication;
    QLineEdit *lineEditISSNDOIISBN;
    QLineEdit *lineEditVolume;
    QLineEdit *lineEditIssue;
    QSpinBox *numResultsField;

    Form(QWidget *widget)
            : OnlineSearchAbstract::Form(widget), configGroupName(QStringLiteral("Search Engine IngentaConnect")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setMargin(0);

        lineEditFullText = new QLineEdit(this);
        lineEditFullText->setClearButtonEnabled(true);
        layout->addRow(i18n("Full text:"), lineEditFullText);
        connect(lineEditFullText, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditTitle = new QLineEdit(this);
        lineEditTitle->setClearButtonEnabled(true);
        layout->addRow(i18n("Title:"), lineEditTitle);
        connect(lineEditTitle, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditAuthor = new QLineEdit(this);
        lineEditAuthor->setClearButtonEnabled(true);
        layout->addRow(i18n("Author:"), lineEditAuthor);
        connect(lineEditAuthor, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditAbstractKeywords = new QLineEdit(this);
        lineEditAbstractKeywords->setClearButtonEnabled(true);
        layout->addRow(i18n("Abstract/Keywords:"), lineEditAbstractKeywords);
        connect(lineEditAbstractKeywords, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditPublication = new QLineEdit(this);
        lineEditPublication->setClearButtonEnabled(true);
        layout->addRow(i18n("Publication:"), lineEditPublication);
        connect(lineEditPublication, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditISSNDOIISBN = new QLineEdit(this);
        lineEditISSNDOIISBN->setClearButtonEnabled(true);
        layout->addRow(i18n("ISSN/ISBN/DOI:"), lineEditISSNDOIISBN);
        connect(lineEditISSNDOIISBN, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditVolume = new QLineEdit(this);
        lineEditVolume->setClearButtonEnabled(true);
        layout->addRow(i18n("Volume:"), lineEditVolume);
        connect(lineEditVolume, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

        lineEditIssue = new QLineEdit(this);
        lineEditIssue->setClearButtonEnabled(true);
        layout->addRow(i18n("Issue/Number:"), lineEditIssue);
        connect(lineEditIssue, &QLineEdit::returnPressed, this, &OnlineSearchIngentaConnect::Form::returnPressed);

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
        lineEditAuthor->setText(d->authorLastNames(entry).join(QStringLiteral(" ")));
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
        lineEditAbstractKeywords->setText(QString());
    }

    void saveState() {
        KConfigGroup configGroup(d->config, configGroupName);
        configGroup.writeEntry(QStringLiteral("fullText"), lineEditFullText->text());
        configGroup.writeEntry(QStringLiteral("title"), lineEditTitle->text());
        configGroup.writeEntry(QStringLiteral("author"), lineEditAuthor->text());
        configGroup.writeEntry(QStringLiteral("abstractKeywords"), lineEditAbstractKeywords->text());
        configGroup.writeEntry(QStringLiteral("publication"), lineEditPublication->text());
        configGroup.writeEntry(QStringLiteral("ISSNDOIISBN"), lineEditISSNDOIISBN->text());
        configGroup.writeEntry(QStringLiteral("volume"), lineEditVolume->text());
        configGroup.writeEntry(QStringLiteral("issue"), lineEditIssue->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        d->config->sync();
    }
};
#endif // HAVE_QTWIDGETS

class OnlineSearchIngentaConnect::OnlineSearchIngentaConnectPrivate
{
private:
    const QString ingentaConnectBaseUrl;

public:
#ifdef HAVE_QTWIDGETS
    OnlineSearchIngentaConnect::Form *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchIngentaConnectPrivate(OnlineSearchIngentaConnect *)
            : ingentaConnectBaseUrl(QStringLiteral("https://www.ingentaconnect.com/search?format=bib"))
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

    QUrl buildQueryUrl(const QMap<QueryKey, QString> &query, int numResults) {
        QUrl queryUrl(ingentaConnectBaseUrl);
        QUrlQuery q(queryUrl);

        int index = 1;
        const QStringList chunksFreeText = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::FreeText]);
        for (const QString &chunk : chunksFreeText) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("fulltext"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksAuthor = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Author]);
        for (const QString &chunk : chunksAuthor) {
            if (index > 1)
                q.addQueryItem(QString(QStringLiteral("operator%1")).arg(index), QStringLiteral("AND"));
            q.addQueryItem(QString(QStringLiteral("option%1")).arg(index), QStringLiteral("author"));
            q.addQueryItem(QString(QStringLiteral("value%1")).arg(index), chunk);
            ++index;
        }

        const QStringList chunksTitle = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Title]);
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

void OnlineSearchIngentaConnect::startSearch(const QMap<QueryKey, QString> &query, int numResults)
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
#ifdef HAVE_KF5
    return i18n("IngentaConnect");
#else // HAVE_KF5
    //= onlinesearch-ingentaconnect-label
    return QObject::tr("IngentaConnect");
#endif // HAVE_KF5
}

#ifdef HAVE_QTWIDGETS
OnlineSearchAbstract::Form *OnlineSearchIngentaConnect::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchIngentaConnect::Form(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchIngentaConnect::homepage() const
{
    return QUrl(QStringLiteral("https://www.ingentaconnect.com/"));
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
