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

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QSpinBox>
#include <QLayout>
#include <QLabel>

#include <KLocale>
#include <KMessageBox>
#include <KDebug>
#include <KConfigGroup>
#include <KLineEdit>
#include <KIcon>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/jobclasses.h>

#include <fileimporterbibtex.h>
#include "websearchgooglescholar.h"
#include "cookiemanager.h"

FileImporterBibTeX importer;

static void dumpData(const QByteArray &byteArray, int index)
{
    Q_UNUSED(byteArray);
    Q_UNUSED(index);
    /*
    for (int i = index; i < 10; ++i) {
        QFile hf(QString("/tmp/dump%1.html").arg(i));
        hf.remove();
    }

    QFile f(QString("/tmp/dump%1.html").arg(index));
    f.open(QIODevice::WriteOnly);
    f.write(byteArray);
    f.close();
    */
}
/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class WebSearchGoogleScholar::WebSearchQueryFormGoogleScholar : public WebSearchQueryFormAbstract
{
public:
    KLineEdit *lineEditAllWords, *lineEditAnyWord, *lineEditWithoutWords, *lineEditExactPhrase, *lineEditAuthor, *lineEditPublication, *lineEditYearStart, *lineEditYearEnd;
    QSpinBox *numResultsField;

    WebSearchQueryFormGoogleScholar(QWidget *parent)
            : WebSearchQueryFormAbstract(parent) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("<qt><b>All</b> words:</qt>"), this);
        layout->addWidget(label, 0, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditAllWords = new KLineEdit(this);
        layout->addWidget(lineEditAllWords, 0, 1, 1, 3);
        label->setBuddy(lineEditAllWords);
        connect(lineEditAllWords, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("<qt><b>Any</b> words:</qt>"), this);
        layout->addWidget(label, 1, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditAnyWord = new KLineEdit(this);
        layout->addWidget(lineEditAnyWord, 1, 1, 1, 3);
        label->setBuddy(lineEditAnyWord);
        connect(lineEditAnyWord, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("<qt><b>Without</b> words:</qt>"), this);
        layout->addWidget(label, 2, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditWithoutWords = new KLineEdit(this);
        layout->addWidget(lineEditWithoutWords, 2, 1, 1, 3);
        label->setBuddy(lineEditWithoutWords);
        connect(lineEditWithoutWords, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("Exact phrase:"), this);
        layout->addWidget(label, 3, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditExactPhrase = new KLineEdit(this);
        layout->addWidget(lineEditExactPhrase, 3, 1, 1, 3);
        label->setBuddy(lineEditExactPhrase);
        connect(lineEditExactPhrase, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("Author:"), this);
        layout->addWidget(label, 4, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditAuthor = new KLineEdit(this);
        layout->addWidget(lineEditAuthor, 4, 1, 1, 3);
        label->setBuddy(lineEditAuthor);
        connect(lineEditAuthor, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("Publication:"), this);
        layout->addWidget(label, 5, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditPublication = new KLineEdit(this);
        layout->addWidget(lineEditPublication, 5, 1, 1, 3);
        label->setBuddy(lineEditPublication);
        connect(lineEditPublication, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("Year:"), this);
        layout->addWidget(label, 6, 0, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
        lineEditYearStart = new KLineEdit(this);
        layout->addWidget(lineEditYearStart, 6, 1, 1, 1);
        label->setBuddy(lineEditYearStart);
        connect(lineEditYearStart, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));
        label = new QLabel(QChar(0x2013), this);
        layout->addWidget(label, 6, 2, 1, 1);
        lineEditYearEnd = new KLineEdit(this);
        layout->addWidget(lineEditYearEnd, 6, 3, 1, 1);
        connect(lineEditYearEnd, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        label = new QLabel(i18n("Number of Results:"), this);
        layout->addWidget(label, 7, 0, 1, 1);
        numResultsField = new QSpinBox(this);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);
        layout->addWidget(numResultsField, 7, 1, 1, 3);
        label->setBuddy(numResultsField);

        layout->setRowStretch(8, 100);
        lineEditAllWords->setFocus(Qt::TabFocusReason);
    }

    virtual bool readyToStart() const {
        return !(lineEditAllWords->text().isEmpty() && lineEditAnyWord->text().isEmpty() && lineEditAuthor->text().isEmpty() && lineEditExactPhrase->text().isEmpty() && lineEditPublication->text().isEmpty() && lineEditWithoutWords->text().isEmpty() && lineEditYearEnd->text().isEmpty() && lineEditYearStart->text().isEmpty());
    }
};

class WebSearchGoogleScholar::WebSearchGoogleScholarPrivate
{
private:
    WebSearchGoogleScholar *p;
    //  QMap<QString, QString> originalCookiesSettings;
    //  bool originalCookiesEnabled;

public:
    int numResults;
    KIO::TransferJob *currentJob;
    QStringList listBibTeXurls;
    QString queryString;
    QString startPageUrl;
    QString advancedSearchPageUrl;
    QString configPageUrl;
    QString setConfigPageUrl;
    QString queryPageUrl;
    bool goAdvanced;
    WebSearchQueryFormGoogleScholar *form;
    CookieManager *cookieManager;

    WebSearchGoogleScholarPrivate(WebSearchGoogleScholar *parent)
            : p(parent), goAdvanced(false), form(NULL), cookieManager(CookieManager::singleton()) {
        startPageUrl = QLatin1String("http://scholar.google.com/?hl=en");
        advancedSearchPageUrl = QLatin1String("http://%1/advanced_scholar_search?hl=en");
        configPageUrl = QLatin1String("http://%1/scholar_preferences?");
        setConfigPageUrl = QLatin1String("http://%1/scholar_setprefs?");
        queryPageUrl = QLatin1String("http://%1/scholar?");
    }

    QMap<QString, QString> formParameters(const QByteArray &byteArray) {
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

        /// initialize data
        QTextStream ts(byteArray);
        QString htmlText = ts.readAll();
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

    QString serializeFormParameters(QMap<QString, QString> &inputMap) {
        QString result;
        for (QMap<QString, QString>::ConstIterator it = inputMap.constBegin(); it != inputMap.constEnd(); ++it) {
            if (!result.isEmpty())
                result.append("&");
            result.append(it.key()).append("=");

            QString value = it.value();
            /// replace some protected characters that would be invalid or ambiguous in an URL
            value = value.replace(" ", "+").replace("%", "%25").replace("&", "%26").replace("=", "%3D").replace("?", "%3F");

            result.append(value);
        }
        return result;
    }

    void startSearch() {
        currentJob = NULL;

        cookieManager->disableCookieRestrictions();

        KIO::StoredTransferJob *job = KIO::storedGet(startPageUrl, KIO::Reload);
        job->addMetaData("cookies", "auto");
        job->addMetaData("cache", "reload");
        connect(job, SIGNAL(result(KJob *)), p, SLOT(doneFetchingStartPage(KJob*)));
        connect(job, SIGNAL(redirection(KIO::Job*, KUrl)), p, SLOT(redirection(KIO::Job*, KUrl)));
        connect(job, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), p, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        currentJob = job;
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
    d->goAdvanced = true;
    d->numResults = d->form->numResultsField->value();
    d->queryString = QString::null;
    d->startSearch();
}

void WebSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->goAdvanced = false;
    d->numResults = numResults;
    m_hasBeenCanceled = false;

    // FIXME: Is there a need for percent encoding?
    d->queryString = query[queryKeyFreeText] + " " + query[queryKeyTitle];
    if (!query[queryKeyAuthor].isEmpty())
        d->queryString.append(" author:\"" + query[queryKeyAuthor] + "\"");
    d->queryString.append(" " + query[queryKeyYear]);

    d->startSearch();
}

void WebSearchGoogleScholar::doneFetchingStartPage(KJob *kJob)
{
    Q_ASSERT(kJob == d->currentJob);
    d->currentJob = NULL;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);
        dumpData(transferJob->data(), 0);
        kDebug() << "Google host:" << transferJob->url().host();

        QMap<QString, QString> inputMap = d->formParameters(transferJob->data());
        inputMap["hl"] = "en";

        QString url = d->configPageUrl;
        url = url.arg(transferJob->url().host()).append(d->serializeFormParameters(inputMap));
        KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingConfigPage(KJob*)));
        connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
        connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        d->currentJob = newJob;
    } else
        d->cookieManager->enableCookieRestrictions();
}

void WebSearchGoogleScholar::doneFetchingConfigPage(KJob *kJob)
{
    Q_ASSERT(kJob == d->currentJob);
    d->currentJob = NULL;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

        dumpData(transferJob->data(), 1);

        QMap<QString, QString> inputMap = d->formParameters(transferJob->data());
        inputMap["hl"] = "en";
        inputMap["scis"] = "yes";
        inputMap["scisf"] = "4";
        inputMap["num"] = QString::number(d->numResults);
        QString url = d->setConfigPageUrl;
        url = url.arg(transferJob->url().host()).append(d->serializeFormParameters(inputMap));
        KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingSetConfigPage(KJob*)));
        connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
        connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        d->currentJob = newJob;
    } else
        d->cookieManager->enableCookieRestrictions();
}

void WebSearchGoogleScholar::doneFetchingAdvSearchPage(KJob *kJob)
{
    Q_ASSERT(kJob == d->currentJob);
    d->currentJob = NULL;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

        dumpData(transferJob->data(), 1);

        QMap<QString, QString> inputMap = d->formParameters(transferJob->data());
        inputMap["hl"] = "en";
        inputMap["num"] = QString::number(d->numResults);
        inputMap["as_q"] = d->form->lineEditAllWords->text();
        inputMap["as_epq"] = d->form->lineEditExactPhrase->text();
        inputMap["as_oq"] = d->form->lineEditAnyWord->text();
        inputMap["as_eq"] = d->form->lineEditWithoutWords->text();
        inputMap["as_sauthors"] = d->form->lineEditAuthor->text();
        inputMap["as_publication"] = d->form->lineEditPublication->text();
        inputMap["as_ylo"] = d->form->lineEditYearStart->text();
        inputMap["as_yhi"] = d->form->lineEditYearEnd->text();
        QString url = d->queryPageUrl;
        url = url.arg(transferJob->url().host()).append(d->serializeFormParameters(inputMap));
        KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingQueryPage(KJob*)));
        connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
        connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        d->currentJob = newJob;
    } else
        d->cookieManager->enableCookieRestrictions();
}


void WebSearchGoogleScholar::doneFetchingSetConfigPage(KJob *kJob)
{
    Q_ASSERT(kJob == d->currentJob);
    d->currentJob = NULL;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

        dumpData(transferJob->data(), 2);

        QMap<QString, QString> inputMap = d->formParameters(transferJob->data());
        inputMap["hl"] = "en";

        if (d->goAdvanced) {
            // TODO
        } else {
            inputMap["num"] = QString::number(d->numResults);
            inputMap["q"] = d->queryString;
        }
        QString url = d->goAdvanced ? d->advancedSearchPageUrl : d->queryPageUrl;
        url = url.arg(transferJob->url().host()).append(d->serializeFormParameters(inputMap));
        KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        if (d->goAdvanced)
            connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingAdvSearchPage(KJob*)));
        else
            connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingQueryPage(KJob*)));
        connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
        connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        d->currentJob = newJob;
    } else
        d->cookieManager->enableCookieRestrictions();
}

void WebSearchGoogleScholar::doneFetchingQueryPage(KJob *kJob)
{
    Q_ASSERT(kJob == d->currentJob);
    d->currentJob = NULL;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

        dumpData(transferJob->data(), 3);

        QRegExp linkToBib("/scholar.bib\\?[^\" >]+");
        QString htmlText(transferJob->data());
        int pos = 0;
        d->listBibTeXurls.clear();
        while ((pos = linkToBib.indexIn(htmlText, pos)) != -1) {
            d->listBibTeXurls << "http://" + transferJob->url().host() + linkToBib.cap(0).replace("&amp;", "&");
            pos += linkToBib.matchedLength();
        }

        if (!d->listBibTeXurls.isEmpty()) {
            KIO::StoredTransferJob * newJob = KIO::storedGet(d->listBibTeXurls.first(), KIO::Reload);
            d->listBibTeXurls.removeFirst();
            newJob->addMetaData("cookies", "auto");
            newJob->addMetaData("cache", "reload");
            connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob*)));
            connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
            connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
            d->currentJob = newJob;
        } else {
            kDebug() << "Searching" << label() << "resulted in no hits";
            QStringList domainList;
            domainList << "scholar.google.com";
            QString ccSpecificDomain = transferJob->url().host();
            if (!domainList.contains(ccSpecificDomain))
                domainList << ccSpecificDomain;
            KMessageBox::information(m_parent, i18n("<qt><p>No hits were found in Google Scholar.</p><p>One possible reason is that cookies are disabled for the following domains:</p><ul><li>%1</li></ul><p>In Konqueror's cookie settings, these domains can be allowed to set cookies in your system.</p></qt>", domainList.join("</li><li>")), label());
            d->cookieManager->enableCookieRestrictions();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        d->cookieManager->enableCookieRestrictions();
}

void WebSearchGoogleScholar::doneFetchingBibTeX(KJob *kJob)
{
    Q_ASSERT(kJob == d->currentJob);
    d->currentJob = NULL;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

        // FIXME has this section to be protected by a mutex?
        dumpData(transferJob->data(), 4);

        QByteArray ba(transferJob->data());
        QBuffer buffer(&ba);
        buffer.open(QIODevice::ReadOnly);
        File *bibtexFile = importer.load(&buffer);
        buffer.close();

        Entry *entry = NULL;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); entry == NULL && it != bibtexFile->constEnd(); ++it) {
                entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL)
                    emit foundEntry(entry);
            }
            delete bibtexFile;
        }

        if (entry == NULL) {
            kWarning() << "Searching" << label() << "resulted in invalid BibTeX data:" << QString(transferJob->data());
            d->cookieManager->enableCookieRestrictions();
            emit stoppedSearch(resultUnspecifiedError);
            return;
        }

        if (!d->listBibTeXurls.isEmpty()) {
            KIO::StoredTransferJob * newJob = KIO::storedGet(d->listBibTeXurls.first(), KIO::Reload);
            d->listBibTeXurls.removeFirst();
            newJob->addMetaData("cookies", "auto");
            newJob->addMetaData("cache", "reload");
            connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob*)));
            connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
            connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
            d->currentJob = newJob;
        } else {
            d->cookieManager->enableCookieRestrictions();
            emit stoppedSearch(resultNoError);
        }
    } else
        d->cookieManager->enableCookieRestrictions();
}

void WebSearchGoogleScholar::permanentRedirection(KIO::Job *, const KUrl &, const KUrl &u)
{
    QString url = "http://" + u.host();
    d->cookieManager->whitelistUrl(url);
}

void WebSearchGoogleScholar::redirection(KIO::Job *, const KUrl &u)
{
    QString url = "http://" + u.host();
    d->cookieManager->whitelistUrl(url);
}

QString WebSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

QString WebSearchGoogleScholar::favIconUrl() const
{
    return QLatin1String("http://scholar.google.com/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchGoogleScholar::customWidget(QWidget *parent)
{
    return (d->form = new WebSearchQueryFormGoogleScholar(parent));
}

KUrl WebSearchGoogleScholar::homepage() const
{
    return KUrl("http://scholar.google.com/");
}

void WebSearchGoogleScholar::cancel()
{
    WebSearchAbstract::cancel();
    if (d->currentJob != NULL)
        d->currentJob->kill(KJob::EmitResult);
}
