/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <QMap>
#include <QFileInfo>
#include <QResizeEvent>
#include <QCheckBox>
#include <QStackedWidget>
#include <QDockWidget>
#include <QMutex>

#include <KLocale>
#include <KComboBox>
#include <KIcon>
#include <KRun>
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

ImageLabel::ImageLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
        : QLabel(text, parent, f)
{
    // nothing
}

void ImageLabel::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    if (!m_pixmap.isNull()) {
        setCursor(Qt::WaitCursor);
        QPixmap scaledPixmap = m_pixmap.width() <= width() && m_pixmap.height() <= height() ? m_pixmap : pixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel::setPixmap(scaledPixmap);
        setMinimumSize(100, 100);
        unsetCursor();
    } else
        QLabel::setPixmap(m_pixmap);
}

void ImageLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    if (!m_pixmap.isNull()) {
        setCursor(Qt::WaitCursor);
        QPixmap scaledPixmap = m_pixmap.width() <= event->size().width() && m_pixmap.height() <= event->size().height() ? m_pixmap : m_pixmap.scaled(event->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel::setPixmap(scaledPixmap);
        setMinimumSize(100, 100);
        unsetCursor();
    }
}

class UrlPreview::UrlPreviewPrivate
{
public:
    struct UrlInfo {
        KUrl url;
        QString mimeType;
        KIcon icon;
    };

private:
    UrlPreview *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString onlyLocalFilesCheckConfig;

    KComboBox *urlComboBox;
    KPushButton *externalViewerButton;
    QStackedWidget *stackedWidget;
    ImageLabel *message;
    QMap<int, struct UrlInfo> cbxEntryToUrlInfo;
    QMutex addingUrlMutex;

    static const QString arXivPDFUrlStart;
    bool anyLocal;

    KParts::ReadOnlyPart *okularPart;
    KParts::ReadOnlyPart *htmlPart;
    int swpMessage, swpOkular, swpHTML;

public:
    QCheckBox *onlyLocalFilesCheckBox;
    QList<KIO::StatJob*> runningJobs;
    const Entry *entry;
    KUrl baseUrl;

    KParts::ReadOnlyPart *locatePart(const QString &desktopFile, QWidget *parentWidget) {
        KService::Ptr service = KService::serviceByDesktopPath(desktopFile);
        if (!service.isNull()) {
            KParts::ReadOnlyPart *part = service->createInstance<KParts::ReadOnlyPart>(parentWidget, p);
            connect(part, SIGNAL(completed()), p, SLOT(loadingFinished()));
            return part;
        } else
            return NULL;
    }

    UrlPreviewPrivate(UrlPreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), entry(NULL) {
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

        /// main part of the widget

        stackedWidget = new QStackedWidget(p);
        layout->addWidget(stackedWidget, 1);

        /// default widget if no preview is available
        message = new ImageLabel(i18n("No preview available"), stackedWidget);
        message->setAlignment(Qt::AlignCenter);
        message->setWordWrap(true);
        swpMessage = stackedWidget->addWidget(message);
        connect(message, SIGNAL(linkActivated(QString)), p, SLOT(linkActivated(QString)));

        /// add parts to stackedWidget
        okularPart = locatePart(QLatin1String("okularPoppler.desktop"), stackedWidget);
        swpOkular = (okularPart == NULL) ? -1 : stackedWidget->addWidget(okularPart->widget());
        if (okularPart == NULL || swpOkular < 0) {
            kWarning() << "No Okular part for PDF or PostScript document preview available.";
        }
        htmlPart = locatePart(QLatin1String("khtml.desktop"), stackedWidget);
        swpHTML = stackedWidget->addWidget(htmlPart->widget());

        loadState();

        connect(externalViewerButton, SIGNAL(clicked()), p, SLOT(openExternally()));
        connect(urlComboBox, SIGNAL(activated(int)), p, SLOT(comboBoxChanged(int)));
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
        urlComboBox->setEnabled(true);
        cbxEntryToUrlInfo.insert(urlComboBox->count() - 1, urlInfo);

        externalViewerButton->setEnabled(true);
        if (urlComboBox->count() == 1 || ///< first entry in combobox
                isLocal || ///< local files always preferred over URLs
                /// prefer arXiv summary URLs over other URLs
                (!anyLocal && urlInfo.url.host().contains("arxiv.org/abs"))) {
            showUrl(urlInfo);
        }

        return true;
    }

    void update() {
        p->setCursor(Qt::WaitCursor);

        /// reset and clear all controls
        okularPart->closeUrl();
        htmlPart->closeUrl();
        urlComboBox->setEnabled(false);
        urlComboBox->clear();
        cbxEntryToUrlInfo.clear();
        externalViewerButton->setEnabled(false);
        showMessage(i18n("Refreshing ..."));

        /// cancel/kill all running jobs
        for (QList<KIO::StatJob*>::ConstIterator it = runningJobs.constBegin(); it != runningJobs.constEnd(); ++it)
            (*it)->kill();
        runningJobs.clear();

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
            if (urlList.isEmpty()) {
                showMessage(i18n("No documents to show."));
                p->setCursor(Qt::ArrowCursor);
            } else if (runningJobs.isEmpty()) {
                showMessage(i18n("<qt>No documents to show.<br/><a href=\"disableonlylocalfiles\">Disable the restriction</a> to local files to see remote documents.</qt>"));
                p->setCursor(Qt::ArrowCursor);
            }
        }
    }

    void showMessage(const QString &msgText) {
        stackedWidget->setCurrentIndex(swpMessage);
        message->setPixmap(QPixmap());
        message->setText(msgText);
        if (swpOkular >= 0)
            stackedWidget->widget(swpOkular)->setEnabled(false);
        stackedWidget->widget(swpHTML)->setEnabled(false);
    }

    void showPart(const KParts::ReadOnlyPart *part, QWidget *widget) {
        if (part == okularPart && swpOkular >= 0) {
            stackedWidget->setCurrentIndex(swpOkular);
            stackedWidget->widget(swpOkular)->setEnabled(true);
        } else if (part == htmlPart) {
            stackedWidget->setCurrentIndex(swpHTML);
            stackedWidget->widget(swpHTML)->setEnabled(true);
        } else if (widget == message) {
            stackedWidget->setCurrentIndex(swpMessage);
        } else
            showMessage(i18n("Cannot show requested part"));
    }

    bool showUrl(const struct UrlInfo &urlInfo) {
        static const QStringList okularMimetypes = QStringList() << QLatin1String("application/x-pdf") << QLatin1String("application/pdf") << QLatin1String("application/x-gzpdf") << QLatin1String("application/x-bzpdf") << QLatin1String("application/x-wwf") << QLatin1String("image/vnd.djvu") << QLatin1String("application/postscript") << QLatin1String("image/x-eps") << QLatin1String("application/x-gzpostscript") << QLatin1String("application/x-bzpostscript") << QLatin1String("image/x-gzeps") << QLatin1String("image/x-bzeps");
        static const QStringList htmlMimetypes = QStringList() << QLatin1String("text/html") << QLatin1String("application/xml") << QLatin1String("application/xhtml+xml");
        static const QStringList imageMimetypes = QStringList() << QLatin1String("image/jpeg") << QLatin1String("image/png") << QLatin1String("image/gif") << QLatin1String("image/tiff");

        if (swpOkular >= 0 && okularPart != NULL) {
            stackedWidget->widget(swpOkular)->setEnabled(false);
            okularPart->closeUrl();
        }
        okularPart->closeUrl();
        htmlPart->closeUrl();

        if (okularPart != NULL && okularMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            showMessage(i18n("Loading ..."));
            okularPart->openUrl(urlInfo.url);
            return true;
        } else if (htmlMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            showMessage(i18n("Loading ..."));
            htmlPart->openUrl(urlInfo.url);
            return true;
        } else if (imageMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            message->setPixmap(QPixmap(urlInfo.url.pathOrUrl()));
            showPart(NULL, message);
            p->unsetCursor();
            return true;
        } else
            showMessage(i18n("<qt>Don't know how to show mimetype '%1'.</qt>", urlInfo.mimeType));

        return false;
    }

    void openExternally() {
        KUrl url(cbxEntryToUrlInfo[urlComboBox->currentIndex()].url);
        /// Guess mime type for url to open
        KMimeType::Ptr mimeType = KMimeType::findByPath(url.path());
        QString mimeTypeName = mimeType->name();
        if (mimeTypeName == QLatin1String("application/octet-stream"))
            mimeTypeName = QLatin1String("text/html");
        /// Ask KDE subsystem to open url in viewer matching mime type
        KRun::runUrl(url, mimeTypeName, p, false, false);
    }

    UrlInfo urlMetaInfo(const KUrl &url) {
        UrlInfo result;
        result.url = url;

        if (!url.isLocalFile() && url.fileName().isEmpty()) {
            /// URLs not pointing to a specific file should be opened with a web browser component
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
            return result;
        }

        int accuracy = 0;
        KMimeType::Ptr mimeTypePtr = KMimeType::findByUrl(url, 0, url.isLocalFile(), true, &accuracy);
        if (accuracy < 50) {
            mimeTypePtr = KMimeType::findByPath(url.fileName(), 0, true, &accuracy);
        }
        result.mimeType = mimeTypePtr->name();
        result.icon = KIcon(mimeTypePtr->iconName());

        if (result.mimeType == QLatin1String("application/octet-stream")) {
            /// application/octet-stream is a fall-back if KDE did not know better
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
        } else if (result.mimeType == QLatin1String("inode/directory") && (result.url.protocol() == QLatin1String("http") || result.url.protocol() == QLatin1String("https"))) {
            /// directory via http means normal webpage (not browsable directory)
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

    void comboBoxChanged(int index) {
        showUrl(cbxEntryToUrlInfo[index]);
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

const QString UrlPreview::UrlPreviewPrivate::arXivPDFUrlStart = QLatin1String("http://arxiv.org/pdf/");
const QString UrlPreview::UrlPreviewPrivate::configGroupName = QLatin1String("URL Preview");
const QString UrlPreview::UrlPreviewPrivate::onlyLocalFilesCheckConfig = QLatin1String("OnlyLocalFiles");

UrlPreview::UrlPreview(QDockWidget *parent)
        : QWidget(parent), d(new UrlPreviewPrivate(this))
{
    connect(parent, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
}

UrlPreview::~UrlPreview()
{
    delete d;
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

void UrlPreview::comboBoxChanged(int index)
{
    d->comboBoxChanged(index);
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
        setCursor(d->runningJobs.isEmpty() ? Qt::ArrowCursor : Qt::BusyCursor);
        d->addUrl(urlInfo);
    } else {
        kDebug() << job->errorString();
        setCursor(d->runningJobs.isEmpty() ? Qt::ArrowCursor : Qt::BusyCursor);
    }
}

void UrlPreview::loadingFinished()
{
    setCursor(Qt::ArrowCursor);
    d->showPart(dynamic_cast<KParts::ReadOnlyPart*>(sender()), dynamic_cast<QWidget*>(sender()));
}

void UrlPreview::linkActivated(const QString &link)
{
    if (link == QLatin1String("disableonlylocalfiles"))
        d->onlyLocalFilesCheckBox->setChecked(false);
}
