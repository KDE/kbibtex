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

#include <QList>
#include <QLayout>
#include <QLabel>
#include <QMap>
#include <QFileInfo>

#include <KLocale>
#include <KComboBox>
#include <KIcon>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KService>
#include <kparts/part.h>
#include <KDebug>
#include <kio/netaccess.h>
#include <KPushButton>
#include <KRun>
#include <KDebug>
#include <kio/jobclasses.h>
#include <kio/job.h>

#include <element.h>
#include <entry.h>
#include <file.h>
#include <fileinfo.h>
#include "urlpreview.h"

class UrlPreview::UrlPreviewPrivate
{
private:
    UrlPreview *p;
    QList<KUrl> urlList;
    KComboBox *urlComboBox;
    KPushButton *externalViewerButton;
    QLabel *message;
    QWidget *partWidget, *currentWidget;
    QMap<QString, KUrl> cbxEntryToUrl;
    QMap<KUrl, QWidget*> urlToWidget;
    QGridLayout *layout;
    KIO::StatJob *runningStatJob;

public:
    const Entry* entry;
    KUrl baseUrl;

    UrlPreviewPrivate(UrlPreview *parent)
            : p(parent), partWidget(NULL), currentWidget(NULL), runningStatJob(NULL) {
        setupGUI();
    }

    void setupGUI() {
        layout = new QGridLayout(p);
        layout->setMargin(0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setRowStretch(0, 0);
        layout->setRowStretch(1, 1);

        urlComboBox = new KComboBox(false, p);
        layout->addWidget(urlComboBox, 0, 0, 1, 1);

        externalViewerButton = new KPushButton(KIcon("document-open"), i18n("Open..."), p);
        layout->addWidget(externalViewerButton, 0, 1, 1, 1);

        connect(externalViewerButton, SIGNAL(clicked()), p, SLOT(openExternally()));
        connect(urlComboBox, SIGNAL(activated(const QString &)), p, SLOT(urlSelected(const QString &)));
    }

    void update() {
        p->unsetCursor();
        urlComboBox->clear();
        cbxEntryToUrl.clear();
        urlToWidget.clear();

        if (runningStatJob != NULL) {
            runningStatJob->kill(KJob::Quietly);
            runningStatJob = NULL;
        }

        urlList = FileInfo::entryUrls(entry, baseUrl);
        for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
            QString fn = (*it).fileName();
            QString full = (*it).prettyUrl();
            QString text = fn.isEmpty() ? full : QString("%1 [%2]").arg(fn).arg(full.left(full.size() - fn.size()));
            QPair<QString, KIcon> mimeTypeIcon = mimeType(*it);
            urlComboBox->addItem(mimeTypeIcon.second, text);
            cbxEntryToUrl.insert(text, *it);
        }
        if (urlComboBox->count() > 0) {
            urlComboBox->setCurrentIndex(0);
            urlSelected(urlComboBox->currentText());
        }
        urlComboBox->setEnabled(urlComboBox->count() > 0);
        externalViewerButton->setEnabled(urlComboBox->count() > 0);
    }

    void urlSelected(const QString &text) {
        KUrl url = cbxEntryToUrl[text];
        QWidget *widget = urlToWidget[url];
        bool stillWaiting = false;

        p->setCursor(Qt::WaitCursor);

        if (widget == NULL) {
            KParts::ReadOnlyPart* part = NULL;

            if (url.isLocalFile()) {
                if (QFileInfo(url.path()).exists()) {
                    QPair<QString, KIcon> mimeTypeIcon = mimeType(url);
                    KService::Ptr serivcePtr = KMimeTypeTrader::self()->preferredService(mimeTypeIcon.first, "KParts/ReadOnlyPart");
                    if (!serivcePtr.isNull())
                        part = serivcePtr->createInstance<KParts::ReadOnlyPart>((QWidget*)p, (QObject*)p);

                    if (part != NULL) {
                        urlToWidget.insert(url, part->widget());
                        part->openUrl(url);
                    } else {
                        QLabel *label = new QLabel(i18n("Cannot create preview for\n%1", url.prettyUrl()), p);
                        label->setAlignment(Qt::AlignCenter);
                        urlToWidget.insert(url, label);
                    }
                } else {
                    QLabel *label = new QLabel(i18n("File does not exist:\n%1", url.prettyUrl()), p);
                    label->setAlignment(Qt::AlignCenter);
                    urlToWidget.insert(url, label);
                }
            } else {
                QLabel *label = new QLabel(i18n("Retrieving file information ..."), p);
                label->setAlignment(Qt::AlignCenter);
                urlToWidget.insert(url, label);

                if (runningStatJob != NULL)
                    runningStatJob->kill(KJob::Quietly);
                runningStatJob = KIO::stat(url, true, 0); // FIXME: deprecated?
                connect(runningStatJob, SIGNAL(finished(KJob*)), p, SLOT(statJobFinished(KJob*)));
                stillWaiting = true;
            }
        }

        switchWidget(urlToWidget[url]);

        if (!stillWaiting)
            p->unsetCursor();
    }

    void switchWidget(QWidget *newWidget) {
        if (currentWidget != NULL)
            delete currentWidget;

        newWidget->show();
        layout->addWidget(newWidget, 1, 0, 1, 2);
        currentWidget = newWidget;
    }

    void openExternally() {
        KUrl url(cbxEntryToUrl[urlComboBox->currentText()]);
        KRun::runUrl(url, KIO::NetAccess::mimetype(url, p), p);
    }

    QPair<QString, KIcon> mimeType(const KUrl &url) {
        if (!url.isLocalFile() && url.fileName().isEmpty()) {
            /// URLs not pointing to a specific file should be opened with a web browser component
            kDebug() << "Falling back to text/html";
            return QPair<QString, KIcon>(QLatin1String("text/html"), KIcon("text-html"));
        }

        int accuracy = 0;
        KMimeType::Ptr mimeTypePtr = KMimeType::findByUrl(url, 0, url.isLocalFile(), true, &accuracy);
        if (accuracy < 50) {
            kDebug() << "discarding mime type " << mimeTypePtr->name() << ", trying filename ";
            mimeTypePtr = KMimeType::findByPath(url.fileName(), 0, true, &accuracy);
        }
        QString mimeTypeName = mimeTypePtr->name();

        if (mimeTypeName == QLatin1String("application/octet-stream")) {
            /// application/octet-stream is a fall-back if KDE did not know better
            kDebug() << "Falling back to text/html";
            return QPair<QString, KIcon>(QLatin1String("text/html"), KIcon("text-html"));
        }

        KIcon icon = KIcon(mimeTypePtr->iconName());
        kDebug() << "For url " << url.prettyUrl() << " selected mime type " << mimeTypeName << " (icon name " << mimeTypePtr->iconName() << ")";

        return QPair<QString, KIcon>(mimeTypeName, icon);
    }

    void statJobFinished(KIO::StatJob *job) {
        Q_ASSERT(runningStatJob == job);
        runningStatJob = NULL;

        if (job->error() == 0) {
            KParts::ReadOnlyPart* part = NULL;
            KUrl url(job->url());
            QPair<QString, KIcon> mimeTypeIcon = mimeType(url);
            KService::Ptr serivcePtr = KMimeTypeTrader::self()->preferredService(mimeTypeIcon.first, "KParts/ReadOnlyPart");
            if (!serivcePtr.isNull())
                part = serivcePtr->createInstance<KParts::ReadOnlyPart>((QWidget*)p, (QObject*)p);

            urlToWidget.remove(url);
            if (part != NULL) {
                urlToWidget.insert(url, part->widget());
                part->openUrl(url);
            } else {
                QLabel *label = new QLabel(i18n("Cannot create preview for\n%1", url.prettyUrl()), p);
                label->setAlignment(Qt::AlignCenter);
                urlToWidget.insert(url, label);
            }

            switchWidget(urlToWidget[url]);
        } else {
            kDebug() << job->url() << " does not exist";
            QLabel *label = dynamic_cast<QLabel*>(urlToWidget[job->url()]);
            if (label != NULL)
                label->setText(i18n("File does not exist:\n%1", job->url().prettyUrl()));
        }

        p->unsetCursor();
    }
};

UrlPreview::UrlPreview(QWidget *parent)
        : QWidget(parent), d(new UrlPreviewPrivate(this))
{
    setEnabled(false);
}

void UrlPreview::setElement(Element* element, const File *)
{
    d->entry = dynamic_cast<const Entry*>(element);
    d->update();
}

void UrlPreview::urlSelected(const QString &text)
{
    d->urlSelected(text);
}

void UrlPreview::openExternally()
{
    d->openExternally();
}

void UrlPreview::statJobFinished(KJob *job)
{
    d->statJobFinished(static_cast<KIO::StatJob*>(job));
}

void UrlPreview::setBibTeXUrl(const KUrl&url)
{
    d->baseUrl = url;
}
