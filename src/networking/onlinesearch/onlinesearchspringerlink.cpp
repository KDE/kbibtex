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

#include "onlinesearchspringerlink.h"

#ifdef HAVE_QTWIDGETS
#include <QFormLayout>
#include <QSpinBox>
#include <QLabel>
#endif // HAVE_QTWIDGETS
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#ifdef HAVE_KF5
#include <KLocalizedString>
#endif // HAVE_KF5
#ifdef HAVE_QTWIDGETS
#include <KLineEdit>
#include <KConfigGroup>
#endif // HAVE_QTWIDGETS

#include "internalnetworkaccessmanager.h"
#include "encoderlatex.h"
#include "encoderxml.h"
#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class OnlineSearchSpringerLink::OnlineSearchQueryFormSpringerLink : public OnlineSearchQueryFormAbstract
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QStringLiteral("free"), QString()));
        lineEditTitle->setText(configGroup.readEntry(QStringLiteral("title"), QString()));
        lineEditBookTitle->setText(configGroup.readEntry(QStringLiteral("bookTitle"), QString()));
        lineEditAuthorEditor->setText(configGroup.readEntry(QStringLiteral("authorEditor"), QString()));
        lineEditYear->setText(configGroup.readEntry(QStringLiteral("year"), QString()));
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

public:
    KLineEdit *lineEditFreeText, *lineEditTitle, *lineEditBookTitle, *lineEditAuthorEditor, *lineEditYear;
    QSpinBox *numResultsField;

    OnlineSearchQueryFormSpringerLink(QWidget *parent)
            : OnlineSearchQueryFormAbstract(parent), configGroupName(QStringLiteral("Search Engine SpringerLink")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setMargin(0);

        lineEditFreeText = new KLineEdit(this);
        lineEditFreeText->setClearButtonEnabled(true);
        QLabel *label = new QLabel(i18n("Free Text:"), this);
        label->setBuddy(lineEditFreeText);
        layout->addRow(label, lineEditFreeText);
        connect(lineEditFreeText, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormSpringerLink::returnPressed);

        lineEditTitle = new KLineEdit(this);
        lineEditTitle->setClearButtonEnabled(true);
        label = new QLabel(i18n("Title:"), this);
        label->setBuddy(lineEditTitle);
        layout->addRow(label, lineEditTitle);
        connect(lineEditTitle, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormSpringerLink::returnPressed);

        lineEditBookTitle = new KLineEdit(this);
        lineEditBookTitle->setClearButtonEnabled(true);
        label = new QLabel(i18n("Book/Journal title:"), this);
        label->setBuddy(lineEditBookTitle);
        layout->addRow(label, lineEditBookTitle);
        connect(lineEditBookTitle, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormSpringerLink::returnPressed);

        lineEditAuthorEditor = new KLineEdit(this);
        lineEditAuthorEditor->setClearButtonEnabled(true);
        label = new QLabel(i18n("Author or Editor:"), this);
        label->setBuddy(lineEditAuthorEditor);
        layout->addRow(label, lineEditAuthorEditor);
        connect(lineEditAuthorEditor, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormSpringerLink::returnPressed);

        lineEditYear = new KLineEdit(this);
        lineEditYear->setClearButtonEnabled(true);
        label = new QLabel(i18n("Year:"), this);
        label->setBuddy(lineEditYear);
        layout->addRow(label, lineEditYear);
        connect(lineEditYear, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormSpringerLink::returnPressed);

        numResultsField = new QSpinBox(this);
        label = new QLabel(i18n("Number of Results:"), this);
        label->setBuddy(numResultsField);
        layout->addRow(label, numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);

        lineEditFreeText->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const override {
        return !(lineEditFreeText->text().isEmpty() && lineEditTitle->text().isEmpty() && lineEditBookTitle->text().isEmpty() && lineEditAuthorEditor->text().isEmpty());
    }

    void copyFromEntry(const Entry &entry) override {
        lineEditTitle->setText(PlainTextValue::text(entry[Entry::ftTitle]));
        QString bookTitle = PlainTextValue::text(entry[Entry::ftBookTitle]);
        if (bookTitle.isEmpty())
            bookTitle = PlainTextValue::text(entry[Entry::ftJournal]);
        lineEditBookTitle->setText(bookTitle);
        lineEditAuthorEditor->setText(authorLastNames(entry).join(QStringLiteral(" ")));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("free"), lineEditFreeText->text());
        configGroup.writeEntry(QStringLiteral("title"), lineEditTitle->text());
        configGroup.writeEntry(QStringLiteral("bookTitle"), lineEditBookTitle->text());
        configGroup.writeEntry(QStringLiteral("authorEditor"), lineEditAuthorEditor->text());
        configGroup.writeEntry(QStringLiteral("year"), lineEditYear->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        config->sync();
    }
};
#endif // HAVE_QTWIDGETS

class OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate
{
private:
    static const QString xsltFilenameBase;

public:
    static const QString springerMetadataKey;
    const XSLTransform xslt;
#ifdef HAVE_QTWIDGETS
    OnlineSearchQueryFormSpringerLink *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchSpringerLinkPrivate(OnlineSearchSpringerLink *)
            : xslt(XSLTransform::locateXSLTfile(xsltFilenameBase))
#ifdef HAVE_QTWIDGETS
        , form(nullptr)
#endif // HAVE_QTWIDGETS
    {
        if (!xslt.isValid())
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to initialize XSL transformation based on file '" << xsltFilenameBase << "'";
    }

#ifdef HAVE_QTWIDGETS
    QUrl buildQueryUrl() {
        if (form == nullptr) return QUrl();

        QUrl queryUrl = QUrl(QString(QStringLiteral("http://api.springer.com/metadata/pam/?api_key=")).append(springerMetadataKey));

        QString queryString = form->lineEditFreeText->text();

        const QStringList titleChunks = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditTitle->text());
        for (const QString &titleChunk : titleChunks) {
            queryString += QString(QStringLiteral(" title:%1")).arg(EncoderLaTeX::instance().convertToPlainAscii(titleChunk));
        }

        const QStringList bookTitleChunks = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditBookTitle->text());
        for (const QString &titleChunk : bookTitleChunks) {
            queryString += QString(QStringLiteral(" ( journal:%1 OR book:%1 )")).arg(EncoderLaTeX::instance().convertToPlainAscii(titleChunk));
        }

        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditAuthorEditor->text());
        for (const QString &author : authors) {
            queryString += QString(QStringLiteral(" name:%1")).arg(EncoderLaTeX::instance().convertToPlainAscii(author));
        }

        const QString year = form->lineEditYear->text();
        if (!year.isEmpty())
            queryString += QString(QStringLiteral(" year:%1")).arg(year);

        queryString = queryString.simplified();
        QUrlQuery query(queryUrl);
        query.addQueryItem(QStringLiteral("q"), queryString);
        queryUrl.setQuery(query);

        return queryUrl;
    }
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QString, QString> &query) {
        QUrl queryUrl = QUrl(QString(QStringLiteral("http://api.springer.com/metadata/pam/?api_key=")).append(springerMetadataKey));

        QString queryString = query[queryKeyFreeText];

        const QStringList titleChunks = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyTitle]);
        for (const QString &titleChunk : titleChunks) {
            queryString += QString(QStringLiteral(" title:%1")).arg(EncoderLaTeX::instance().convertToPlainAscii(titleChunk));
        }

        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (const QString &author : authors) {
            queryString += QString(QStringLiteral(" name:%1")).arg(EncoderLaTeX::instance().convertToPlainAscii(author));
        }

        QString year = query[queryKeyYear];
        if (!year.isEmpty()) {
            static const QRegularExpression yearRegExp("\\b(18|19|20)[0-9]{2}\\b");
            const QRegularExpressionMatch yearRegExpMatch = yearRegExp.match(year);
            if (yearRegExpMatch.hasMatch()) {
                year = yearRegExpMatch.captured(0);
                queryString += QString(QStringLiteral(" year:%1")).arg(year);
            }
        }

        queryString = queryString.simplified();
        QUrlQuery q(queryUrl);
        q.addQueryItem(QStringLiteral("q"), queryString);
        queryUrl.setQuery(q);

        return queryUrl;
    }
};

const QString OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate::xsltFilenameBase = QStringLiteral("pam2bibtex.xsl");
const QString OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate::springerMetadataKey(InternalNetworkAccessManager::reverseObfuscate("\xce\xb8\x4d\x2c\x8d\xba\xa9\xc4\x61\x9\x58\x6c\xbb\xde\x86\xb5\xb1\xc6\x15\x71\x76\x45\xd\x79\x12\x65\x95\xe1\x5d\x2f\x1d\x24\x10\x72\x2a\x5e\x69\x4\xdc\xba\xab\xc3\x28\x58\x8a\xfa\x5e\x69"));


OnlineSearchSpringerLink::OnlineSearchSpringerLink(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate(this))
{
    /// nothing
}

OnlineSearchSpringerLink::~OnlineSearchSpringerLink()
{
    delete d;
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchSpringerLink::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QUrl springerLinkSearchUrl = d->buildQueryUrl();

    QNetworkRequest request(springerLinkSearchUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchSpringerLink::doneFetchingPAM);

    if (d->form != nullptr) d->form->saveState();

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchSpringerLink::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    QUrl springerLinkSearchUrl = d->buildQueryUrl(query);
    QUrlQuery q(springerLinkSearchUrl);
    q.addQueryItem(QStringLiteral("p"), QString::number(numResults));
    springerLinkSearchUrl.setQuery(q);

    emit progress(curStep = 0, numSteps = 1);
    QNetworkRequest request(springerLinkSearchUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchSpringerLink::doneFetchingPAM);

    refreshBusyProperty();
}

QString OnlineSearchSpringerLink::label() const
{
    return i18n("SpringerLink");
}

QString OnlineSearchSpringerLink::favIconUrl() const
{
    return QStringLiteral("http://link.springer.com/static/0.6623/sites/link/images/favicon.ico");
}

#ifdef HAVE_QTWIDGETS
OnlineSearchQueryFormAbstract *OnlineSearchSpringerLink::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchQueryFormSpringerLink(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchSpringerLink::homepage() const
{
    return QUrl(QStringLiteral("http://www.springerlink.com/"));
}

void OnlineSearchSpringerLink::doneFetchingPAM()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        const QString xmlSource = QString::fromUtf8(reply->readAll().constData());

        const QString bibTeXcode = EncoderXML::instance().decode(d->xslt.transform(xmlSource).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")));
        if (bibTeXcode.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultInvalidArguments);
        } else {
            FileImporterBibTeX importer(this);
            const File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != nullptr) {
                for (const QSharedPointer<Element> &element : *bibtexFile) {
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

    refreshBusyProperty();
}

#include "onlinesearchspringerlink.moc"
