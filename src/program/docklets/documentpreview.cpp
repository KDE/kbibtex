/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "documentpreview.h"

#include <typeinfo>

#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QLayout>
#include <QMap>
#include <QFileInfo>
#include <QResizeEvent>
#include <QCheckBox>
#include <QMenuBar>
#include <QStackedWidget>
#include <QDockWidget>
#include <QPushButton>
#include <QComboBox>
#include <QMutex>
#include <QMimeType>
#include <QIcon>
#ifdef HAVE_WEBENGINEWIDGETS
#include <QWebEngineView>
#endif // HAVE_WEBENGINEWIDGETS

#include <kio_version.h>
#include <kservice_version.h>
#include <KLocalizedString>
#include <KJobWidgets>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
#include <KIO/OpenUrlJob>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
#include <KIO/JobUiDelegateFactory>
#else // < 5.98.0
#include <KIO/JobUiDelegate>
#endif // QT_VERSION_CHECK(5, 98, 0)
#else // < 5.71.0
#include <KRun>
#endif // KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
#if KSERVICE_VERSION < 0x055200 // < 5.82.0
#include <KMimeTypeTrader>
#endif // KSERVICE_VERSION < 0x055200 // < 5.82.0
#include <KService>
#include <KParts/Part>
#if KSERVICE_VERSION >= 0x055200 // >= 5.82.0
#include <KParts/PartLoader>
#endif // KSERVICE_VERSION >= 0x055200 // >= 5.82.0
#include <KParts/ReadOnlyPart>
#include <kio/statjob.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KToolBar>
#include <KActionCollection>
#include <KSharedConfig>
#include <KConfigGroup>

#include <KBibTeX>
#include <Element>
#include <Entry>
#include <File>
#include <FileInfo>
#include "logging_program.h"

ImageLabel::ImageLabel(const QString &text, QWidget *parent)
        : QLabel(text, parent)
{
    /// nothing
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
        QUrl url;
        QString mimeType;
        QIcon icon;
    };

private:
    DocumentPreview *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString onlyLocalFilesCheckConfig;

    QPushButton *externalViewerButton;
    QStackedWidget *stackedWidget;
    ImageLabel *message;
    QMap<int, struct UrlInfo> cbxEntryToUrlInfo;
    QMutex addingUrlMutex;

    bool anyLocal;

    QMenuBar *menuBar;
    KToolBar *toolBar;
    KParts::ReadOnlyPart *okularPart;
#ifdef HAVE_WEBENGINEWIDGETS
    QWebEngineView *htmlWidget;
#else // HAVE_WEBENGINEWIDGETS
    KParts::ReadOnlyPart *htmlPart;
#endif // HAVE_WEBENGINEWIDGETS
    int swpMessage, swpOkular, swpHTML;

public:
    QComboBox *urlComboBox;
    QPushButton *onlyLocalFilesButton;
    QList<KIO::StatJob *> runningJobs;
    QSharedPointer<const Entry> entry;
    QUrl baseUrl;
    bool anyRemote;

    KParts::ReadOnlyPart *locatePart(const QString &mimeType, QWidget *parentWidget) {
#if KSERVICE_VERSION>= 0x055200 // >= 5.82.0
        const QVector<KPluginMetaData> parts {KParts::PartLoader::partsForMimeType(mimeType)};
        if (!parts.isEmpty()) {
            KPluginFactory::Result<KPluginFactory> pluginFactory{KPluginFactory::loadFactory(parts.first())};
#if KSERVICE_VERSION>= 0x055900 // >= 5.89.0
            KParts::ReadOnlyPart *part{pluginFactory.plugin->create<KParts::ReadOnlyPart>(parentWidget, p, QVariantList())};
#else // < 5.89.0
            KParts::ReadOnlyPart *part{pluginFactory.plugin->create<KParts::ReadOnlyPart>(parentWidget, p)};
#endif // >= 5.89.0
#else // < 5.82.0
        KService::Ptr service = KMimeTypeTrader::self()->preferredService(mimeType, QStringLiteral("KParts/ReadOnlyPart"));
        if (service) {
            KParts::ReadOnlyPart *part = service->createInstance<KParts::ReadOnlyPart>(parentWidget, p);
#endif // >= 5.82.0
            connect(part, static_cast<void(KParts::ReadOnlyPart::*)()>(&KParts::ReadOnlyPart::completed), p, &DocumentPreview::loadingFinished);
            return part;
        } else
            return nullptr;
    }

    DocumentPreviewPrivate(DocumentPreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), anyLocal(false), entry(nullptr), anyRemote(false) {
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
        layout->setContentsMargins(0, 0, 0, 0);

        /// some widgets on the top to control the view

        QHBoxLayout *innerLayout = new QHBoxLayout();
        layout->addLayout(innerLayout, 0);

        onlyLocalFilesButton = new QPushButton(QIcon::fromTheme(QStringLiteral("applications-internet")), QString(), p);
        onlyLocalFilesButton->setToolTip(i18n("Toggle between local files only and all documents including remote ones"));
        innerLayout->addWidget(onlyLocalFilesButton, 0);
        onlyLocalFilesButton->setCheckable(true);
        QSizePolicy sp = onlyLocalFilesButton->sizePolicy();
        sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
        onlyLocalFilesButton->setSizePolicy(sp);

        urlComboBox = new QComboBox(p);
        innerLayout->addWidget(urlComboBox, 1);

        externalViewerButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), QString(), p);
        externalViewerButton->setToolTip(i18n("Open in external program"));
        innerLayout->addWidget(externalViewerButton, 0);
        sp = externalViewerButton->sizePolicy();
        sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
        externalViewerButton->setSizePolicy(sp);

        menuBar = new QMenuBar(p);
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
        connect(message, &QLabel::linkActivated, p, &DocumentPreview::linkActivated);

        /// add parts to stackedWidget
        okularPart = locatePart(QStringLiteral("application/pdf"), stackedWidget);
        swpOkular = (okularPart == nullptr) ? -1 : stackedWidget->addWidget(okularPart->widget());
        if (okularPart == nullptr || swpOkular < 0) {
            qCWarning(LOG_KBIBTEX_PROGRAM) << "No 'KDE Frameworks 5'-based Okular part for PDF or PostScript document preview available.";
        }
#ifdef HAVE_WEBENGINEWIDGETS
        qCDebug(LOG_KBIBTEX_PROGRAM) << "WebEngine is available, using it instead of HTML KPart (neither considered nor tested for) for HTML/Web preview.";
        /// To make DrKonqi handle crashes in Chromium-based QtWebEngine,
        /// set a certain environment variable. For details, see here:
        /// https://www.dvratil.cz/2018/10/drkonqi-and-qtwebengine/
        /// https://phabricator.kde.org/D16004
        auto chromiumFlags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        if (!chromiumFlags.contains("disable-in-process-stack-traces")) {
            chromiumFlags.append(" --disable-in-process-stack-traces");
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags);
        }
        htmlWidget = new QWebEngineView(stackedWidget);
        swpHTML = stackedWidget->addWidget(htmlWidget);
        connect(htmlWidget, &QWebEngineView::loadFinished, p, &DocumentPreview::loadingFinished);
#else // HAVE_WEBENGINEWIDGETS
        htmlPart = locatePart(QStringLiteral("text/html"), stackedWidget);
        if (htmlPart != nullptr) {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "HTML KPart is available, using it instead of WebEngine (not available) for HTML/Web preview.";
            swpHTML = stackedWidget->addWidget(htmlPart->widget());
        } else {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "No HTML viewing component is available, disabling HTML/Web preview.";
            swpHTML = -1;
        }
#endif // HAVE_WEBENGINEWIDGETS

        loadState();

        connect(externalViewerButton, &QPushButton::clicked, p, [this]() {
            openExternally();
        });
        connect(urlComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), p, [this](int index) {
            comboBoxChanged(index);
        });
        connect(onlyLocalFilesButton, &QPushButton::toggled, p, &DocumentPreview::onlyLocalFilesChanged);
    }

    inline bool isLocalOrRelative(const QUrl &url) const
    {
        return url.isLocalFile() || url.isRelative() || url.scheme().isEmpty();
    }

    bool addUrl(const struct UrlInfo &urlInfo) {
        bool isLocal = isLocalOrRelative(urlInfo.url);
        anyLocal |= isLocal;

        if (!onlyLocalFilesButton->isChecked() && !isLocal) return true; ///< ignore URL if only local files are allowed

        if (isLocal) {
            /// create a drop-down list entry if file is a local file
            /// (based on patch by Luis Silva)
            QString fn = urlInfo.url.fileName();
            QString full = urlInfo.url.url(QUrl::PreferLocalFile);
            QString dir = urlInfo.url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();
            QString text = fn.isEmpty() ? full : (dir.isEmpty() ? fn : QString(QStringLiteral("%1 [%2]")).arg(fn, dir));
            urlComboBox->addItem(urlInfo.icon, text);
        } else {
            /// create a drop-down list entry if file is a remote file
            urlComboBox->addItem(urlInfo.icon, urlInfo.url.toDisplayString());
        }
        urlComboBox->setEnabled(true);
        cbxEntryToUrlInfo.insert(urlComboBox->count() - 1, urlInfo);

        externalViewerButton->setEnabled(true);
        if (urlComboBox->count() == 1 || ///< first entry in combobox
                isLocal || ///< local files always preferred over URLs
                /// prefer arXiv summary URLs over other URLs
                (!anyLocal && urlInfo.url.host().contains(QStringLiteral("arxiv.org/abs")))) {
            showUrl(urlInfo);
        }

        return true;
    }

    void update() {
        p->setCursor(Qt::WaitCursor);

        /// reset and clear all controls
        if (swpOkular >= 0 && okularPart != nullptr)
            okularPart->closeUrl();
#ifdef HAVE_WEBENGINEWIDGETS
        htmlWidget->stop();
#else // HAVE_WEBENGINEWIDGETS
        if (swpHTML >= 0 && htmlPart != nullptr)
            htmlPart->closeUrl();
#endif // HAVE_WEBENGINEWIDGETS
        urlComboBox->setEnabled(false);
        urlComboBox->clear();
        cbxEntryToUrlInfo.clear();
        externalViewerButton->setEnabled(false);
        showMessage(i18n("Refreshing...")); // krazy:exclude=qmethods

        /// cancel/kill all running jobs
        auto it = runningJobs.begin();
        while (it != runningJobs.end()) {
            (*it)->kill();
            it = runningJobs.erase(it);
        }

        /// clear flag that memorizes if any local file was referenced
        anyLocal = false;
        anyRemote = false;

        /// do not load external reference if widget is hidden
        if (isVisible()) {
            const auto urlList = FileInfo::entryUrls(entry, baseUrl, FileInfo::TestExistence::Yes);
            for (const QUrl &url : urlList) {
                bool isLocal = isLocalOrRelative(url);
                anyRemote |= !isLocal;
                if (!onlyLocalFilesButton->isChecked() && !isLocal) continue;

                KIO::StatJob *job = KIO::mostLocalUrl(url, KIO::HideProgressInfo);
                runningJobs << job;
                KJobWidgets::setWindow(job, p);
                connect(job, &KIO::StatJob::result, p, &DocumentPreview::statFinished);
            }
            if (urlList.isEmpty()) {
                /// Case no URLs associated with this entry.
                /// For-loop above was never executed.
                showMessage(i18n("No documents to show.")); // krazy:exclude=qmethods
                p->setCursor(Qt::ArrowCursor);
            } else if (runningJobs.isEmpty()) {
                /// Case no stat jobs are running. As there were URLs (tested in
                /// previous condition), this implies that there were remote
                /// references that were ignored by executing "continue" above.
                /// Give user hint that by enabling remote files, more can be shown.
                showMessage(i18n("<qt>No documents to show.<br/><a href=\"disableonlylocalfiles\">Disable the restriction</a> to local files to see remote documents.</qt>")); // krazy:exclude=qmethods
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
        if (swpHTML >= 0)
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

        QDomNodeList toolbarNodes = docElem.elementsByTagName(QStringLiteral("ToolBar"));
        for (int i = 0; i < toolbarNodes.count(); ++i) {
            QDomNodeList toolbarItems = toolbarNodes.at(i).childNodes();
            for (int j = 0; j < toolbarItems.count(); ++j) {
                QDomNode toolbarItem = toolbarItems.at(j);
                if (toolbarItem.nodeName() == QStringLiteral("Action")) {
                    QString actionName = toolbarItem.attributes().namedItem(QStringLiteral("name")).nodeValue();
                    toolBar->addAction(part->actionCollection()->action(actionName));
                } else if (toolbarItem.nodeName() == QStringLiteral("Separator")) {
                    toolBar->addSeparator();
                }
            }
        }


        QDomNodeList menubarNodes = docElem.elementsByTagName(QStringLiteral("MenuBar"));
        for (int i = 0; i < menubarNodes.count(); ++i) {
            QDomNodeList menubarNode = menubarNodes.at(i).childNodes();
            for (int j = 0; j < menubarNode.count(); ++j) {
                QDomNode menubarItem = menubarNode.at(j);
                if (menubarItem.nodeName() == QStringLiteral("Menu")) {
                    QDomNodeList menuNode = menubarItem.childNodes();
                    QString text;
                    for (int k = 0; k < menuNode.count(); ++k) {
                        QDomNode menuItem = menuNode.at(k);
                        if (menuItem.nodeName() == QStringLiteral("text")) {
                            text = menuItem.firstChild().toText().data();
                            break;
                        }
                    }
                    QMenu *menu = menuBar->addMenu(text);

                    for (int k = 0; k < menuNode.count(); ++k) {
                        QDomNode menuItem = menuNode.at(k);
                        if (menuItem.nodeName() == QStringLiteral("Action")) {
                            QString actionName = menuItem.attributes().namedItem(QStringLiteral("name")).nodeValue();
                            menu->addAction(part->actionCollection()->action(actionName));
                        } else if (menuItem.nodeName() == QStringLiteral("Separator")) {
                            menu->addSeparator();
                        }
                    }
                }
            }
        }

        QDomNodeList actionPropertiesList = docElem.elementsByTagName(QStringLiteral("ActionProperties"));
        for (int i = 0; i < actionPropertiesList.count(); ++i) {
            QDomNodeList actionProperties = actionPropertiesList.at(i).childNodes();
            for (int j = 0; j < actionProperties.count(); ++j) {
                QDomNode actionNode = actionProperties.at(j);
                if (actionNode.nodeName() == QStringLiteral("Action")) {
                    const QString actionName = actionNode.attributes().namedItem(QStringLiteral("name")).toAttr().nodeValue();
                    const QString actionShortcut = actionNode.attributes().namedItem(QStringLiteral("shortcut")).toAttr().value();
                    QAction *action = part->actionCollection()->action(actionName);
                    if (action != nullptr) {
                        action->setShortcut(QKeySequence(actionShortcut));
                    }
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

        if (okularPart != nullptr && part == okularPart && swpOkular >= 0) {
            stackedWidget->setCurrentIndex(swpOkular);
            stackedWidget->widget(swpOkular)->setEnabled(true);
            setupToolMenuBarForPart(okularPart);
#ifdef HAVE_WEBENGINEWIDGETS
        } else if (widget == htmlWidget) {
            stackedWidget->setCurrentIndex(swpHTML);
            stackedWidget->widget(swpHTML)->setEnabled(true);
#else // HAVE_WEBENGINEWIDGETS
        } else if (htmlPart != nullptr && part == htmlPart && swpHTML >= 0) {
            stackedWidget->setCurrentIndex(swpHTML);
            stackedWidget->widget(swpHTML)->setEnabled(true);
            setupToolMenuBarForPart(htmlPart);
#endif // HAVE_WEBENGINEWIDGETS
        } else if (widget == message) {
            stackedWidget->setCurrentIndex(swpMessage);
        } else
            showMessage(i18n("Cannot show requested part")); // krazy:exclude=qmethods
    }

    bool showUrl(const struct UrlInfo &urlInfo) {
        static const QStringList okularMimetypes {QStringLiteral("application/x-pdf"), QStringLiteral("application/pdf"), QStringLiteral("application/x-gzpdf"), QStringLiteral("application/x-bzpdf"), QStringLiteral("application/x-wwf"), QStringLiteral("image/vnd.djvu"), QStringLiteral("image/vnd.djvu+multipage"), QStringLiteral("application/postscript"), QStringLiteral("image/x-eps"), QStringLiteral("application/x-gzpostscript"), QStringLiteral("application/x-bzpostscript"), QStringLiteral("image/x-gzeps"), QStringLiteral("image/x-bzeps")};
        static const QStringList htmlMimetypes {QStringLiteral("text/html"), QStringLiteral("application/xml"), QStringLiteral("application/xhtml+xml")};
        static const QStringList imageMimetypes {QStringLiteral("image/jpeg"), QStringLiteral("image/png"), QStringLiteral("image/gif"), QStringLiteral("image/tiff")};

        if (swpHTML >= 0)
            stackedWidget->widget(swpHTML)->setEnabled(false);
        if (swpOkular >= 0 && okularPart != nullptr) {
            stackedWidget->widget(swpOkular)->setEnabled(false);
            okularPart->closeUrl();
        }
#ifdef HAVE_WEBENGINEWIDGETS
        htmlWidget->stop();
#else // HAVE_WEBENGINEWIDGETS
        if (swpHTML >= 0 && htmlPart != nullptr)
            htmlPart->closeUrl();
#endif // HAVE_WEBENGINEWIDGETS

        if (swpOkular >= 0 && okularPart != nullptr && okularMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            showMessage(i18n("Loading...")); // krazy:exclude=qmethods
            return okularPart->openUrl(urlInfo.url);
        } else if (htmlMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            showMessage(i18n("Loading...")); // krazy:exclude=qmethods
#ifdef HAVE_WEBENGINEWIDGETS
            htmlWidget->load(urlInfo.url);
            return true;
#else // HAVE_WEBENGINEWIDGETS
            return (swpHTML >= 0 && htmlPart != nullptr) ? htmlPart->openUrl(urlInfo.url) : false;
#endif // HAVE_WEBENGINEWIDGETS
        } else if (imageMimetypes.contains(urlInfo.mimeType)) {
            p->setCursor(Qt::BusyCursor);
            message->setPixmap(QPixmap(urlInfo.url.url(QUrl::PreferLocalFile)));
            showPart(nullptr, message);
            p->unsetCursor();
            return true;
        } else {
            QString additionalInformation;
            if (urlInfo.mimeType == QStringLiteral("application/pdf"))
                additionalInformation = i18nc("Additional information in case there is not KPart available for mime type 'application/pdf'", "<br/><br/>Please install <a href=\"https://userbase.kde.org/Okular\">Okular</a> for KDE Frameworks&nbsp;5 to make use of its PDF viewing component.<br/>Okular for KDE&nbsp;4 will not work.");
            showMessage(i18nc("First parameter is mime type, second parameter is optional information (may be empty)", "<qt>Don't know how to show mimetype '%1'.%2</qt>", urlInfo.mimeType, additionalInformation)); // krazy:exclude=qmethods
        }

        return false;
    }

    void openExternally() {
        QUrl url(cbxEntryToUrlInfo[urlComboBox->currentIndex()].url);
        /// Guess mime type for url to open
        QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
        const QString mimeTypeName = mimeType.name();
        /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
        KRun::runUrl(url, mimeTypeName, p, KRun::RunFlags());
#else // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0) // >= 5.71.0
        KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url, mimeTypeName);
#if KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // < 5.98.0
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, p));
#else // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // >= 5.98.0
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, p));
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0)
        job->start();
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
    }

    UrlInfo urlMetaInfo(const QUrl &url) {
        UrlInfo result;
        result.url = url;

        if (!isLocalOrRelative(url) && url.fileName().isEmpty()) {
            /// URLs not pointing to a specific file should be opened with a web browser component
            result.icon = QIcon::fromTheme(QStringLiteral("text-html"));
            result.mimeType = QStringLiteral("text/html");
            return result;
        }

        /// Generic guess for URL's MIME type
        const QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
        result.mimeType = mimeType.name();

        /// Guesses made for local file systems may not be applicable for URLs to web pages
        if (result.mimeType == QStringLiteral("application/octet-stream")) {
            /// application/octet-stream is a fall-back if KDE did not know better
            result.mimeType = QStringLiteral("text/html");
        } else if ((result.mimeType.isEmpty() || result.mimeType == QStringLiteral("inode/directory")) && (result.url.scheme() == QStringLiteral("http") || result.url.scheme() == QStringLiteral("https"))) {
            /// directory via http means normal webpage (not browsable directory)
            result.mimeType = QStringLiteral("text/html");
        }

        /// Make more specific guesses on an URL's MIME type
        if (QRegularExpression(QStringLiteral("^http[s]?://arxiv.org/pdf/")).match(url.url(QUrl::PreferLocalFile)).hasMatch()) {
            result.mimeType = QStringLiteral("application/pdf");
        }

        result.icon = QIcon::fromTheme(mimeType.iconName());
        return result;
    }

    void comboBoxChanged(int index) {
        showUrl(cbxEntryToUrlInfo[index]);
    }

    bool isVisible() {
        /// get dock where this widget is inside
        /// static cast is save as constructor requires parent to be QDockWidget
        QDockWidget *pp = static_cast<QDockWidget *>(p->parent());
        return pp != nullptr && !pp->isHidden();
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

const QString DocumentPreview::DocumentPreviewPrivate::configGroupName = QStringLiteral("URL Preview");
const QString DocumentPreview::DocumentPreviewPrivate::onlyLocalFilesCheckConfig = QStringLiteral("OnlyLocalFiles");

DocumentPreview::DocumentPreview(QDockWidget *parent)
        : QWidget(parent), d(new DocumentPreviewPrivate(this))
{
    connect(parent, &QDockWidget::visibilityChanged, this, [this]() {
        d->update();
    });
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

void DocumentPreview::setBibTeXUrl(const QUrl &url)
{
    d->baseUrl = url;
}

void DocumentPreview::onlyLocalFilesChanged()
{
    d->saveState();
    d->update();
}

void DocumentPreview::statFinished(KJob *kjob)
{
    KIO::StatJob *job = static_cast<KIO::StatJob *>(kjob);
    d->runningJobs.removeOne(job);
    if (!job->error()) {
        const QUrl url = job->mostLocalUrl();
        DocumentPreviewPrivate::UrlInfo urlInfo = d->urlMetaInfo(url);
        setCursor(d->runningJobs.isEmpty() ? Qt::ArrowCursor : Qt::BusyCursor);
        d->addUrl(urlInfo);
    } else {
        qCWarning(LOG_KBIBTEX_PROGRAM) << job->error() << job->errorString();
    }

    if (d->runningJobs.isEmpty()) {
        /// If this was the last background stat job ...
        setCursor(Qt::ArrowCursor);

        if (d->urlComboBox->count() < 1) {
            /// In case that no valid references were found by the stat jobs ...
            if (d->anyRemote && !d->onlyLocalFilesButton->isChecked()) {
                /// There are some remote URLs to probe,
                /// but user was only looking for local files
                d->showMessage(i18n("<qt>No documents to show.<br/><a href=\"disableonlylocalfiles\">Disable the restriction</a> to local files to see remote documents.</qt>")); // krazy:exclude=qmethods
            } else {
                /// No stat job at all succeeded. Show message to user.
                d->showMessage(i18n("No documents to show.\nSome URLs or files could not be retrieved.")); // krazy:exclude=qmethods
            }
        }
    }
}

void DocumentPreview::loadingFinished()
{
    setCursor(Qt::ArrowCursor);
    d->showPart(qobject_cast<KParts::ReadOnlyPart *>(sender()), qobject_cast<QWidget *>(sender()));
}

void DocumentPreview::linkActivated(const QString &link)
{
    if (link == QStringLiteral("disableonlylocalfiles"))
        d->onlyLocalFilesButton->setChecked(true);
    else if (link.startsWith(QStringLiteral("http://")) || link.startsWith(QStringLiteral("https://"))) {
        const QUrl urlToOpen = QUrl::fromUserInput(link);
        if (urlToOpen.isValid()) {
            /// Guess mime type for url to open
            QMimeType mimeType = FileInfo::mimeTypeForUrl(urlToOpen);
            const QString mimeTypeName = mimeType.name();
            /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
            KRun::runUrl(urlToOpen, mimeTypeName, this, KRun::RunFlags());
#else // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0) // >= 5.71.0
            KIO::OpenUrlJob *job = new KIO::OpenUrlJob(urlToOpen, mimeTypeName);
#if KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // < 5.98.0
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
#else // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // >= 5.98.0
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0)
            job->start();
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
        }
    }
}
