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
#ifdef HAVE_QTWEBKIT
#include <QWebView>
#endif // HAVE_QTWEBKIT

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
#include <KMenuBar>
#include <KToolBar>
#include <KActionCollection>

#include <kbibtexnamespace.h>
#include <element.h>
#include <entry.h>
#include <file.h>
#include <fileinfo.h>
#include "documentpreview.h"

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

class DocumentPreview::DocumentPreviewPrivate
{
public:
    struct UrlInfo {
        KUrl url;
        QString mimeType;
        KIcon icon;
    };

private:
    DocumentPreview *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString onlyLocalFilesCheckConfig;

    KPushButton *externalViewerButton;
    QStackedWidget *stackedWidget;
    ImageLabel *message;
    QMap<int, struct UrlInfo> cbxEntryToUrlInfo;
    QMutex addingUrlMutex;

    static const QString arXivPDFUrlStart;
    bool anyLocal;

    KMenuBar *menuBar;
    KToolBar *toolBar;
    KParts::ReadOnlyPart *okularPart;
#ifdef HAVE_QTWEBKIT
    QWebView *htmlWidget;
#else // HAVE_QTWEBKIT
    KParts::ReadOnlyPart *htmlPart;
#endif // HAVE_QTWEBKIT
    int swpMessage, swpOkular, swpHTML;

public:
    KComboBox *urlComboBox;
    KPushButton *onlyLocalFilesButton;
    QList<KIO::StatJob *> runningJobs;
    QSharedPointer<const Entry> entry;
    KUrl baseUrl;
    bool anyRemote;

    KParts::ReadOnlyPart *locatePart(const QString &desktopFile, QWidget *parentWidget) {
        KService::Ptr service = KService::serviceByDesktopPath(desktopFile);
        if (!service.isNull()) {
            KParts::ReadOnlyPart *part = service->createInstance<KParts::ReadOnlyPart>(parentWidget, p);
            connect(part, SIGNAL(completed()), p, SLOT(loadingFinished()));
            return part;
        } else
            return NULL;
    }

    DocumentPreviewPrivate(DocumentPreview *parent)
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

        onlyLocalFilesButton = new KPushButton(KIcon("applications-internet"), QString::null, p);
        onlyLocalFilesButton->setToolTip(i18n("Toggle between local files only and all documents including remote ones"));
        innerLayout->addWidget(onlyLocalFilesButton, 0);
        onlyLocalFilesButton->setCheckable(true);
        QSizePolicy sp = onlyLocalFilesButton->sizePolicy();
        sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
        onlyLocalFilesButton->setSizePolicy(sp);

        urlComboBox = new KComboBox(false, p);
        innerLayout->addWidget(urlComboBox, 1);

        externalViewerButton = new KPushButton(KIcon("document-open"), QString::null, p);
        externalViewerButton->setToolTip(i18n("Open in external program"));
        innerLayout->addWidget(externalViewerButton, 0);
        sp = externalViewerButton->sizePolicy();
        sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
        externalViewerButton->setSizePolicy(sp);

        menuBar = new KMenuBar(p);
        menuBar->setBackgroundRole(QPalette::Window);
        menuBar->setVisible(false);
        layout->addWidget(menuBar, 0);

        toolBar = new KToolBar(p);
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolBar->setBackgroundRole(QPalette::Window);
        toolBar->setVisible(false);
        layout->addWidget(toolBar, 0);

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
#ifdef HAVE_QTWEBKIT
        kDebug() << "WebKit is available, using it instead of KHTML for HTML/Web preview.";
        htmlWidget = new QWebView(stackedWidget);
        swpHTML = stackedWidget->addWidget(htmlWidget);
        connect(htmlWidget, SIGNAL(loadFinished(bool)), p, SLOT(loadingFinished()));
#else // HAVE_QTWEBKIT
        kDebug() << "WebKit not is available, using KHTML instead for HTML/Web preview.";
        htmlPart = locatePart(QLatin1String("khtml.desktop"), stackedWidget);
        swpHTML = stackedWidget->addWidget(htmlPart->widget());
#endif // HAVE_QTWEBKIT

        loadState();

        connect(externalViewerButton, SIGNAL(clicked()), p, SLOT(openExternally()));
        connect(urlComboBox, SIGNAL(activated(int)), p, SLOT(comboBoxChanged(int)));
        connect(onlyLocalFilesButton, SIGNAL(toggled(bool)), p, SLOT(onlyLocalFilesChanged()));
    }

    bool addUrl(const struct UrlInfo &urlInfo) {
        bool isLocal = KBibTeX::isLocalOrRelative(urlInfo.url);
        anyLocal |= isLocal;

        if (!onlyLocalFilesButton->isChecked() && !isLocal) return true; ///< ignore URL if only local files are allowed

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
        if (okularPart != NULL)
            okularPart->closeUrl();
#ifdef HAVE_QTWEBKIT
        htmlWidget->stop();
#else // HAVE_QTWEBKIT
        htmlPart->closeUrl();
#endif // HAVE_QTWEBKIT
        urlComboBox->setEnabled(false);
        urlComboBox->clear();
        cbxEntryToUrlInfo.clear();
        externalViewerButton->setEnabled(false);
        showMessage(i18n("Refreshing ..."));

        /// cancel/kill all running jobs
        for (QList<KIO::StatJob *>::ConstIterator it = runningJobs.constBegin(); it != runningJobs.constEnd(); ++it)
            (*it)->kill();
        runningJobs.clear();

        /// clear flag that memorizes if any local file was referenced
        anyLocal = false;
        anyRemote = false;

        /// do not load external reference if widget is hidden
        if (isVisible()) {
            QList<KUrl> urlList = FileInfo::entryUrls(entry.data(), baseUrl, FileInfo::TestExistanceYes);

            for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
                bool isLocal = KBibTeX::isLocalOrRelative(*it);
                kDebug() << "testing " << (*it).prettyUrl() << isLocal;
                anyRemote |= !isLocal;
                if (!onlyLocalFilesButton->isChecked() && !isLocal) continue;

                KIO::StatJob *job = KIO::stat(*it, KIO::StatJob::SourceSide, 3, KIO::HideProgressInfo);
                runningJobs << job;
                job->ui()->setWindow(p);
                connect(job, SIGNAL(result(KJob *)), p, SLOT(statFinished(KJob *)));
            }
            if (urlList.isEmpty()) {
                /// Case no URLs associated with this entry.
                /// For-loop above was never executed.
                showMessage(i18n("No documents to show."));
                p->setCursor(Qt::ArrowCursor);
            } else if (runningJobs.isEmpty()) {
                /// Case no stat jobs are running. As there were URLs (tested in
                /// previous condition), this implies that there were remote
                /// references that were ignored by executing "continue" above.
                /// Give user hint that by enabling remote files, more can be shown.
                showMessage(i18n("<qt>No documents to show.<br/><a href=\"disableonlylocalfiles\">Disable the restriction</a> to local files to see remote documents.</qt>"));
                p->setCursor(Qt::ArrowCursor);
            }
        } else
            p->setCursor(Qt::ArrowCursor);
    }

    void showMessage(const QString &msgText) {
        stackedWidget->setCurrentIndex(swpMessage);
        message->setPixmap(QPixmap());
        message->setText(msgText);
        if (swpOkular >= 0)
            stackedWidget->widget(swpOkular)->setEnabled(false);
        stackedWidget->widget(swpHTML)->setEnabled(false);
        menuBar->setVisible(false);
        toolBar->setVisible(true);
        menuBar->clear();
        toolBar->clear();
    }


    void setupToolMenuBarForPart(const KParts::ReadOnlyPart *part) {
        /*
        KAction *printAction = KStandardAction::print(part, SLOT(slotPrint()), part->actionCollection());
        printAction->setEnabled(false);
        connect(part, SIGNAL(enablePrintAction(bool)), printAction, SLOT(setEnabled(bool)));
        */

        QDomDocument doc = part->domDocument();
        QDomElement docElem = doc.documentElement();

        QDomNodeList toolbarNodes = docElem.elementsByTagName("ToolBar");
        for (int i = 0; i < toolbarNodes.count(); ++i) {
            QDomNodeList toolbarItems = toolbarNodes.at(i).childNodes();
            for (int j = 0; j < toolbarItems.count(); ++j) {
                QDomNode toolbarItem = toolbarItems.at(j);
                if (toolbarItem.nodeName() == QLatin1String("Action")) {
                    QString actionName = toolbarItem.attributes().namedItem(QLatin1String("name")).nodeValue();
                    toolBar->addAction(part->actionCollection()->action(actionName));
                } else if (toolbarItem.nodeName() == QLatin1String("Separator")) {
                    toolBar->addSeparator();
                }
            }
        }


        QDomNodeList menubarNodes = docElem.elementsByTagName("MenuBar");
        for (int i = 0; i < menubarNodes.count(); ++i) {
            QDomNodeList menubarNode = menubarNodes.at(i).childNodes();
            for (int j = 0; j < menubarNode.count(); ++j) {
                QDomNode menubarItem = menubarNode.at(j);
                if (menubarItem.nodeName() == QLatin1String("Menu")) {
                    QDomNodeList menuNode = menubarItem.childNodes();
                    QString text;
                    for (int k = 0; k < menuNode.count(); ++k) {
                        QDomNode menuItem = menuNode.at(k);
                        if (menuItem.nodeName() == QLatin1String("text")) {
                            text = menuItem.firstChild().toText().data();
                            break;
                        }
                    }
                    QMenu *menu = menuBar->addMenu(text);

                    for (int k = 0; k < menuNode.count(); ++k) {
                        QDomNode menuItem = menuNode.at(k);
                        if (menuItem.nodeName() == QLatin1String("Action")) {
                            QString actionName = menuItem.attributes().namedItem(QLatin1String("name")).nodeValue();
                            menu->addAction(part->actionCollection()->action(actionName));
                        } else if (menuItem.nodeName() == QLatin1String("Separator")) {
                            menu->addSeparator();
                        }
                    }
                }
            }
        }

        QDomNodeList actionPropertiesList = docElem.elementsByTagName("ActionProperties");
        for (int i = 0; i < actionPropertiesList.count(); ++i) {
            QDomNodeList actionProperties = actionPropertiesList.at(i).childNodes();
            for (int j = 0; j < actionProperties.count(); ++j) {
                QDomNode actionNode = actionProperties.at(j);
                if (actionNode.nodeName() == QLatin1String("Action")) {
                    kDebug() << actionNode.attributes().namedItem("name").isNull() << actionNode.attributes().namedItem("name").isAttr();

                    const QString actionName = actionNode.attributes().namedItem("name").toAttr().nodeValue();
                    const QString actionShortcut = actionNode.attributes().namedItem("shortcut").toAttr().value();
                    kDebug() << actionName << actionShortcut;
                    QAction *action = part->actionCollection()->action(actionName);
                    if (action != NULL) {
                        kDebug() << "setting shortcut" << actionShortcut << "to action" << actionName;
                        action->setShortcut(QKeySequence(actionShortcut));
                    } else
                        kDebug() << "Could not locate an action with name " << actionName << "for shortcut" << actionShortcut;
                }
            }
        }

        menuBar->setVisible(true);
        toolBar->setVisible(true);
    }


    void showPart(const KParts::ReadOnlyPart *part, QWidget *widget) {
        menuBar->setVisible(false);
        toolBar->setVisible(false);
        menuBar->clear();
        toolBar->clear();

        if (part == okularPart && swpOkular >= 0) {
            stackedWidget->setCurrentIndex(swpOkular);
            stackedWidget->widget(swpOkular)->setEnabled(true);
            setupToolMenuBarForPart(okularPart);
#ifdef HAVE_QTWEBKIT
        } else if (widget == htmlWidget) {
            stackedWidget->setCurrentIndex(swpHTML);
            stackedWidget->widget(swpHTML)->setEnabled(true);
#else // HAVE_QTWEBKIT
        } else if (part == htmlPart) {
            stackedWidget->setCurrentIndex(swpHTML);
            stackedWidget->widget(swpHTML)->setEnabled(true);
            setupToolMenuBarForPart(htmlPart);
#endif // HAVE_QTWEBKIT
        } else if (widget == message) {
            stackedWidget->setCurrentIndex(swpMessage);
        } else
            showMessage(i18n("Cannot show requested part"));
    }

    bool showUrl(const struct UrlInfo &urlInfo) {
        static const QStringList okularMimetypes = QStringList() << QLatin1String("application/x-pdf") << QLatin1String("application/pdf") << QLatin1String("application/x-gzpdf") << QLatin1String("application/x-bzpdf") << QLatin1String("application/x-wwf") << QLatin1String("image/vnd.djvu") << QLatin1String("application/postscript") << QLatin1String("image/x-eps") << QLatin1String("application/x-gzpostscript") << QLatin1String("application/x-bzpostscript") << QLatin1String("image/x-gzeps") << QLatin1String("image/x-bzeps");
        static const QStringList htmlMimetypes = QStringList() << QLatin1String("text/html") << QLatin1String("application/xml") << QLatin1String("application/xhtml+xml");
        static const QStringList imageMimetypes = QStringList() << QLatin1String("image/jpeg") << QLatin1String("image/png") << QLatin1String("image/gif") << QLatin1String("image/tiff");

        stackedWidget->widget(swpHTML)->setEnabled(false);
        if (swpOkular >= 0 && okularPart != NULL) {
            stackedWidget->widget(swpOkular)->setEnabled(false);
            okularPart->closeUrl();
        }
#ifdef HAVE_QTWEBKIT
        htmlWidget->stop();
#else // HAVE_QTWEBKIT
        htmlPart->closeUrl();
#endif // HAVE_QTWEBKIT

        if (okularPart != NULL && okularMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            showMessage(i18n("Loading ..."));
            okularPart->openUrl(urlInfo.url);
            return true;
        } else if (htmlMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            showMessage(i18n("Loading ..."));
#ifdef HAVE_QTWEBKIT
            htmlWidget->load(urlInfo.url);
#else // HAVE_QTWEBKIT
            htmlPart->openUrl(urlInfo.url);
#endif // HAVE_QTWEBKIT
            return true;
        } else if (imageMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            message->setPixmap(QPixmap(urlInfo.url.pathOrUrl()));
            showPart(NULL, message);
            p->unsetCursor();
            return true;
        } else {
            QString additionalInformation;
            if (urlInfo.mimeType == QLatin1String("application/pdf"))
                additionalInformation = i18nc("Additional information in case there is not KPart available for mime type 'application/pdf'", "<br/><br/>Please install <a href=\"https://userbase.kde.org/Okular\">Okular</a> to make use of its PDF viewing component.");
            showMessage(i18nc("First parameter is mime type, second parameter is optional information (may be empty)", "<qt>Don't know how to show mimetype '%1'.%2</qt>", urlInfo.mimeType, additionalInformation));
        }

        return false;
    }

    void openExternally() {
        KUrl url(cbxEntryToUrlInfo[urlComboBox->currentIndex()].url);
        /// Guess mime type for url to open
        KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(url);
        const QString mimeTypeName = mimeType->name();
        /// Ask KDE subsystem to open url in viewer matching mime type
        KRun::runUrl(url, mimeTypeName, p, false, false);
    }

    UrlInfo urlMetaInfo(const KUrl &url) {
        UrlInfo result;
        result.url = url;

        if (!KBibTeX::isLocalOrRelative(url) && url.fileName().isEmpty()) {
            /// URLs not pointing to a specific file should be opened with a web browser component
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
            return result;
        }

        int accuracy = 0;
        KMimeType::Ptr mimeTypePtr = KMimeType::findByUrl(url, 0, KBibTeX::isLocalOrRelative(url), true, &accuracy);
        if (accuracy < 50) {
            mimeTypePtr = KMimeType::findByPath(url.fileName(), 0, true, &accuracy);
        }
        result.mimeType = mimeTypePtr->name();
        result.icon = KIcon(mimeTypePtr->iconName());

        if (result.mimeType == QLatin1String("application/octet-stream")) {
            /// application/octet-stream is a fall-back if KDE did not know better
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
        } else if ((result.mimeType.isEmpty() || result.mimeType == QLatin1String("inode/directory")) && (result.url.protocol() == QLatin1String("http") || result.url.protocol() == QLatin1String("https"))) {
            /// directory via http means normal webpage (not browsable directory)
            result.icon = KIcon("text-html");
            result.mimeType = QLatin1String("text/html");
        }

        if (url.pathOrUrl().startsWith(arXivPDFUrlStart)) {
            kDebug() << "URL looks like a PDF url from arXiv";
            result.icon = KIcon("application-pdf");
            result.mimeType = QLatin1String("application/pdf");
        }

        return result;
    }

    void comboBoxChanged(int index) {
        showUrl(cbxEntryToUrlInfo[index]);
    }

    bool isVisible() {
        /// get dock where this widget is inside
        /// static cast is save as constructor requires parent to be QDockWidget
        QDockWidget *pp = static_cast<QDockWidget *>(p->parent());
        return pp != NULL && !pp->isHidden();
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        onlyLocalFilesButton->setChecked(!configGroup.readEntry(onlyLocalFilesCheckConfig, true));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(onlyLocalFilesCheckConfig, !onlyLocalFilesButton->isChecked());
        config->sync();
    }
};

const QString DocumentPreview::DocumentPreviewPrivate::arXivPDFUrlStart = QLatin1String("http://arxiv.org/pdf/");
const QString DocumentPreview::DocumentPreviewPrivate::configGroupName = QLatin1String("URL Preview");
const QString DocumentPreview::DocumentPreviewPrivate::onlyLocalFilesCheckConfig = QLatin1String("OnlyLocalFiles");

DocumentPreview::DocumentPreview(QDockWidget *parent)
        : QWidget(parent), d(new DocumentPreviewPrivate(this))
{
    connect(parent, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
}

DocumentPreview::~DocumentPreview()
{
    delete d;
}

void DocumentPreview::setElement(QSharedPointer<Element> element, const File *)
{
    d->entry = element.dynamicCast<const Entry>();
    d->update();
}

void DocumentPreview::openExternally()
{
    d->openExternally();
}

void DocumentPreview::setBibTeXUrl(const KUrl &url)
{
    d->baseUrl = url;
}

void DocumentPreview::onlyLocalFilesChanged()
{
    d->saveState();
    d->update();
}

void DocumentPreview::visibilityChanged(bool)
{
    d->update();
}

void DocumentPreview::comboBoxChanged(int index)
{
    d->comboBoxChanged(index);
}

void DocumentPreview::statFinished(KJob *kjob)
{
    KIO::StatJob *job = static_cast<KIO::StatJob *>(kjob);
    d->runningJobs.removeOne(job);
    if (!job->error()) {
#if KDE_IS_VERSION(4, 4, 0)
        const KUrl url = job->mostLocalUrl();
#else // KDE_IS_VERSION
        const KUrl url = job->url();
#endif // KDE_IS_VERSION
        DocumentPreviewPrivate::UrlInfo urlInfo = d->urlMetaInfo(url);
        setCursor(d->runningJobs.isEmpty() ? Qt::ArrowCursor : Qt::BusyCursor);
        d->addUrl(urlInfo);
    } else {
        kDebug() << job->error() << job->errorString();
    }

    if (d->runningJobs.isEmpty()) {
        /// If this was the last background stat job ...
        setCursor(Qt::ArrowCursor);

        if (d->urlComboBox->count() < 1) {
            /// In case that no valid references were found by the stat jobs ...
            if (d->anyRemote && !d->onlyLocalFilesButton->isChecked()) {
                /// There are some remote URLs to probe,
                /// but user was only looking for local files
                d->showMessage(i18n("<qt>No documents to show.<br/><a href=\"disableonlylocalfiles\">Disable the restriction</a> to local files to see remote documents.</qt>"));
            } else {
                /// No stat job at all succeeded. Show message to user.
                d->showMessage(i18n("No documents to show.\nSome URLs or files could not be retrieved."));
            }
        }
    }
}

void DocumentPreview::loadingFinished()
{
    setCursor(Qt::ArrowCursor);
    d->showPart(dynamic_cast<KParts::ReadOnlyPart *>(sender()), dynamic_cast<QWidget *>(sender()));
}

void DocumentPreview::linkActivated(const QString &link)
{
    if (link == QLatin1String("disableonlylocalfiles"))
        d->onlyLocalFilesButton->setChecked(true);
    else if (link.startsWith(QLatin1String("http://")) || link.startsWith(QLatin1String("https://"))) {
        const KUrl urlToOpen = KUrl::fromUserInput(link);
        if (urlToOpen.isValid()) {
            /// Guess mime type for url to open
            KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(urlToOpen);
            const QString mimeTypeName = mimeType->name();
            /// Ask KDE subsystem to open url in viewer matching mime type
            KRun::runUrl(urlToOpen, mimeTypeName, this, false, false);
        }
    }
}
