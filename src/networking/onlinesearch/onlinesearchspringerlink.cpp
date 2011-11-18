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

#include <KLocale>
#include <KDebug>
#include <KLineEdit>
#include <KConfigGroup>

#include <internalnetworkaccessmanager.h>
#include <fileimporterbibtex.h>
#include <encoderlatex.h>
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
        lineEditAuthorEditor->setText(configGroup.readEntry(QLatin1String("authorEditor"), QString()));
        lineEditPublication->setText(configGroup.readEntry(QLatin1String("publication"), QString()));
        lineEditVolume->setText(configGroup.readEntry(QLatin1String("volume"), QString()));
        lineEditIssue->setText(configGroup.readEntry(QLatin1String("issue"), QString()));
        spinBoxYearBegin->setValue(configGroup.readEntry(QLatin1String("yearBegin"), 1970));
        spinBoxYearEnd->setValue(configGroup.readEntry(QLatin1String("yearEnd"), 2015));
        numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
    }

public:
    KLineEdit *lineEditFreeText, *lineEditAuthorEditor, *lineEditPublication, *lineEditVolume, *lineEditIssue;
    QSpinBox *numResultsField, *spinBoxYearBegin, *spinBoxYearEnd;

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

        spinBoxYearBegin = new QSpinBox(this);
        label = new QLabel(i18n("Year (start):"), this);
        label->setBuddy(spinBoxYearBegin);
        layout->addRow(label, spinBoxYearBegin);
        spinBoxYearBegin->setMinimum(1800);
        spinBoxYearBegin->setMaximum(2030);
        spinBoxYearBegin->setValue(1970);

        spinBoxYearEnd = new QSpinBox(this);
        label = new QLabel(i18n("Year (end):"), this);
        label->setBuddy(spinBoxYearEnd);
        layout->addRow(label, spinBoxYearEnd);
        spinBoxYearEnd->setMinimum(1800);
        spinBoxYearEnd->setMaximum(2030);
        spinBoxYearEnd->setValue(2015);

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

    bool readyToStart() const {
        return !(lineEditFreeText->text().isEmpty() && lineEditAuthorEditor->text().isEmpty() && lineEditPublication->text().isEmpty() && lineEditVolume->text().isEmpty() && lineEditIssue->text().isEmpty());
    }

    void copyFromEntry(const Entry &entry) {
        lineEditFreeText->setText(PlainTextValue::text(entry[Entry::ftTitle]));
        lineEditAuthorEditor->setText(authorLastNames(entry).join(" "));
        lineEditPublication->setText(QString(PlainTextValue::text(entry[Entry::ftJournal]) + " " + PlainTextValue::text(entry[Entry::ftBookTitle])).simplified());
        lineEditVolume->setText(PlainTextValue::text(entry[Entry::ftVolume]));
        lineEditIssue->setText(PlainTextValue::text(entry[Entry::ftNumber]));
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

class OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate
{
private:
    OnlineSearchSpringerLink *p;

public:
    const QString springerLinkBaseUrl;
    const QString springerLinkQueryUrl;
    int numExpectedResults, numFoundResults;
    int currentSearchPosition;
    OnlineSearchQueryFormSpringerLink *form;
    int numSteps, curStep;
    QList<KUrl> queueResultPages, queueExportPages;
    QMap<KUrl, QString> queueBibTeX;

    OnlineSearchSpringerLinkPrivate(OnlineSearchSpringerLink *parent)
            : p(parent), springerLinkBaseUrl(QLatin1String("http://www.springerlink.com")), springerLinkQueryUrl(QLatin1String("http://www.springerlink.com/content/")), form(NULL) {
        // nothing
    }

    KUrl& buildQueryUrl(KUrl& url) {
        Q_ASSERT(form != NULL);

        // FIXME encode for URL
        QString queryString(form->lineEditFreeText->text());

        QStringList authors = p->splitRespectingQuotationMarks(form->lineEditAuthorEditor->text());
        foreach(QString author, authors) {
            author = EncoderLaTeX::instance()->convertToPlainAscii(author);
            queryString += QString(QLatin1String(" ( au:(%1) OR ed:(%1) )")).arg(author);
        }

        if (!form->lineEditPublication->text().isEmpty())
            queryString += QString(QLatin1String(" pub:(%1)")).arg(form->lineEditPublication->text());

        if (!form->lineEditVolume->text().isEmpty())
            queryString += QString(QLatin1String(" vol:(%1)")).arg(form->lineEditVolume->text());

        if (!form->lineEditIssue->text().isEmpty())
            queryString += QString(QLatin1String(" iss:(%1)")).arg(form->lineEditIssue->text());

        url.addQueryItem(QLatin1String("db"), QString::number(form->spinBoxYearBegin->value()) + QLatin1String("0101"));
        url.addQueryItem(QLatin1String("de"), QString::number(form->spinBoxYearEnd->value()) + QLatin1String("1231"));

        queryString = queryString.simplified();
        url.addQueryItem(QLatin1String("k"), queryString);

        return url;
    }

    KUrl& buildQueryUrl(KUrl& url, const QMap<QString, QString> &query) {
        // FIXME encode for URL
        QString queryString = query[queryKeyFreeText] + ' ' + query[queryKeyTitle];

        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(QString author, authors) {
            author = EncoderLaTeX::instance()->convertToPlainAscii(author);
            queryString += QString(QLatin1String(" ( au:(%1) OR ed:(%1) )")).arg(author);
        }

        QString year = query[queryKeyYear];
        if (!year.isEmpty()) {
            static const QRegExp yearRegExp("\\b(18|19|20)[0-9]{2}\\b");
            if (yearRegExp.indexIn(year) >= 0) {
                year = yearRegExp.cap(0);
                url.addQueryItem(QLatin1String("db"), year + QLatin1String("0101"));
                url.addQueryItem(QLatin1String("de"), year + QLatin1String("1231"));
            }
        }

        queryString = queryString.simplified();
        url.addQueryItem(QLatin1String("k"), queryString);

        return url;
    }

    void sanitizeBibTeXCode(QString &code) {
        /// DOI is "hidden" in a "note" field, rename field to "DOI"
        code = code.replace(QLatin1String("note = {10."), QLatin1String("doi = {10."));
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
    d->numFoundResults = 0;
    d->queueResultPages.clear();
    d->queueExportPages.clear();
    d->queueBibTeX.clear();
    d->numExpectedResults = d->form->numResultsField->value();
    d->curStep = 0;
    d->numSteps = d->numExpectedResults * 2 + 1 + d->numExpectedResults / 10;

    KUrl springerLinkSearchUrl = KUrl(d->springerLinkQueryUrl);
    springerLinkSearchUrl = d->buildQueryUrl(springerLinkSearchUrl);

    d->queueResultPages.append(springerLinkSearchUrl);
    for (int i = 10; i < d->numExpectedResults; i += 10) {
        KUrl url = springerLinkSearchUrl;
        url.addQueryItem(QLatin1String("o"), QString::number(i));
        d->queueResultPages.append(url);
    }

    emit progress(0, d->numSteps);
    processNextQueuedUrl();
    d->form->saveState();
}

void OnlineSearchSpringerLink::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->numFoundResults = 0;
    d->currentSearchPosition = 0;
    d->queueResultPages.clear();
    d->queueExportPages.clear();
    d->queueBibTeX.clear();
    d->numExpectedResults = numResults;
    d->curStep = 0;
    d->numSteps = d->numExpectedResults * 2 + 1 + d->numExpectedResults / 10;

    KUrl springerLinkSearchUrl = KUrl(d->springerLinkQueryUrl);
    springerLinkSearchUrl = d->buildQueryUrl(springerLinkSearchUrl, query);

    d->queueResultPages.append(springerLinkSearchUrl);
    for (int i = 10; i < numResults; i += 10) {
        KUrl url = springerLinkSearchUrl;
        url.addQueryItem(QLatin1String("o"), QString::number(i));
        d->queueResultPages.append(url);
    }

    emit progress(0, d->numSteps);
    processNextQueuedUrl();
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

void OnlineSearchSpringerLink::doneFetchingResultPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (handleErrors(reply)) {
        QString htmlSource = reply->readAll();
        int p1 = htmlSource.indexOf("div id=\"ContentPrimary"), p2 = -1;
        while (p1 >= 0 && (p1 = htmlSource.indexOf("class=\"title\"><a href=\"/content/", p1 + 1)) >= 0 && (p2 = htmlSource.indexOf("\"", p1 + 26)) >= 0) {
            QString datacode = htmlSource.mid(p1 + 32, p2 - p1 - 33).toLower();

            if (d->numFoundResults < d->numExpectedResults) {
                ++d->numFoundResults;
                QString url = QString("http://www.springerlink.com/content/%1/export-citation/").arg(datacode);
                d->queueExportPages.append(KUrl(url));
            }
        }

        processNextQueuedUrl();
    } else
        kDebug() << "url was" << reply->url().toString();
}


void OnlineSearchSpringerLink::doneFetchingExportPage()
{
    emit progress(++d->curStep, d->numSteps);

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

        d->queueBibTeX.insert(reply->url(), body);

        processNextQueuedUrl();
    } else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchSpringerLink::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        QString bibTeXcode = ts.readAll();
        d->sanitizeBibTeXCode(bibTeXcode);

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

        processNextQueuedUrl();
    }  else
        kDebug() << "url was" << reply->url().toString();
}

void OnlineSearchSpringerLink::processNextQueuedUrl()
{
    if (!d->queueBibTeX.isEmpty()) {
        QMap<KUrl, QString>::Iterator it = d->queueBibTeX.begin();
        KUrl url(it.key());
        QString body(it.value());
        d->queueBibTeX.erase(it);

        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->post(request, body.toUtf8());
        setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
    } else if (!d->queueExportPages.isEmpty()) {
        KUrl url = d->queueExportPages.first();
        d->queueExportPages.removeFirst();

        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingExportPage()));
    } else if (!d->queueResultPages.isEmpty()) {
        KUrl url = d->queueResultPages.first();
        d->queueResultPages.removeFirst();

        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
    } else {
        emit stoppedSearch(resultNoError);
        emit progress(d->numSteps, d->numSteps);
    }
}
