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

#include <typeinfo>

#include <QList>
#include <QLayout>
#include <QLabel>
#include <QMap>
#include <QFileInfo>
#include <QDesktopServices>
#include <QCheckBox>
#include <QStackedWidget>
#include <QDockWidget>
#include <QMutex>

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
#include <KDebug>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KTemporaryFile>

#include <element.h>
#include <entry.h>
#include <file.h>
#include <fileinfo.h>
#include "urlpreview.h"

class UrlPreview::UrlPreviewPrivate
{
private:
    UrlPreview *p;

    KSharedConfigPtr config;
    const QString configGroupName;
    const QString onlyLocalFilesCheckConfig;

    KComboBox *urlComboBox;
    KPushButton *externalViewerButton;
    QCheckBox *onlyLocalFilesCheckBox;
    QStackedWidget *stackedWidget;
    QLabel *message;
    QMap<int, KUrl> cbxEntryToUrl;
    QMutex addingUrlMutex;

    QString arXivPDFUrlStart;
    bool anyLocal;

public:
    struct UrlInfo {
        KUrl url;
        QString mimeType;
        KIcon icon;
    };

    QList<KIO::StatJob*> runningJobs;
    const Entry* entry;
    KUrl baseUrl;

    UrlPreviewPrivate(UrlPreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
            configGroupName(QLatin1String("URL Preview")), onlyLocalFilesCheckConfig(QLatin1String("OnlyLocalFiles")),
            arXivPDFUrlStart("http://arxiv.org/pdf/"), entry(NULL) {
        setupGUI();
    }

    /**
      * Create user interface for this widget.
      * It consists of some controlling widget on the top,
      * but the most space is consumed by KPart widgets
      * inside a QStackedWidget to show the external content
      * (PDF file, web page, ...).
      */
    void setupGUI() {
        QVBoxLayout *layout = new QVBoxLayout(p);
        layout->setMargin(0);

        /// some widgets on the top to control the view

        QHBoxLayout *innerLayout = new QHBoxLayout();
        layout->addLayout(innerLayout, 0);
        urlComboBox = new KComboBox(false, p);
        innerLayout->addWidget(urlComboBox, 1);

        externalViewerButton = new KPushButton(KIcon("document-open"), i18n("Open..."), p);
        innerLayout->addWidget(externalViewerButton, 0);

        onlyLocalFilesCheckBox = new QCheckBox(i18n("Only local files"), p);
        layout->addWidget(onlyLocalFilesCheckBox, 0);

        /// main part of the widget: A stacked widget to hold
        /// one KPart widget per URL in above combo box

        stackedWidget = new QStackedWidget(p);
        layout->addWidget(stackedWidget, 1);
        stackedWidget->hide();

        /// default widget if no preview is available
        message = new QLabel(i18n("No preview available"), p);
        message->setAlignment(Qt::AlignCenter);
        layout->addWidget(message, 1);

        loadState();

        connect(externalViewerButton, SIGNAL(clicked()), p, SLOT(openExternally()));
        connect(urlComboBox, SIGNAL(activated(int)), stackedWidget, SLOT(setCurrentIndex(int)));
        connect(onlyLocalFilesCheckBox, SIGNAL(toggled(bool)), p, SLOT(onlyLocalFilesChanged()));
    }

    bool addUrl(const struct UrlInfo &urlInfo) {
        bool isLocal = urlInfo.url.isLocalFile();
        anyLocal |= isLocal;

        if (onlyLocalFilesCheckBox->isChecked() && !isLocal) return true; ///< ignore URL if only local files are allowed

        if (isLocal) {
            /// create a drop-down list entry if file is a local file
            /// (based on patch by Luis Silva)
            QString fn = urlInfo.url.fileName();
            QString full = urlInfo.url.pathOrUrl();
            QString dir = urlInfo.url.directory();
            QString text = fn.isEmpty() ? full : (dir.isEmpty() ? fn : QString("%1 [%2]").arg(fn).arg(dir));
            urlComboBox->addItem(urlInfo.icon, text);
        } else {
            /// create a drop-down list entry if file is a remote file
            urlComboBox->addItem(urlInfo.icon, urlInfo.url.prettyUrl());
        }
        cbxEntryToUrl.insert(urlComboBox->count() - 1, urlInfo.url);

        KParts::ReadOnlyPart* part = NULL;
        KService::Ptr serivcePtr = KMimeTypeTrader::self()->preferredService(urlInfo.mimeType, "KParts/ReadOnlyPart");
        if (!serivcePtr.isNull())
            part = serivcePtr->createInstance<KParts::ReadOnlyPart>((QWidget*)p, (QObject*)p);
        if (part != NULL) {
            stackedWidget->addWidget(part->widget());
            part->openUrl(urlInfo.url);
        } else {
            QLabel *label = new QLabel(i18n("Cannot create preview for\n%1\n\nNo part available.", urlInfo.url.pathOrUrl()), stackedWidget);
            label->setAlignment(Qt::AlignCenter);
            stackedWidget->addWidget(label);
        }

        urlComboBox->setEnabled(true);
        externalViewerButton->setEnabled(true);
        if (isLocal || ///< local files always preferred over URLs
                /// prefer arXiv summary URLs over other URLs
                (!anyLocal && urlInfo.url.host().contains("arxiv.org/abs"))) {
            urlComboBox->setCurrentIndex(urlComboBox->count() - 1);
            stackedWidget->setCurrentIndex(urlComboBox->count() - 1);
        }
        message->hide();
        stackedWidget->show();

        return true;
    }

    void update() {
        QCursor prevCursor = p->cursor();
        p->setCursor(Qt::WaitCursor);

        /// cancel/kill all running jobs
        for (QList<KIO::StatJob*>::ConstIterator it = runningJobs.constBegin(); it != runningJobs.constEnd(); ++it)
            (*it)->kill();
        runningJobs.clear();
        /// remove all shown widgets/parts
        urlComboBox->clear();
        while (stackedWidget->count() > 0)
            stackedWidget->removeWidget(stackedWidget->currentWidget());
        urlComboBox->setEnabled(false);
        externalViewerButton->setEnabled(false);

        /// clear flag that memorizes if any local file was referenced
        anyLocal = false;

        /// do not load external reference if widget is hidden
        if (isVisible()) {
            QList<KUrl> urlList = FileInfo::entryUrls(entry, baseUrl);
            for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
                bool isLocal = (*it).isLocalFile();
                if (onlyLocalFilesCheckBox->isChecked() && !isLocal) continue;

                KIO::StatJob *job = KIO::stat(*it, KIO::StatJob::SourceSide, 3, KIO::HideProgressInfo);
                runningJobs << job;
                job->ui()->setWindow(p);
                connect(job, SIGNAL(result(KJob*)), p, SLOT(statFinished(KJob*)));
            }
        }

        message->setText(i18n("No preview available"));
        message->show();
        stackedWidget->hide();
        p->setEnabled(isVisible());

        p->setCursor(prevCursor);
    }

    void openExternally() {
        KUrl url(cbxEntryToUrl[urlComboBox->currentIndex()]);
        QDesktopServices::openUrl(url); // TODO KDE way?
    }

    UrlInfo urlMetaInfo(const KUrl &url) {
        UrlInfo result;
        result.url = url;

        if (!url.isLocalFile() && url.fileName().isEmpty()) {
            /// URLs not pointing to a specific file should be opened with a web browser component
            kDebug() << "Not pointing to file, falling back to text/html for url " << url.pathOrUrl();
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
            return result;
        }

        int accuracy = 0;
        KMimeType::Ptr mimeTypePtr = KMimeType::findByUrl(url, 0, url.isLocalFile(), true, &accuracy);
        if (accuracy < 50) {
            kDebug() << "discarding mime type " << mimeTypePtr->name() << ", trying filename ";
            mimeTypePtr = KMimeType::findByPath(url.fileName(), 0, true, &accuracy);
        }
        result.mimeType = mimeTypePtr->name();
        result.icon = KIcon(mimeTypePtr->iconName());

        if (result.mimeType == QLatin1String("application/octet-stream")) {
            /// application/octet-stream is a fall-back if KDE did not know better
            kDebug() << "Got mime type \"application/octet-stream\", falling back to text/html";
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
        } else if (result.mimeType == QLatin1String("inode/directory") && (result.url.protocol() == QLatin1String("http") || result.url.protocol() == QLatin1String("https"))) {
            /// directory via http means normal webpage (not browsable directory)
            kDebug() << "Got mime type \"inode/directory\" via http, falling back to text/html";
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
        }

        if (url.pathOrUrl().startsWith(arXivPDFUrlStart)) {
            kDebug() << "URL looks like a PDF url from arXiv";
            result.icon = KIcon("application-pdf");
            result.mimeType = QLatin1String("application/pdf");
        }

        kDebug() << "For url " << result.url.pathOrUrl() << " selected mime type " << result.mimeType;
        return result;
    }

    bool isVisible() {
        /// get dock where this widget is inside
        /// static cast is save as constructor requires parent to be QDockWidget
        QDockWidget *pp = static_cast<QDockWidget*>(p->parent());
        return pp != NULL && !pp->isHidden();
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        onlyLocalFilesCheckBox->setChecked(configGroup.readEntry(onlyLocalFilesCheckConfig, true));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(onlyLocalFilesCheckConfig, onlyLocalFilesCheckBox->isChecked());
        config->sync();
    }
};

UrlPreview::UrlPreview(QDockWidget *parent)
        : QWidget(parent), d(new UrlPreviewPrivate(this))
{
    connect(parent, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
}

void UrlPreview::setElement(Element* element, const File *)
{
    d->entry = dynamic_cast<const Entry*>(element);
    d->update();
}

void UrlPreview::openExternally()
{
    d->openExternally();
}

void UrlPreview::setBibTeXUrl(const KUrl&url)
{
    d->baseUrl = url;
}

void UrlPreview::onlyLocalFilesChanged()
{
    d->saveState();
    d->update();
}

void UrlPreview::visibilityChanged(bool)
{
    d->update();
}

void UrlPreview::statFinished(KJob *kjob)
{
    KIO::StatJob *job = static_cast<KIO::StatJob*>(kjob);
    d->runningJobs.removeOne(job);
    if (!job->error()) {
#if KDE_VERSION_MINOR >= 4
        const KUrl url = job->mostLocalUrl();
#else // KDE_VERSION_MINOR
        const KUrl url = job->url();
#endif // KDE_VERSION_MINOR
        kDebug() << "stat succeeded for " << url.pathOrUrl();
        UrlPreviewPrivate::UrlInfo urlInfo = d->urlMetaInfo(url);
        d->addUrl(urlInfo);
    } else
        kDebug() << job->errorString();
}
