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

#include <QTextStream>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QNetworkReply>

#include <KLineEdit>
#include <KLocale>
#include <KDebug>
#include <KStandardDirs>
#include <KMessageBox>
#include <KConfigGroup>

#include "fileimporterbibtex.h"
#include "websearcharxiv.h"
#include "xsltransform.h"


class WebSearchArXiv::WebSearchQueryFormArXiv : public WebSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QLatin1String("freeText"), QString()));
        numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
    }

public:
    KLineEdit *lineEditFreeText;
    QSpinBox *numResultsField;

    WebSearchQueryFormArXiv(QWidget *parent)
            : WebSearchQueryFormAbstract(parent), configGroupName(QLatin1String("Search Engine arXiv.org")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("Free text:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditFreeText = new KLineEdit(this);
        lineEditFreeText->setClearButtonShown(true);
        lineEditFreeText->setFocus(Qt::TabFocusReason);
        layout->addWidget(lineEditFreeText, 0, 1, 1, 1);
        label->setBuddy(lineEditFreeText);
        connect(lineEditFreeText, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("Number of Results:"), this);
        layout->addWidget(label, 1, 0, 1, 1);
        numResultsField = new QSpinBox(this);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);
        layout->addWidget(numResultsField, 1, 1, 1, 1);
        label->setBuddy(numResultsField);

        layout->setRowStretch(2, 100);

        loadState();
    }

    bool readyToStart() const {
        return !lineEditFreeText->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) {
        lineEditFreeText->setText(authorLastNames(entry).join(" ") + " " + PlainTextValue::text(entry[Entry::ftTitle]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("freeText"), lineEditFreeText->text());
        configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
        config->sync();
    }
};

class WebSearchArXiv::WebSearchArXivPrivate
{
private:
    WebSearchArXiv *p;

public:
    XSLTransform xslt;
    WebSearchQueryFormArXiv *form;
    const QString arXivQueryBaseUrl;
    int numSteps, curStep;

    WebSearchArXivPrivate(WebSearchArXiv *parent)
            : p(parent), xslt(KStandardDirs::locate("appdata", "arxiv2bibtex.xsl")),
            form(NULL), arXivQueryBaseUrl("http://export.arxiv.org/api/query?") {
        // nothing
    }

    KUrl buildQueryUrl() {
        /// format search terms
        QStringList queryFragments;

        foreach(QString queryFragment, p->splitRespectingQuotationMarks(form->lineEditFreeText->text()))
        queryFragments.append(p->encodeURL(queryFragment));
        return KUrl(QString("%1search_query=all:\"%3\"&start=0&max_results=%2").arg(arXivQueryBaseUrl).arg(form->numResultsField->value()).arg(queryFragments.join("\"+AND+all:\"")));
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        /// format search terms
        QStringList queryFragments;
        for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it)
            foreach(QString queryFragment, p->splitRespectingQuotationMarks(it.value()))
            queryFragments.append(p->encodeURL(queryFragment));
        return KUrl(QString("%1search_query=all:\"%3\"&start=0&max_results=%2").arg(arXivQueryBaseUrl).arg(numResults).arg(queryFragments.join("\"+AND+all:\"")));
    }
};

WebSearchArXiv::WebSearchArXiv(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchArXiv::WebSearchArXivPrivate(this))
{
    // nothing
}

void WebSearchArXiv::startSearch()
{
    d->curStep = 0;
    d->numSteps = 1;
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildQueryUrl());
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);

    d->form->saveState();
}

void WebSearchArXiv::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->curStep = 0;
    d->numSteps = 1;
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    setSuggestedHttpHeaders(request);
    QNetworkReply *reply = networkAccessManager()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);
}

QString WebSearchArXiv::label() const
{
    return i18n("arXiv.org");
}

QString WebSearchArXiv::favIconUrl() const
{
    return QLatin1String("http://arxiv.org/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchArXiv::customWidget(QWidget *parent)
{
    return (d->form = new WebSearchArXiv::WebSearchQueryFormArXiv(parent));
}

KUrl WebSearchArXiv::homepage() const
{
    return KUrl("http://arxiv.org/");
}

void WebSearchArXiv::cancel()
{
    WebSearchAbstract::cancel();
}

void WebSearchArXiv::downloadDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString result = reply->readAll();
        result = result.replace("xmlns=\"http://www.w3.org/2005/Atom\"", ""); // FIXME fix arxiv2bibtex.xsl to handle namespace

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = d->xslt.transform(result).replace(QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), QString());

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    emit foundEntry(entry);
                    hasEntries = true;
                }

            }

            if (!hasEntries)
                kDebug() << "No hits found in" << reply->url().toString();
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);

            delete bibtexFile;
        } else {
            kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
