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

#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QAbstractListModel>
#include <QListView>
#include <QStringListModel>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QMouseEvent>
#include <QDesktopServices>

#include <kio/jobclasses.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>
#include <KStandardDirs>
#include <KDebug>
#include <KUrl>
#include <KLocale>

#include <value.h>
#include <entry.h>
#include <fileinfo.h>
#include "findpdf.h"

//const int URLRole = Qt::UserRole + 42;
const int CachedFilenameRole = Qt::UserRole + 43;
const char *pdfMagic = "%PDF";

class UrlListModel : public QAbstractListModel
{
private:
    QList<KUrl> urlList;

    QString fileNameFromUrl(const KUrl &url) const {
        QString result = url.pathOrUrl().replace(QRegExp("[^-a-z0-9_]", Qt::CaseInsensitive), "");
        result.prepend(KStandardDirs::locateLocal("cache", "pdf/"));
        return result;
    }

public:
    int rowCount(const QModelIndex &parent) const {
        if (parent.isValid())
            return 0;
        return urlList.count();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        if (index.row() < 0 || index.row() >= urlList.count())
            return QVariant();

        switch (role) {
        case Qt::DecorationRole:
            return KIcon("application-pdf");
        case Qt::DisplayRole:
            return QVariant(urlList[index.row()].pathOrUrl());
            /*      case URLRole:
                      return QVariant::fromValue<KUrl>(urlList[index.row()]);*/
        case CachedFilenameRole:
            return fileNameFromUrl(urlList[index.row()]);
        default:
            return QVariant();
        }
    }

    void clear() {
        urlList.clear();
        reset();
    }

    void append(const KUrl &url, const QByteArray &data = QByteArray()) {
        if (!urlList.contains(url)) {
            if (!data.isEmpty()) {
                QFile file(fileNameFromUrl(url));
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(data);
                    file.close();
                }

            }
            urlList << url;
            reset();
        }
    }

    bool contains(const KUrl &url) const {
        return urlList.contains(url);
    }

    bool isCached(const KUrl &url) const {
        return QFileInfo(fileNameFromUrl(url)).exists();
    }
};

class UrlListView : public QListView
{
public:
    UrlListView(QWidget *parent)
            : QListView(parent) {
        // nothing
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent * event) {
        QModelIndex index = indexAt(event->pos());
        QDesktopServices::openUrl(index.data(CachedFilenameRole).toString()); // TODO KDE way?
    }
};

class FindPDF::FindPDFPrivate
{
private:
    FindPDF *parent;
    UrlListModel *model;
    UrlListView *listView;
    QWidget *container;
    bool stoppedByForce;

public:
    enum ExternalPDFProvider {Google, CiteSeerX};

    int numAsynchronousJobs;
    QWidget *widget;
    KDialog *dlg;

    FindPDFPrivate(QWidget *w, FindPDF *p)
            : parent(p), widget(w), dlg(NULL) {
        numAsynchronousJobs = 0;
    }

    QString selectedFilename() {
        QModelIndex index = listView->selectionModel()->selectedIndexes().first();
        return index.data(CachedFilenameRole).toString();
    }

    QWidget *createGUI(KDialog *parent) {
        container = new QWidget(parent);
        QBoxLayout *layout = new QVBoxLayout(container);

        QLabel *label = new QLabel(i18n("<qt><p>The following files have been identified as potential PDF documents representing this entry.</p><p>Double-click on a list item below to view the PDF file in an external viewer.</p></qt>"), container);
        label->setWordWrap(true);
        layout->addWidget(label);
        layout->addSpacing(4);

        model = new UrlListModel();

        listView = new UrlListView(container);
        layout->addWidget(listView);
        listView->setModel(model);
        listView->setAlternatingRowColors(true);

        return container;
    }

    void findPDFinFetchedWebpage(const KUrl &url) {
        ++numAsynchronousJobs;
        kDebug() << "url = " << url.pathOrUrl();
        KIO::StoredTransferJob *job = KIO::storedGet(url);
        job->ui()->setWindow(widget);
        connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
    }

    void findPDFbyUrlRewriting(const KUrl &url) {
        QRegExp springerLink("^ht.*springerlink.com/(content|index)/([a-z0-9]+)(/|\\.pdf)$", Qt::CaseInsensitive);
        QRegExp ieeeExplorer("^ht.*ieeexplore.ieee.org/.*[?&]arnumber=([0-9]+)([?&]|$)", Qt::CaseInsensitive);
        QRegExp arXiv("^ht.*arxiv.org/[a-z]+/([.0-9a-z]+)(v([0-9]+))?$", Qt::CaseInsensitive);
        const QString urlText = url.pathOrUrl();

        if (urlText.indexOf(springerLink) >= 0) {
            /// handle special case of SpringerLink
            KUrl url("http://www.springerlink.com/content/" + springerLink.cap(2) + "/fulltext.pdf");
            if (!model->isCached(url)) {
                ++numAsynchronousJobs;
                kDebug() << "url = " << url.pathOrUrl();
                KIO::StoredTransferJob *job = KIO::storedGet(url);
                job->ui()->setWindow(widget);
                connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
            } else
                model->append(url);
        } else if (urlText.indexOf(ieeeExplorer) >= 0) {
            /// handle special case of IEEE Xplore
            KUrl url(QString("http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=%1").arg(ieeeExplorer.cap(1)));
            if (!model->isCached(url)) {
                ++numAsynchronousJobs;
                kDebug() << "url = " << url.pathOrUrl();
                KIO::StoredTransferJob *job = KIO::storedGet(url);
                job->ui()->setWindow(widget);
                connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
            } else
                model->append(url);
        } else if (urlText.indexOf(arXiv) >= 0) {
            /// handle special case of arXiv
            bool ok = false;
            int version = qMax(1, arXiv.cap(3).toInt(&ok));
            if (!ok) version = 1;
            while (version < 16 && (ok = KIO::NetAccess::exists(KUrl(QString("http://arxiv.org/abs/%1v%2").arg(arXiv.cap(1)).arg(version)), KIO::NetAccess::DestinationSide, widget))) ++version;
            KUrl url(QString("http://arxiv.org/pdf/%1v%2").arg(arXiv.cap(1)).arg(version - 1));
            if (version > 1) {
                if (!model->isCached(url)) {
                    ++numAsynchronousJobs;
                    kDebug() << "url = " << url.pathOrUrl();
                    KIO::StoredTransferJob *job = KIO::storedGet(url);
                    job->ui()->setWindow(widget);
                    connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
                } else model->append(url);
            }
        }
    }

    void findPDFexternally(ExternalPDFProvider externalPDFProvider, const Entry &entry) {
        switch (externalPDFProvider) {
        case Google: {
            QString query;
            if (entry.contains(Entry::ftTitle)) {
                Value v = entry[Entry::ftTitle];
                query = PlainTextValue::text(v);
            }
            if (entry.contains(Entry::ftAuthor)) {
                Value v = entry[Entry::ftAuthor];
                for (Value::ConstIterator it = v.constBegin(); it != v.constEnd(); ++it) {
                    Person *pers = dynamic_cast<Person*>(*it);
                    if (pers != NULL) {
                        query += " " + pers->lastName();
                    }
                }
            } else    if (entry.contains(Entry::ftEditor)) {
                Value v = entry[Entry::ftEditor];
                for (Value::ConstIterator it = v.constBegin(); it != v.constEnd(); ++it) {
                    Person *pers = dynamic_cast<Person*>(*it);
                    if (pers != NULL) {
                        query += " " + pers->lastName();
                    }
                }
            }

            query = query.replace("-", "+").replace(" ", "+").replace("%", "%25").replace("\"", "%22").replace("&", "%26").replace(":", "%3A").replace("?", "%3F");

            ++numAsynchronousJobs;
            KUrl url(QString("http://www.google.com/search?num=5&q=filetype%3Apdf+%1&ie=UTF-8&oe=UTF-8").arg(query));
            kDebug() << "google url " << url.pathOrUrl();
            KIO::StoredTransferJob *job = KIO::storedGet(url);
            job->ui()->setWindow(widget);
            connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
        }
        break;
        case CiteSeerX: {
            if (entry.contains(Entry::ftTitle)) {
                Value v = entry[Entry::ftTitle];
                QString query = PlainTextValue::text(v);

                query = query.replace("-", "+").replace(" ", "+").replace("%", "%25").replace("\"", "%22").replace("&", "%26").replace(":", "%3A").replace("?", "%3F");

                ++numAsynchronousJobs;
                KUrl url(QString("http://citeseerx.ist.psu.edu/search?q=%1&submit=Search&sort=rel").arg(query));
                kDebug() << "CiteSeerX url " << url.pathOrUrl();
                KIO::StoredTransferJob *job = KIO::storedGet(url);
                job->ui()->setWindow(widget);
                connect(job, SIGNAL(result(KJob*)), parent, SLOT(citeseerXfinished(KJob*)));
            }
        }
        break;
        default:
            kWarning() << "unknown external PDF provider: " << externalPDFProvider;
        }
    }

    void downloadFinished(KIO::StoredTransferJob *job) {
        if (stoppedByForce) return; ///< ignore if was forced to stop

        --numAsynchronousJobs;

        if (job->error() == 0) {
            KUrl url = job->url();
            if (job->data().startsWith(pdfMagic)) {
                model->append(url, job->data());
            } else {
                findPDFbyUrlRewriting(url);

                QRegExp anchor("<a [^>]*href=\"([^\"]+)\"[^>]*>(.+)</a>", Qt::CaseInsensitive);
                anchor.setMinimal(true);
                QRegExp iframe("<iframe [^>]*src=\"([^\"]+)\"[^>]*>", Qt::CaseInsensitive);
                QRegExp meta("(<meta [^>]*content=\"([^\"]+)\"[^>]*name=\"([^\"]+)\"[^>]*>|<meta [^>]*name=\"([^\"]+)\"[^>]*content=\"([^\"]+)\"[^>]*>)", Qt::CaseInsensitive);
                QRegExp pdfUrl("^.*pdf(\\?.+)?$");
                QRegExp fullText("(\\[pdf\\]|full text|download pdf|get pdf|^pdf$)", Qt::CaseInsensitive);

                QTextStream ts(job->data());
                QString allText = ts.readAll();

                int p = -1;
                while ((p = allText.indexOf(anchor, p + 1)) >= 0) {
                    QString href = anchor.cap(1);
                    QString text = anchor.cap(2);
                    if (!href.isEmpty() && (href.indexOf(pdfUrl, 0) == 0 || text.indexOf(fullText) >= 0)) {
                        KUrl pdfUrl;
                        if (href[0] == '/') {
                            /// absolute path, starting with `/' (not full URL)
                            pdfUrl = url;
                            pdfUrl.setPath(href);
                        } else if (href.indexOf("://") > 0) {
                            /// full URL
                            pdfUrl = KUrl(href);
                        } else {
                            /// relative URL
                            pdfUrl = url;
                            pdfUrl.setFileName(href); // FIXME: Does this work?
                        }

                        if (!model->isCached(pdfUrl)) {
                            ++numAsynchronousJobs;
                            kDebug() << "pdfUrl = " << pdfUrl.pathOrUrl();
                            KIO::StoredTransferJob *job = KIO::storedGet(pdfUrl);
                            job->ui()->setWindow(widget);
                            connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
                        } else
                            model->append(pdfUrl);
                    }
                }

                p = -1;
                while ((p = allText.indexOf(iframe, p + 1)) >= 0) {
                    QString src = iframe.cap(1);
                    if (!src.isEmpty() && src.indexOf(pdfUrl, 0) == 0) {
                        KUrl pdfUrl;
                        if (src[0] == '/') {
                            /// absolute path, starting with `/' (not full URL)
                            pdfUrl = url;
                            pdfUrl.setPath(src);
                        } else if (src.indexOf("://") >= 0) {
                            /// full URL
                            pdfUrl = KUrl(src);
                        } else {
                            /// relative URL
                            pdfUrl = url;
                            pdfUrl.setFileName(src); // FIXME: Does this work?
                        }

                        if (!model->isCached(pdfUrl)) {
                            ++numAsynchronousJobs;
                            kDebug() << "pdfUrl = " << pdfUrl.pathOrUrl();
                            KIO::StoredTransferJob *job = KIO::storedGet(pdfUrl);
                            job->ui()->setWindow(widget);
                            connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
                        } else
                            model->append(pdfUrl);
                    }
                }

                p = -1;
                while ((p = allText.indexOf(meta, p + 1)) >= 0) {
                    QString name = meta.cap(3) + meta.cap(4);
                    QString content = meta.cap(2) + meta.cap(5);
                    if (!content.isEmpty() && name.compare("citation_pdf_url", Qt::CaseInsensitive) == 0) {
                        KUrl pdfUrl;
                        if (content[0] == '/') {
                            /// absolute path, starting with `/' (not full URL)
                            pdfUrl = url;
                            pdfUrl.setPath(content);
                        } else if (content.indexOf("://") >= 0) {
                            /// full URL
                            pdfUrl = KUrl(content);
                        } else {
                            /// relative URL
                            pdfUrl = url;
                            pdfUrl.setFileName(content); // FIXME: Does this work?
                        }

                        if (!model->isCached(pdfUrl)) {
                            ++numAsynchronousJobs;
                            kDebug() << "pdfUrl = " << pdfUrl.pathOrUrl();
                            KIO::StoredTransferJob *job = KIO::storedGet(pdfUrl);
                            job->ui()->setWindow(widget);
                            connect(job, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
                        } else
                            model->append(pdfUrl);
                    }
                }
            }
        } else {
            kWarning() << "Receiving data failed";
        }

        if (numAsynchronousJobs == 0)
            stopSearch();
    }

    void downloadCiteSeerXFinished(KIO::StoredTransferJob *job) {
        if (stoppedByForce) return; ///< ignore if was forced to stop

        --numAsynchronousJobs;
        if (job->error() == 0) {
            QRegExp anchor("<a [^>]*href=\"([^\"]+/summary\\?[^\"]+)\"[^>]*>", Qt::CaseInsensitive);
            anchor.setMinimal(true);

            QTextStream ts(job->data());
            QString allText = ts.readAll();

            if (allText.indexOf(anchor) >= 0) {
                ++numAsynchronousJobs;
                QString href = anchor.cap(1);
                kDebug() << "href = " << href;
                KIO::StoredTransferJob *newJob = KIO::storedGet(KUrl(href));
                newJob->ui()->setWindow(job->ui()->window());
                connect(newJob, SIGNAL(result(KJob*)), parent, SLOT(finished(KJob*)));
            }
        } else {
            kWarning() << "Receiving data failed";
        }

        if (numAsynchronousJobs == 0)
            stopSearch();
    }

    void probeKnownUrls(QList<KUrl> &urls) {
        for (QList<KUrl>::Iterator it = urls.begin(); it != urls.end(); ++it) {
            findPDFbyUrlRewriting(*it);
            findPDFinFetchedWebpage(*it);
        }
    }

    void probeExternalSources(const Entry &entry) {
        findPDFexternally(Google, entry);
        findPDFexternally(CiteSeerX, entry);
    }

    void startSearch(const Entry &entry, const File *file) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QList<KUrl> urls = FileInfo::entryUrls(&entry, file->url());
        probeKnownUrls(urls);
        probeExternalSources(entry);
    }

    void stopSearch() {
        container->setEnabled(true);
        dlg->enableButton(KDialog::User1, false);
        dlg->enableButton(KDialog::Ok, true);
        dlg->enableButton(KDialog::Cancel, true);
        QApplication::restoreOverrideCursor();
    }

    void forcedStop() {
        stoppedByForce = true;
        numAsynchronousJobs = 0;
        stopSearch();
    }
};

FindPDF::FindPDF(QWidget *parent)
        : QObject(parent), d(new FindPDFPrivate(parent, this))
{
    // nothing
}


QString FindPDF::findPDF(const Entry &entry, const File *file)
{
    if (d->dlg != NULL) return QString::null;

    d->dlg = new KDialog(d->widget);
    d->dlg->setButtons(KDialog::User1 | KDialog::Ok | KDialog::Cancel);
    d->dlg->setButtonText(KDialog::Ok, i18n("Download"));
    d->dlg->setButtonIcon(KDialog::Ok, KIcon("download"));
    d->dlg->setButtonText(KDialog::User1, i18n("Stop"));
    d->dlg->setButtonIcon(KDialog::User1, KIcon("media-playback-stop"));
    connect(d->dlg, SIGNAL(user1Clicked()), this, SLOT(forcedStop()));
    d->dlg->setCaption(i18n("Find PDF File"));
    d->dlg->setMainWidget(d->createGUI(d->dlg));

    d->startSearch(entry, file);

    d->dlg->mainWidget()->setEnabled(false);
    d->dlg->enableButton(KDialog::Ok, false);
    d->dlg->enableButton(KDialog::Cancel, false);

    QString result = d->dlg->exec() == KDialog::Accepted ? d->selectedFilename() : QString::null;

    d->dlg->deleteLater();
    d->dlg = NULL;
    return result;
}

void FindPDF::finished(KJob *j)
{
    KIO::StoredTransferJob *job = static_cast<  KIO::StoredTransferJob*>(j);
    d->downloadFinished(job);
}

void FindPDF::citeseerXfinished(KJob *j)
{
    KIO::StoredTransferJob *job = static_cast<  KIO::StoredTransferJob*>(j);
    d->downloadCiteSeerXFinished(job);
}

void FindPDF::forcedStop()
{
    d->forcedStop();
}
