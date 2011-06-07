/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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
#include <QNetworkCookieJar>
#include <QNetworkCookie>

#include <KLocale>
#include <KDebug>
#include <KLineEdit>
#include <KConfigGroup>

#include <fileimporterbibtex.h>
#include "websearchspringerlink.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class WebSearchSpringerLink::WebSearchQueryFormSpringerLink : public WebSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QLatin1String("free"), QString()));
        lineEditAuthorEditor->setText(configGroup.readEntry(QLatin1String("authorEditor"), QString()));
        lineEditPublication->setText(configGroup.readEntry(QLatin1String("publication"), QString()));
        lineEditVolume->setText(configGroup.readEntry(QLatin1String("volume"), QString()));
        lineEditIssue->setText(configGroup.readEntry(QLatin1String("issue"), QString()));
        numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
    }

public:
    KLineEdit *lineEditFreeText, *lineEditAuthorEditor, *lineEditPublication, *lineEditVolume, *lineEditIssue;
    QSpinBox *numResultsField;

    WebSearchQueryFormSpringerLink(QWidget *parent)
            : WebSearchQueryFormAbstract(parent), configGroupName(QLatin1String("Search Engine SpringerLink")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setMargin(0);

        lineEditFreeText = new KLineEdit(this);
        lineEditFreeText->setClearButtonShown(true);
        QLabel *label = new QLabel(i18n("Free Text:"), this);
        label->setBuddy(lineEditFreeText);
        layout->addRow(label, lineEditFreeText);
        connect(lineEditFreeText, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditAuthorEditor = new KLineEdit(this);
        lineEditAuthorEditor->setClearButtonShown(true);
        label = new QLabel(i18n("Author or Editor:"), this);
        label->setBuddy(lineEditAuthorEditor);
        layout->addRow(label, lineEditAuthorEditor);
        connect(lineEditAuthorEditor, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditPublication = new KLineEdit(this);
        lineEditPublication->setClearButtonShown(true);
        label = new QLabel(i18n("Publication:"), this);
        label->setBuddy(lineEditPublication);
        layout->addRow(label, lineEditPublication);
        connect(lineEditPublication, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditVolume = new KLineEdit(this);
        lineEditVolume->setClearButtonShown(true);
        label = new QLabel(i18n("Volume:"), this);
        label->setBuddy(lineEditVolume);
        layout->addRow(label, lineEditVolume);
        connect(lineEditVolume, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        lineEditIssue = new KLineEdit(this);
        lineEditIssue->setClearButtonShown(true);
        label = new QLabel(i18n("Issue:"), this);
        label->setBuddy(lineEditIssue);
        layout->addRow(label, lineEditIssue);
        connect(lineEditIssue, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        numResultsField = new QSpinBox(this);
        label = new QLabel(i18n("Number of Results:"), this);
        label->setBuddy(numResultsField);
        layout->addRow(label, numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);

        lineEditFreeText->setFocus(Qt::TabFocusReason);

        loadState();
    }

    virtual bool readyToStart() const {
        return !(lineEditFreeText->text().isEmpty() && lineEditAuthorEditor->text().isEmpty() && lineEditPublication->text().isEmpty() && lineEditVolume->text().isEmpty() && lineEditIssue->text().isEmpty());
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("free"), lineEditFreeText->text());
        configGroup.writeEntry(QLatin1String("authorEditor"), lineEditAuthorEditor->text());
        configGroup.writeEntry(QLatin1String("publication"), lineEditPublication->text());
        configGroup.writeEntry(QLatin1String("volume"), lineEditVolume->text());
        configGroup.writeEntry(QLatin1String("issue"), lineEditIssue->text());
        configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
        config->sync();
    }
};

class WebSearchSpringerLink::WebSearchSpringerLinkPrivate
{
private:
    WebSearchSpringerLink *p;

public:
    const QString springerLinkBaseUrl;
    const QString springerLinkQueryUrl;
    KUrl springerLinkSearchUrl;
    int numExpectedResults, numFoundResults;
    int runningJobs;
    int currentSearchPosition;
    QStringList articleUrls;
    WebSearchQueryFormSpringerLink *form;

    WebSearchSpringerLinkPrivate(WebSearchSpringerLink *parent)
            : p(parent), springerLinkBaseUrl(QLatin1String("http://www.springerlink.com")), springerLinkQueryUrl(QLatin1String("http://www.springerlink.com/content/")), runningJobs(0), form(NULL) {
        // nothing
    }

    KUrl& buildQueryUrl(KUrl& url) {
        Q_ASSERT(form != NULL);

        // FIXME encode for URL
        QString queryString(form->lineEditFreeText->text());

        QStringList authors = p->splitRespectingQuotationMarks(form->lineEditAuthorEditor->text());
        foreach(QString author, authors) {
            queryString += QString(QLatin1String(" ( au:(%1) OR ed:(%1) )")).arg(author);
        }

        if (!form->lineEditPublication->text().isEmpty())
            queryString += QString(QLatin1String(" pub:(%1)")).arg(form->lineEditPublication->text());

        if (!form->lineEditVolume->text().isEmpty())
            queryString += QString(QLatin1String(" vol:(%1)")).arg(form->lineEditVolume->text());

        if (!form->lineEditIssue->text().isEmpty())
            queryString += QString(QLatin1String(" iss:(%1)")).arg(form->lineEditIssue->text());

        queryString = queryString.trimmed();
        url.addQueryItem(QLatin1String("k"), queryString);

        return url;
    }

    KUrl& buildQueryUrl(KUrl& url, const QMap<QString, QString> &query) {
        // FIXME encode for URL
        QString queryString = query[queryKeyFreeText] + ' ' + query[queryKeyTitle] + ' ' + query[queryKeyYear];

        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(QString author, authors) {
            queryString += QString(QLatin1String(" ( au:(%1) OR ed:(%1) )")).arg(author);
        }

        queryString = queryString.trimmed();
        url.addQueryItem(QLatin1String("k"), queryString);

        return url;
    }

    void sanitizeBibTeXCode(QString &code) {
        /// DOI is "hidden" in a "note" field, rename field to "DOI"
        code = code.replace(QLatin1String("note = {10."), QLatin1String("doi = {10."));
    }
};


WebSearchSpringerLink::WebSearchSpringerLink(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchSpringerLink::WebSearchSpringerLinkPrivate(this))
{
    // nothing
}

void WebSearchSpringerLink::startSearch()
{
    m_hasBeenCanceled = false;
    d->numFoundResults = 0;
    d->currentSearchPosition = 0;
    d->articleUrls.clear();

    d->numExpectedResults = d->form->numResultsField->value();

    ++d->runningJobs;
    d->springerLinkSearchUrl = KUrl(d->springerLinkQueryUrl);
    d->springerLinkSearchUrl = d->buildQueryUrl(d->springerLinkSearchUrl);

    QNetworkRequest request(d->springerLinkSearchUrl);
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));

    kDebug() << "url =" << request.url();
    foreach(QNetworkCookie cookie, networkAccessManager()->cookieJar()->cookiesForUrl(request.url())) {
        kDebug() << "cookie " << cookie.name() << "=" << cookie.value();
    }

    d->form->saveState();
}

void WebSearchSpringerLink::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->numFoundResults = 0;
    d->currentSearchPosition = 0;
    d->articleUrls.clear();

    d->numExpectedResults = numResults;

    ++d->runningJobs;
    d->springerLinkSearchUrl = KUrl(d->springerLinkQueryUrl);
    d->springerLinkSearchUrl = d->buildQueryUrl(d->springerLinkSearchUrl, query);

    QNetworkRequest request(d->springerLinkSearchUrl);
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));

    kDebug() << "url =" << request.url();
    foreach(QNetworkCookie cookie, networkAccessManager()->cookieJar()->cookiesForUrl(request.url())) {
        kDebug() << "cookie " << cookie.name() << "=" << cookie.value();
    }
}

QString WebSearchSpringerLink::label() const
{
    return i18n("SpringerLink");
}

QString WebSearchSpringerLink::favIconUrl() const
{
    return QLatin1String("http://www.springerlink.com/images/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchSpringerLink::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new WebSearchQueryFormSpringerLink(parent);
    return d->form;
}

KUrl WebSearchSpringerLink::homepage() const
{
    return KUrl("http://www.springerlink.com/");
}

void WebSearchSpringerLink::cancel()
{
    WebSearchAbstract::cancel();
}

void WebSearchSpringerLink::doneFetchingResultPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs == 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    kDebug() << "url =" << reply->url();
    foreach(QNetworkCookie cookie, networkAccessManager()->cookieJar()->cookiesForUrl(reply->url())) {
        kDebug() << "cookie " << cookie.name() << "=" << cookie.value();
    }
    if (handleErrors(reply)) {
        QString htmlSource = reply->readAll();
        int p1 = -1, p2;
        while ((p1 = htmlSource.indexOf(" data-code=\"", p1 + 1)) >= 0 && (p2 = htmlSource.indexOf("\"", p1 + 14)) >= 0) {
            QString datacode = htmlSource.mid(p1 + 12, p2 - p1 - 12).toLower();

            if (datacode.length() > 8 && d->numFoundResults < d->numExpectedResults) {
                ++d->numFoundResults;
                ++d->runningJobs;
                QString url = QString("http://www.springerlink.com/content/%1/export-citation/").arg(datacode);
                QNetworkRequest request(url);
                setSuggestedHttpHeaders(request);
                QNetworkReply *reply = networkAccessManager()->get(request);
                setNetworkReplyTimeout(reply);
                connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingExportPage()));
            }
        }

        d->currentSearchPosition += 10;
        if (d->numExpectedResults > d->currentSearchPosition) {
            KUrl url(d->springerLinkSearchUrl);
            url.addQueryItem(QLatin1String("o"), QString::number(d->currentSearchPosition));
            QNetworkRequest request(url);
            setSuggestedHttpHeaders(request);
            QNetworkReply *reply = networkAccessManager()->get(request);
            setNetworkReplyTimeout(reply);
            connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
            ++d->runningJobs;
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}


void WebSearchSpringerLink::doneFetchingExportPage()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);


    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = formParameters(reply->readAll(), QLatin1String("<form name=\"aspnetForm\""));
        inputMap.remove("ctl00$ContentPrimary$ctl00$ctl00$CitationManagerDropDownList");
        inputMap.remove("citation-type");
        inputMap.remove("ctl00$ctl19$goButton");
        inputMap.remove("ctl00$ctl19$SearchControl$AdvancedGoButton");
        inputMap.remove("ctl00$ctl19$SearchControl$AdvancedSearchButton");
        inputMap.remove("ctl00$ctl19$SearchControl$SearchTipsButton");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicAuthorOrEditorTextBox");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicGoButton");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicIssueTextBox");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicPageTextBox");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicPublicationTextBox");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicSearchForTextBox");
        inputMap.remove("ctl00$ctl19$SearchControl$BasicVolumeTextBox");

        QString body = encodeURL("ctl00$ContentPrimary$ctl00$ctl00$CitationManagerDropDownList") + "=BibTex&" + encodeURL("ctl00$ContentPrimary$ctl00$ctl00$Export") + "=AbstractRadioButton";
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            body += '&' + encodeURL(it.key()) + '=' + encodeURL(it.value());

        ++d->runningJobs;
        QNetworkRequest request(reply->url());
        setSuggestedHttpHeaders(request, reply);
        QNetworkReply *newReply = networkAccessManager()->post(request, body.toUtf8());
        setNetworkReplyTimeout(newReply);
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchSpringerLink::doneFetchingBibTeX()
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs >= 0);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        ts.setCodec("ISO-8859-1");
        QString bibTeXcode = ts.readAll();
        d->sanitizeBibTeXCode(bibTeXcode);

        kDebug() << "bibTeXcode =" << bibTeXcode;

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntry = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    hasEntry = true;
                    if (entry != NULL) {
                        Value v;
                        v.append(new VerbatimText(label()));
                        entry->insert("x-fetchedfrom", v);
                        emit foundEntry(entry);
                    }
                }
            }
            delete bibtexFile;
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(hasEntry ? resultNoError : resultUnspecifiedError);
    }  else
        kDebug() << "url was" << reply->url().toString();
}
