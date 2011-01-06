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

#include <KLocale>
#include <KMessageBox>
#include <KDebug>
#include <KConfigGroup>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <fileimporterbibtex.h>
#include "websearchgooglescholar.h"


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

class WebSearchGoogleScholar::WebSearchGoogleScholarPrivate
{
private:
    WebSearchGoogleScholar *p;
    QMap<QString, QString> originalCookiesSettings;
    bool originalCookiesEnabled;


public:
    int numResults;
    QWidget *w;
    bool hasBeenCancelled;
    KIO::TransferJob *currentJob;
    QStringList listBibTeXurls;
    QString queryString;
    QString startPageUrl;
    QString configPageUrl;
    QString setConfigPageUrl;
    QString queryPageUrl;

    WebSearchGoogleScholarPrivate(QWidget *widget, WebSearchGoogleScholar *parent)
            : p(parent), w(widget) {
        startPageUrl = QLatin1String("http://scholar.google.com/?hl=en");
        configPageUrl = QLatin1String("http://%1/scholar_preferences?");
        setConfigPageUrl = QLatin1String("http://%1/scholar_setprefs?");
        queryPageUrl = QLatin1String("http://%1/scholar?");
    }

    bool cookiesEnabled() {
        QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<bool> reply = kded.call("loadModule", QString("kcookiejar"));

        KConfig cfg("kcookiejarrc");
        KConfigGroup group = cfg.group("Cookie Policy");
        QString answer = group.readEntry("Cookies", "false");
        return answer.toLower() == "true";
    }

    void setCookiesEnabled(bool enabled) {
        QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<bool> reply = kded.call("loadModule", QString("kcookiejar"));

        KConfig cfg("kcookiejarrc");
        KConfigGroup group = cfg.group("Cookie Policy");
        group.writeEntry("Cookies", enabled ? "true" : "false");

        if (enabled) {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
            kcookiejar.call("reloadPolicy");
        } else {
            QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
            kded.call("shutdown");
        }
    }

    void changeCookieSettings(const QString &url) {
        if (originalCookiesSettings.contains(url))
            return;

        bool success = false;
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
        QDBusReply<QString> replyGetDomainAdvice = kcookiejar.call("getDomainAdvice", url);
        if (replyGetDomainAdvice.isValid()) {
            originalCookiesSettings.insert(url, replyGetDomainAdvice.value());
            QDBusReply<bool> replySetDomainAdvice = kcookiejar.call("setDomainAdvice", url, "accept");
            success = replySetDomainAdvice.isValid() && replySetDomainAdvice.value();
            if (success) {
                QDBusReply<void> reply = kcookiejar.call("reloadPolicy");
                success = reply.isValid();
            }
        }

        if (!success) {
            restoreOldCookieSettings();
            p->doStopSearch(resultUnspecifiedError);
            KMessageBox::error(w, i18n("Could not change cookie policy for url \"%1\".\n\nIt is necessary that cookies are accepted from this source.", url));
            return;
        }
    }

    void initializeCookieSettings() {
        originalCookiesEnabled = cookiesEnabled();
        setCookiesEnabled(true);
        originalCookiesSettings.clear();
        changeCookieSettings(startPageUrl);
    }

    void restoreOldCookieSettings() {
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
        for (QMap<QString, QString>::ConstIterator it = originalCookiesSettings.constBegin(); it != originalCookiesSettings.constEnd(); ++it)
            QDBusReply<bool> replySetDomainAdvice = kcookiejar.call("setDomainAdvice", it.key(), it.value());

        setCookiesEnabled(originalCookiesEnabled);
        kcookiejar.call("reloadPolicy");
        originalCookiesSettings.clear();
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
};

WebSearchGoogleScholar::WebSearchGoogleScholar(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchGoogleScholar::WebSearchGoogleScholarPrivate(parent, this))
{
    // nothing
}

void WebSearchGoogleScholar::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->numResults = numResults;
    d->hasBeenCancelled = false;
    d->currentJob = NULL;

    QStringList queryFragments;
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        queryFragments << it.value();
    }
    d->queryString = queryFragments.join(" ");

    d->initializeCookieSettings();

    KIO::StoredTransferJob *job = KIO::storedGet(d->startPageUrl, KIO::Reload);
    job->addMetaData("cookies", "auto");
    job->addMetaData("cache", "reload");
    connect(job, SIGNAL(result(KJob *)), this, SLOT(doneFetchingStartPage(KJob*)));
    connect(job, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
    connect(job, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
    d->currentJob = job;
}

void WebSearchGoogleScholar::doneFetchingStartPage(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        KMessageBox::error(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);
    dumpData(transferJob->data(), 0);
    kDebug() << "Google host: " << transferJob->url().host();

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
}

void WebSearchGoogleScholar::doneFetchingConfigPage(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        d->restoreOldCookieSettings();
        KMessageBox::sorry(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

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
}

void WebSearchGoogleScholar::doneFetchingSetConfigPage(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else  if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        d->restoreOldCookieSettings();
        KMessageBox::sorry(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);
    dumpData(transferJob->data(), 2);

    QMap<QString, QString> inputMap = d->formParameters(transferJob->data());
    inputMap["hl"] = "en";
    inputMap["num"] = QString::number(d->numResults);
    inputMap["q"] = d->queryString;
    QString url = d->queryPageUrl;
    url = url.arg(transferJob->url().host()).append(d->serializeFormParameters(inputMap));
    KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
    newJob->addMetaData("cookies", "auto");
    newJob->addMetaData("cache", "reload");
    connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingQueryPage(KJob*)));
    connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
    connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
    d->currentJob = newJob;
}

void WebSearchGoogleScholar::doneFetchingQueryPage(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        d->restoreOldCookieSettings();
        KMessageBox::sorry(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

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
        kWarning() << "Searching " << label() << " resulted in no hits";
        QStringList domainList;
        domainList << "scholar.google.com";
        QString ccSpecificDomain = transferJob->url().host();
        if (!domainList.contains(ccSpecificDomain))
            domainList << ccSpecificDomain;
        KMessageBox::information(d->w, i18n("<qt><p>No hits were found in Google Scholar.</p><p>One possible reason is that cookies are disabled for the following domains:</p><ul><li>%1</li></ul><p>In Konqueror's cookie settings, these domains can be allowed to set cookies in your system.</p></qt>", domainList.join("</li><li>")), label());
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
    }
}

void WebSearchGoogleScholar::doneFetchingBibTeX(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        d->restoreOldCookieSettings();
        KMessageBox::sorry(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    // FIXME has this section to be protected by a mutex?
    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);
    dumpData(transferJob->data(), 4);

    QByteArray ba(transferJob->data());
    QBuffer buffer(&ba);
    buffer.open(QIODevice::ReadOnly);
    File *bibtexFile = importer.load(&buffer);
    buffer.close();

    Entry *entry = NULL;
    if (bibtexFile != NULL) {
        for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
            entry = dynamic_cast<Entry*>(*it);
            if (entry != NULL) {
                Value v;
                v << new PlainText(i18n("%1 with search term \"%2\"", label(), d->queryString));
                entry->insert(QLatin1String("kbibtex-websearch"), v);
                emit foundEntry(entry);
            }
        }
        delete bibtexFile;
    }

    if (entry == NULL) {
        kWarning() << "Searching " << label() << " resulted in invalid BibTeX data: " << QString(transferJob->data());
        d->restoreOldCookieSettings();
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
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultNoError);
    }
}

void WebSearchGoogleScholar::permanentRedirection(KIO::Job *, const KUrl &, const KUrl &u)
{
    QString url = "http://" + u.host();
    d->changeCookieSettings(url);
}

void WebSearchGoogleScholar::redirection(KIO::Job *, const KUrl &u)
{
    QString url = "http://" + u.host();
    d->changeCookieSettings(url);
}

QString WebSearchGoogleScholar::label() const
{
    return i18n("Google Scholar");
}

void WebSearchGoogleScholar::cancel()
{
    if (d->currentJob != NULL)
        d->currentJob->kill(KJob::EmitResult);
    d->hasBeenCancelled = true;
}

void WebSearchGoogleScholar::doStopSearch(int e)
{
    emit stoppedSearch(e);
}
