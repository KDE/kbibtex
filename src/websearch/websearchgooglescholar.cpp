/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <QSpinBox>
#include <QLayout>
#include <QLabel>
#include <QFormLayout>
#include <QNetworkReply>
#include <QNetworkCookieJar>

#include <KLocale>
#include <KMessageBox>
#include <KDebug>
#include <KConfigGroup>
#include <KLineEdit>
#include <KIcon>

#include <fileimporterbibtex.h>
#include "websearchgooglescholar.h"
#include "cookiemanager.h"

FileImporterBibTeX importer;

class WebSearchGoogleScholar::WebSearchGoogleScholarPrivate
{
private:
    WebSearchGoogleScholar *p;

public:
    int numResults;
    QStringList listBibTeXurls;
    QString queryFreetext, queryAuthor, queryYear;
    QString startPageUrl;
    QString advancedSearchPageUrl;
    QString configPageUrl;
    QString setConfigPageUrl;
    QString queryPageUrl;

    WebSearchGoogleScholarPrivate(WebSearchGoogleScholar *parent)
            : p(parent) {
        startPageUrl = QLatin1String("http://scholar.google.com/");
        configPageUrl = QLatin1String("http://%1/scholar_preferences");
        setConfigPageUrl = QLatin1String("http://%1/scholar_setprefs");
        queryPageUrl = QLatin1String("http://%1/scholar");
    }

    QMap<QString, QString> formParameters(const QString &htmlText) {
        /// how to recognize HTML tags
        static const QString formTagBegin = QLatin1String("<form ");
        static const QString formTagEnd = QLatin1String("</form>");
        static const QString inputTagBegin = QLatin1String("<input");
        static const QString selectTagBegin = QLatin1String("<select ");
        static const QString selectTagEnd = QLatin1String("</select>");
        static const QString optionTagBegin = QLatin1String("<option ");
        /// regular expressions to test or retrieve attributes in HTML tags
        QRegExp inputTypeRegExp("<input[^>]+\\btype=[\"]?([^\" >\n\t]+)");
        QRegExp inputNameRegExp("<input[^>]+\\bname=[\"]?([^\" >\n\t]+)");
        QRegExp inputValueRegExp("<input[^>]+\\bvalue=([^\"][^ >\n\t]*|\"[^\"]*\")");
        QRegExp inputIsCheckedRegExp("<input[^>]* checked([> \t\n]|=[\"]?checked)");
        QRegExp selectNameRegExp("<select[^>]+\\bname=[\"]?([^\" >\n\t]*)");
        QRegExp optionValueRegExp("<option[^>]+\\bvalue=([^\"][^ >\n\t]*|\"[^\"]*\")");
        QRegExp optionSelectedRegExp("<option[^>]* selected([> \t\n]|=[\"]?selected)");

        /// initialize result map
        QMap<QString, QString> result;

        /// determined boundaries of (only) "form" tag
        int startPos = htmlText.indexOf(formTagBegin);
        int endPos = htmlText.indexOf(formTagEnd, startPos);

        /// search for "input" tags within form
        int p = htmlText.indexOf(inputTagBegin, startPos);
        while (p > startPos && p < endPos) {
            /// get "type", "name", and "value" attributes
            QString inputType = htmlText.indexOf(inputTypeRegExp, p) == p ? inputTypeRegExp.cap(1) : QString::null;
            QString inputName = htmlText.indexOf(inputNameRegExp, p) == p ? inputNameRegExp.cap(1) : QString::null;
            QString inputValue = htmlText.indexOf(inputValueRegExp, p) ? inputValueRegExp.cap(1) : QString::null;
            /// some values have quotation marks around, remove them
            if (inputValue[0] == '"')
                inputValue = inputValue.mid(1, inputValue.length() - 2);

            if (!inputValue.isNull() && !inputName.isNull()) {
                /// get value of input types
                if (inputType == "hidden" || inputType == "text" || inputType == "submit")
                    result[inputName] = inputValue;
                else if (inputType == "radio" || inputType == "checkbox") {
                    /// must be checked or selected
                    if (htmlText.indexOf(inputIsCheckedRegExp, p) == p) {
                        result[inputName] = inputValue;
                    }
                }
            }
            /// ignore input type "image"

            p = htmlText.indexOf(inputTagBegin, p + 1);
        }

        /// search for "select" tags within form
        p = htmlText.indexOf(selectTagBegin, startPos);
        while (p > startPos && p < endPos) {
            /// get "name" attribute from "select" tag
            QString selectName = htmlText.indexOf(selectNameRegExp, p) == p ? selectNameRegExp.cap(1) : QString::null;

            /// "select" tag contains one or several "option" tags, search all
            int popt = htmlText.indexOf(optionTagBegin, p);
            int endSelect = htmlText.indexOf(selectTagEnd, p);
            while (popt > p && popt < endSelect) {
                /// get "value" attribute from "option" tag
                QString optionValue = htmlText.indexOf(optionValueRegExp, popt) == popt ? optionValueRegExp.cap(1) : QString::null;
                if (!selectName.isNull() && !optionValue.isNull()) {
                    /// if this "option" tag is "selected", store value
                    if (htmlText.indexOf(optionSelectedRegExp, popt) == popt) {
                        result[selectName] = optionValue;
                    }
                }

                popt = htmlText.indexOf(optionTagBegin, popt + 1);
            }

            p = htmlText.indexOf(selectTagBegin, p + 1);
        }

        return result;
    }
};

WebSearchGoogleScholar::WebSearchGoogleScholar(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchGoogleScholar::WebSearchGoogleScholarPrivate(this))
{
    // nothing
}

void WebSearchGoogleScholar::startSearch()
{
    m_hasBeenCanceled = false;
    emit stoppedSearch(resultNoError);
}

void WebSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->numResults = numResults;
    m_hasBeenCanceled = false;

    QStringList queryFragments;
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyFreeText]))
    queryFragments.append(encodeURL(queryFragment));
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyTitle]))
    queryFragments.append(encodeURL(queryFragment));
    d->queryFreetext = queryFragments.join(" ");
    queryFragments.clear();
    foreach(QString queryFragment, splitRespectingQuotationMarks(query[queryKeyAuthor]))
    queryFragments.append(encodeURL(queryFragment));
    d->queryAuthor = queryFragments.join(" ");
    d->queryYear = encodeURL(query[queryKeyYear]);

    KUrl url(d->startPageUrl);
    QNetworkReply *reply = networkAccessManager()->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
}

void WebSearchGoogleScholar::doneFetchingStartPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = d->formParameters(reply->readAll());
        inputMap["hl"] = "en";

        KUrl url(d->configPageUrl.arg(reply->url().host()));
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());

        QNetworkReply *newReply = networkAccessManager()->get(QNetworkRequest(url));
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingConfigPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchGoogleScholar::doneFetchingConfigPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = d->formParameters(reply->readAll());
        inputMap["hl"] = "en";
        inputMap["scis"] = "yes";
        inputMap["scisf"] = "4";
        inputMap["num"] = QString::number(d->numResults);

        KUrl url(d->setConfigPageUrl.arg(reply->url().host()));
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());

        QNetworkReply *newReply = networkAccessManager()->get(QNetworkRequest(url));
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSetConfigPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchGoogleScholar::doneFetchingSetConfigPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QMap<QString, QString> inputMap = d->formParameters(reply->readAll());
        QStringList dummyArguments = QStringList() << "as_epq" << "as_oq" << "as_eq" << "as_occt" << "as_publication" << "as_sdtf";
        foreach(QString dummyArgument, dummyArguments) {
            inputMap[dummyArgument] = "";
        }
        inputMap["hl"] = "en";
        inputMap["num"] = QString::number(d->numResults);

        KUrl url(QString(d->queryPageUrl).arg(reply->url().host()));
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it)
            url.addQueryItem(it.key(), it.value());
        url.addEncodedQueryItem(QString("as_q").toAscii(), d->queryFreetext.toAscii());
        url.addEncodedQueryItem(QString("as_sauthors").toAscii(), d->queryAuthor.toAscii());
        url.addEncodedQueryItem(QString("as_ylo").toAscii(), d->queryYear.toAscii());
        url.addEncodedQueryItem(QString("as_yhi").toAscii(), d->queryYear.toAscii());

        kDebug() << "url =" << url.prettyUrl();

        QNetworkReply *newReply = networkAccessManager()->get(QNetworkRequest(url));
        connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingQueryPage()));
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchGoogleScholar::doneFetchingQueryPage()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString htmlText = reply->readAll();

        QRegExp linkToBib("/scholar.bib\\?[^\" >]+");
        int pos = 0;
        d->listBibTeXurls.clear();
        while ((pos = linkToBib.indexIn(htmlText, pos)) != -1) {
            d->listBibTeXurls << "http://" + reply->url().host() + linkToBib.cap(0).replace("&amp;", "&");
            pos += linkToBib.matchedLength();
        }

        if (!d->listBibTeXurls.isEmpty()) {
            QNetworkReply *newReply = networkAccessManager()->get(QNetworkRequest(d->listBibTeXurls.first()));
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->listBibTeXurls.removeFirst();
        } else
            emit stoppedSearch(resultNoError);
    } else
        kDebug() << "url was" << reply->url().toString();
}

void WebSearchGoogleScholar::doneFetchingBibTeX()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString rawText = reply->readAll();
        File *bibtexFile = importer.fromString(rawText);

        Entry *entry = NULL;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); entry == NULL && it != bibtexFile->constEnd(); ++it) {
                entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    Value v;
                    v.append(new VerbatimText(label()));
                    entry->insert("x-fetchedfrom", v);
                    emit foundEntry(entry);
                }
            }
            delete bibtexFile;
        }

        if (entry == NULL) {
            kWarning() << "Searching" << label() << "resulted in invalid BibTeX data:" << QString(reply->readAll());
            emit stoppedSearch(resultUnspecifiedError);
            return;
        }

        if (!d->listBibTeXurls.isEmpty()) {
            QNetworkReply *newReply = networkAccessManager()->get(QNetworkRequest(d->listBibTeXurls.first()));
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->listBibTeXurls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}

QString WebSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

QString WebSearchGoogleScholar::favIconUrl() const
{
    return QLatin1String("http://scholar.google.com/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchGoogleScholar::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchGoogleScholar::homepage() const
{
    return KUrl("http://scholar.google.com/");
}

void WebSearchGoogleScholar::cancel()
{
    WebSearchAbstract::cancel();
}
