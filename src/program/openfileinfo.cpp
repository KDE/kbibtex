/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "openfileinfo.h"

#include <QString>
#include <QTimer>
#include <QFileInfo>
#include <QWidget>
#include <QUrl>
#include <QApplication>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KParts/Part>
#include <KParts/ReadOnlyPart>
#include <KParts/ReadWritePart>
#include <KParts/PartLoader>
#include <kparts_version.h>

#ifdef HAVE_POPPLERQT
#include <FileImporterPDF>
#endif // HAVE_POPPLERQT
#include "logging_program.h"

#if KPARTS_VERSION < QT_VERSION_CHECK(5, 100, 0)
// Copied from kparts/partloader.h
namespace KParts::PartLoader {
template<typename T>
static KPluginFactory::Result<T>
instantiatePart(const KPluginMetaData &data, QWidget *parentWidget = nullptr, QObject *parent = nullptr, const QVariantList &args = {})
{
    KPluginFactory::Result<T> result;
    KPluginFactory::Result<KPluginFactory> factoryResult = KPluginFactory::loadFactory(data);
    if (!factoryResult.plugin) {
        result.errorString = factoryResult.errorString;
        result.errorReason = factoryResult.errorReason;
        return result;
    }
    T *instance = factoryResult.plugin->create<T>(parentWidget, parent, args);
    if (!instance) {
        const QString fileName = data.fileName();
        result.errorString = QObject::tr("KPluginFactory could not load the plugin: %1").arg(fileName);
        result.errorText = QStringLiteral("KPluginFactory could not load the plugin: %1").arg(fileName);
        result.errorReason = KPluginFactory::INVALID_KPLUGINFACTORY_INSTANTIATION;
    } else {
        result.plugin = instance;
    }
    return result;
}
}
#endif

class OpenFileInfo::OpenFileInfoPrivate
{
private:
    static int globalCounter;
    int m_counter;

public:
    static const QString keyLastAccess;
    static const QString keyURL;
    static const QString dateTimeFormat;

    OpenFileInfo *p;

    KParts::ReadOnlyPart *part;
    KPluginMetaData currentPart;
    QWidget *internalWidgetParent;
    QDateTime lastAccessDateTime;
    StatusFlags flags;
    OpenFileInfoManager *openFileInfoManager;
    QString mimeType;
    QUrl url;

    OpenFileInfoPrivate(OpenFileInfoManager *openFileInfoManager, const QUrl &url, const QString &mimeType, OpenFileInfo *p)
        :  m_counter(-1), p(p), part(nullptr), internalWidgetParent(nullptr) {
        this->openFileInfoManager = openFileInfoManager;
        this->url = url;
        if (this->url.isValid() && this->url.scheme().isEmpty())
            qCWarning(LOG_KBIBTEX_PROGRAM) << "No scheme specified for URL" << this->url.toDisplayString();
        this->mimeType = mimeType;
    }

    ~OpenFileInfoPrivate() {
        if (part != nullptr) {
            KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(part);
            if (rwp != nullptr)
                rwp->closeUrl(true);
            delete part;
        }
    }

    KParts::ReadOnlyPart *createPart(QWidget *newWidgetParent, const KPluginMetaData &_partMetaData = {}) {

        KPluginMetaData partMetaData = _partMetaData;

        if (!p->flags().testFlag(OpenFileInfo::StatusFlag::Open)) {
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Cannot create part for a file which is not open";
            return nullptr;
        }

        Q_ASSERT_X(internalWidgetParent == nullptr || internalWidgetParent == newWidgetParent, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, const KPluginMetaData & = {})", "internal widget should be either NULL or the same one as supplied as \"newWidgetParent\"");

        /** use cached part for this parent if possible */
        if (internalWidgetParent == newWidgetParent && (!partMetaData.isValid() || currentPart == partMetaData)) {
            Q_ASSERT_X(part != nullptr, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, const KPluginMetaData & = {})", "Part is NULL");
            return part;
        } else if (part != nullptr) {
            KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(part);
            if (rwp != nullptr)
                rwp->closeUrl(true);
            part->deleteLater();
            part = nullptr;
        }

        /// reset to invalid values in case something goes wrong
        currentPart = {};
        internalWidgetParent = nullptr;

        if (!partMetaData.isValid()) {
            /// no valid KPartMetaData has been passed
            /// try to find a read-write part to open file
            partMetaData = p->defaultService();
        }
        if (!partMetaData.isValid()) {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "PATH=" << getenv("PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "LD_LIBRARY_PATH=" << getenv("LD_LIBRARY_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "XDG_DATA_DIRS=" << getenv("XDG_DATA_DIRS");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "QT_PLUGIN_PATH=" << getenv("QT_PLUGIN_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "KDEDIRS=" << getenv("KDEDIRS");
            qCCritical(LOG_KBIBTEX_PROGRAM) << "Cannot find part to handle mimetype " << mimeType;
            return nullptr;
        }

        QString errorString;

        const auto loadResult = KParts::PartLoader::instantiatePart<KParts::ReadWritePart>(partMetaData, newWidgetParent, qobject_cast<QObject *>(newWidgetParent));

        errorString = loadResult.errorString;
        part = loadResult.plugin;

        if (part == nullptr) {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "name:" << partMetaData.name();
            qCDebug(LOG_KBIBTEX_PROGRAM) << "version:" << partMetaData.version();
            qCDebug(LOG_KBIBTEX_PROGRAM) << "category:" << partMetaData.category();
            qCDebug(LOG_KBIBTEX_PROGRAM) << "description:" << partMetaData.description();
            qCDebug(LOG_KBIBTEX_PROGRAM) << "fileName:" << partMetaData.fileName();
            qCDebug(LOG_KBIBTEX_PROGRAM) << "PATH=" << getenv("PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "LD_LIBRARY_PATH=" << getenv("LD_LIBRARY_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "XDG_DATA_DIRS=" << getenv("XDG_DATA_DIRS");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "QT_PLUGIN_PATH=" << getenv("QT_PLUGIN_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "KDEDIRS=" << getenv("KDEDIRS");
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Could not instantiate read-write part for" << partMetaData.name() << "(mimeType=" << mimeType << ", library=" << partMetaData.fileName() << ", error msg=" << errorString << ")";
            /// creating a read-write part failed, so maybe it is read-only (like Okular's PDF viewer)?
            const auto loadResult = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(partMetaData, newWidgetParent, qobject_cast<QObject *>(newWidgetParent));
            errorString = loadResult.errorString;
            part = loadResult.plugin;
        }
        if (part == nullptr) {
            /// still cannot create part, must be error
            qCCritical(LOG_KBIBTEX_PROGRAM) << "Could not instantiate part for" << partMetaData.name() << "(mimeType=" << mimeType << ", library=" << partMetaData.fileName() << ", error msg=" << errorString << ")";
            return nullptr;
        }

        if (url.isValid()) {
            /// open URL in part
            part->openUrl(url);
            /// update document list widget accordingly
            p->addFlags(OpenFileInfo::StatusFlag::RecentlyUsed);
            p->addFlags(OpenFileInfo::StatusFlag::HasName);
        } else {
            /// initialize part with empty document
            part->openUrl(QUrl());
        }
        p->addFlags(OpenFileInfo::StatusFlag::Open);

        currentPart = partMetaData;
        internalWidgetParent = newWidgetParent;

        Q_ASSERT_X(part != nullptr, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, const KPluginMetaData & = {})", "Creation of part failed, is NULL"); /// test should not be necessary, but just to be save ...
        return part;
    }

    int counter() {
        if (!url.isValid() && m_counter < 0)
            m_counter = ++globalCounter;
        else if (url.isValid())
            qCWarning(LOG_KBIBTEX_PROGRAM) << "This function should not be called if URL is valid";
        return m_counter;
    }

};

int OpenFileInfo::OpenFileInfoPrivate::globalCounter = 0;
const QString OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat = QStringLiteral("yyyy-MM-dd-hh-mm-ss-zzz");
const QString OpenFileInfo::OpenFileInfoPrivate::keyLastAccess = QStringLiteral("LastAccess");
const QString OpenFileInfo::OpenFileInfoPrivate::keyURL = QStringLiteral("URL");

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QUrl &url)
        : d(new OpenFileInfoPrivate(openFileInfoManager, url, FileInfo::mimeTypeForUrl(url).name(), this))
{
    /// nothing
}

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType)
        : d(new OpenFileInfoPrivate(openFileInfoManager, QUrl(), mimeType, this))
{
    /// nothing
}

OpenFileInfo::~OpenFileInfo()
{
    delete d;
}

void OpenFileInfo::setUrl(const QUrl &url)
{
    Q_ASSERT_X(url.isValid(), "void OpenFileInfo::setUrl(const QUrl&)", "URL is not valid");
    d->url = url;
    if (d->url.scheme().isEmpty())
        qCWarning(LOG_KBIBTEX_PROGRAM) << "No scheme specified for URL" << d->url.toDisplayString();
    d->mimeType = FileInfo::mimeTypeForUrl(url).name();
    addFlags(OpenFileInfo::StatusFlag::HasName);
}

QUrl OpenFileInfo::url() const
{
    return d->url;
}

bool OpenFileInfo::isModified() const
{
    KParts::ReadWritePart *rwPart = qobject_cast< KParts::ReadWritePart *>(d->part);
    if (rwPart == nullptr)
        return false;
    else
        return rwPart->isModified();
}

bool OpenFileInfo::save()
{
    KParts::ReadWritePart *rwPart = qobject_cast< KParts::ReadWritePart *>(d->part);
    if (rwPart == nullptr)
        return true;
    else
        return rwPart->save();
}

bool OpenFileInfo::close()
{
    if (d->part == nullptr) {
        /// if there is no part, closing always "succeeds"
        return true;
    }

    KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(d->part);
    if (rwp == nullptr || rwp->closeUrl(true)) {
        d->part->deleteLater();
        d->part = nullptr;
        d->internalWidgetParent = nullptr;
        return true;
    }
    return false;
}

QString OpenFileInfo::mimeType() const
{
    return d->mimeType;
}

QString OpenFileInfo::shortCaption() const
{
    if (d->url.isValid())
        return d->url.fileName();
    else
        return i18n("Unnamed-%1", d->counter());
}

QString OpenFileInfo::fullCaption() const
{
    if (d->url.isValid())
        return d->url.url(QUrl::PreferLocalFile);
    else
        return shortCaption();
}

KParts::ReadOnlyPart *OpenFileInfo::part(QWidget *parent, const KPluginMetaData &service)
{
    return d->createPart(parent, service);
}

OpenFileInfo::StatusFlags OpenFileInfo::flags() const
{
    return d->flags;
}

void OpenFileInfo::setFlags(StatusFlags statusFlags)
{
    /// disallow files without name or valid url to become favorites
    if (!d->url.isValid() || !d->flags.testFlag(StatusFlag::HasName)) statusFlags &= ~static_cast<int>(StatusFlag::Favorite);
    /// files that got opened are by definition recently used files
    if (!d->url.isValid() && d->flags.testFlag(StatusFlag::Open)) statusFlags &= StatusFlag::RecentlyUsed;

    bool hasChanged = d->flags != statusFlags;
    d->flags = statusFlags;
    if (hasChanged)
        Q_EMIT flagsChanged(statusFlags);
}

void OpenFileInfo::addFlags(StatusFlags statusFlags)
{
    /// disallow files without name or valid url to become favorites
    if (!d->url.isValid() || !d->flags.testFlag(StatusFlag::HasName)) statusFlags &= ~static_cast<int>(StatusFlag::Favorite);

    bool hasChanged = (~d->flags & statusFlags) > 0;
    d->flags |= statusFlags;
    if (hasChanged)
        Q_EMIT flagsChanged(statusFlags);
}

void OpenFileInfo::removeFlags(StatusFlags statusFlags)
{
    bool hasChanged = (d->flags & statusFlags) > 0;
    d->flags &= ~statusFlags;
    if (hasChanged)
        Q_EMIT flagsChanged(statusFlags);
}

QDateTime OpenFileInfo::lastAccess() const
{
    return d->lastAccessDateTime;
}

void OpenFileInfo::setLastAccess(const QDateTime &dateTime)
{
    d->lastAccessDateTime = dateTime;
    Q_EMIT flagsChanged(OpenFileInfo::StatusFlag::RecentlyUsed);
}

QVector<KPluginMetaData> OpenFileInfo::listOfServices()
{
    const QString mt = mimeType();

    QVector<KPluginMetaData> result = KParts::PartLoader::partsForMimeType(mt);

    // Always include KBibTeX's KPart in list of services:
    // First, check if KBibTeX's KPart is already in list
    auto it = std::find_if(result.cbegin(), result.cend(), [](const KPluginMetaData &md){
        return md.pluginId() == QStringLiteral("kbibtexpart");
    });
    // If not, insert it
    if (it == result.cend()) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        auto kbibtexpart {KPluginMetaData(QStringLiteral("kbibtexpart"))};
#else // (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        auto kbibtexpart {KPluginMetaData::findPluginById(QStringLiteral("kf6/parts"), QStringLiteral("kbibtexpart"))};
#endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        if (kbibtexpart.isValid())
            result << kbibtexpart;
    }

    return result;
}

KPluginMetaData OpenFileInfo::defaultService()
{
    const QString mt = mimeType();

    KPluginMetaData result;
    static const QSet<QString> supportedMimeTypes{
        QStringLiteral("text/x-bibtex"),
#ifdef HAVE_POPPLERQT5
        QStringLiteral("application/pdf"),
#endif // HAVE_POPPLERQT5
        QStringLiteral("application/x-endnote-refer"), QStringLiteral("application/x-isi-export-format"), QStringLiteral("text/x-research-info-systems"),
        QStringLiteral("application/xml")
    };
    if (supportedMimeTypes.contains(mt)) {
        // If the mime type is natively supported by KBibTeX's part, enforce using this part
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        result = KPluginMetaData(QStringLiteral("kbibtexpart"));
#else // (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        result = KPluginMetaData::findPluginById(QStringLiteral("kf6/parts"), QStringLiteral("kbibtexpart"));
#endif // (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    }

    if (!result.isValid()) {
        // If above test for supported mime types failed
        // or no valid KPluginMetaData could be instanciated for 'kbibtexpart',
        // pick any of the other available parts supporting the requested mime type

        const QVector<KPluginMetaData> parts = listOfServices();
        if (!parts.isEmpty())
            result = parts.first();
    }

    if (result.isValid())
        qCDebug(LOG_KBIBTEX_PROGRAM) << "Using" << result.name() << "(" << result.description() << ") for mime type" << mt << "through library" << result.fileName();
    else
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Could not find part for mime type" << mt;
    return result;
}

KPluginMetaData OpenFileInfo::currentService()
{
    return d->currentPart;
}

class OpenFileInfoManager::OpenFileInfoManagerPrivate
{
private:
    static const QString configGroupNameRecentlyUsed;
    static const QString configGroupNameFavorites;
    static const QString configGroupNameOpen;
    static const int maxNumRecentlyUsedFiles, maxNumFavoriteFiles, maxNumOpenFiles;

public:
    OpenFileInfoManager *p;

    OpenFileInfoManager::OpenFileInfoList openFileInfoList;
    OpenFileInfo *currentFileInfo;

    OpenFileInfoManagerPrivate(OpenFileInfoManager *parent)
            : p(parent), currentFileInfo(nullptr) {
        /// nothing
    }

    ~OpenFileInfoManagerPrivate() {
        for (OpenFileInfoManager::OpenFileInfoList::Iterator it = openFileInfoList.begin(); it != openFileInfoList.end();) {
            OpenFileInfo *ofi = *it;
            delete ofi;
            it = openFileInfoList.erase(it);
        }
    }

    static bool byNameLessThan(const OpenFileInfo *left, const OpenFileInfo *right) {
        return left->shortCaption() < right->shortCaption();
    }

    static  bool byLRULessThan(const OpenFileInfo *left, const OpenFileInfo *right) {
        return left->lastAccess() > right->lastAccess(); /// reverse sorting!
    }

    void readConfig(QUrl &recentlyOpenURL) {
        readConfig(OpenFileInfo::StatusFlag::RecentlyUsed, configGroupNameRecentlyUsed, maxNumRecentlyUsedFiles);
        readConfig(OpenFileInfo::StatusFlag::Favorite, configGroupNameFavorites, maxNumFavoriteFiles);
        readConfig(OpenFileInfo::StatusFlag::Open, configGroupNameOpen, maxNumOpenFiles);

        KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
        KConfigGroup cg(config, configGroupNameOpen);
        static const QString recentlyOpenURLkey = OpenFileInfo::OpenFileInfoPrivate::keyURL + QStringLiteral("-current");
        recentlyOpenURL = cg.hasKey(recentlyOpenURLkey) ? QUrl(cg.readEntry(recentlyOpenURLkey)) : QUrl();
    }

    void writeConfig() {
        writeConfig(OpenFileInfo::StatusFlag::RecentlyUsed, configGroupNameRecentlyUsed, maxNumRecentlyUsedFiles);
        writeConfig(OpenFileInfo::StatusFlag::Favorite, configGroupNameFavorites, maxNumFavoriteFiles);
        writeConfig(OpenFileInfo::StatusFlag::Open, configGroupNameOpen, maxNumOpenFiles);

        KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
        KConfigGroup cg(config, configGroupNameOpen);
        static const QString recentlyOpenURLkey = OpenFileInfo::OpenFileInfoPrivate::keyURL + QStringLiteral("-current");
        if (currentFileInfo != nullptr && currentFileInfo->url().isValid())
            /// If there is a currently active and open file, remember that file's URL
            cg.writeEntry(recentlyOpenURLkey, currentFileInfo->url().url(QUrl::PreferLocalFile));
        else
            cg.deleteEntry(recentlyOpenURLkey);
    }

    void readConfig(OpenFileInfo::StatusFlag statusFlag, const QString &configGroupName, int maxNumFiles) {
        KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));

        KConfigGroup cg(config, configGroupName);
        for (int i = 0; i < maxNumFiles; ++i) {
            QUrl fileUrl = QUrl(cg.readEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ""));
            if (!fileUrl.isValid()) break;
            if (fileUrl.scheme().isEmpty())
                fileUrl.setScheme(QStringLiteral("file"));

            /// For local files, test if they exist; ignore local files that do not exist
            if (fileUrl.isLocalFile()) {
                if (!QFileInfo::exists(fileUrl.toLocalFile()))
                    continue;
            }

            OpenFileInfo *ofi = p->contains(fileUrl);
            if (ofi == nullptr) {
                ofi = p->open(fileUrl);
            }
            ofi->addFlags(statusFlag);
            ofi->addFlags(OpenFileInfo::StatusFlag::HasName);
            ofi->setLastAccess(QDateTime::fromString(cg.readEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ""), OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
        }
    }

    void writeConfig(OpenFileInfo::StatusFlag statusFlag, const QString &configGroupName, int maxNumFiles) {
        KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
        KConfigGroup cg(config, configGroupName);
        OpenFileInfoManager::OpenFileInfoList list = p->filteredItems(statusFlag);

        int i = 0;
        for (OpenFileInfoManager::OpenFileInfoList::ConstIterator it = list.constBegin(); i < maxNumFiles && it != list.constEnd(); ++it, ++i) {
            OpenFileInfo *ofi = *it;

            cg.writeEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ofi->url().url(QUrl::PreferLocalFile));
            cg.writeEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ofi->lastAccess().toString(OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
        }
        for (; i < maxNumFiles; ++i) {
            cg.deleteEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i));
            cg.deleteEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i));
        }
        config->sync();
    }
};

const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameRecentlyUsed = QStringLiteral("DocumentList-RecentlyUsed");
const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameFavorites = QStringLiteral("DocumentList-Favorites");
const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameOpen = QStringLiteral("DocumentList-Open");
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumFavoriteFiles = 256;
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumRecentlyUsedFiles = 8;
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumOpenFiles = 16;

OpenFileInfoManager::OpenFileInfoManager(QObject *parent)
        : QObject(parent), d(new OpenFileInfoManagerPrivate(this))
{
    QUrl recentlyOpenURL;
    d->readConfig(recentlyOpenURL);

    QTimer::singleShot(250, this, [this, recentlyOpenURL]() {
        /// In case there is at least one file marked as 'open' but it is not yet actually open,
        /// set it as current file now. The file to be opened (identified by URL) should be
        /// preferably the file that was actively open at the end of last KBibTeX session.
        /// Slightly delaying the actually opening of files is necessary to give precendence
        /// to bibliography files passed as command line arguments (see program.cpp) over files
        /// that where open when the previous KBibTeX session was quit.
        /// See KDE bug 417164.
        /// TODO still necessary to refactor the whole 'file opening' control flow to avoid hacks like this one
        if (d->currentFileInfo == nullptr) {
            OpenFileInfo *ofiToUse = nullptr;
            /// Go over the list of files that are flagged as 'open' ...
            for (auto *ofi : const_cast<const OpenFileInfoManager::OpenFileInfoList &>(d->openFileInfoList))
                if (ofi->flags().testFlag(OpenFileInfo::StatusFlag::Open) && (ofiToUse == nullptr || (recentlyOpenURL.isValid() && ofi->url() == recentlyOpenURL)))
                    ofiToUse = ofi;
            if (ofiToUse != nullptr)
                setCurrentFile(ofiToUse);
        }
    });
}

OpenFileInfoManager &OpenFileInfoManager::instance() {
    /// Allocate this singleton on heap not stack like most other singletons
    /// Supposedly, QCoreApplication will clean this singleton at application's end
    static OpenFileInfoManager *singleton = new OpenFileInfoManager(QCoreApplication::instance());
    return *singleton;
}

OpenFileInfoManager::~OpenFileInfoManager()
{
    delete d;
}

OpenFileInfo *OpenFileInfoManager::createNew(const QString &mimeType)
{
    OpenFileInfo *result = new OpenFileInfo(this, mimeType);
    connect(result, &OpenFileInfo::flagsChanged, this, &OpenFileInfoManager::flagsChanged);
    d->openFileInfoList << result;
    result->setLastAccess();
    return result;
}

OpenFileInfo *OpenFileInfoManager::open(const QUrl &url)
{
    Q_ASSERT_X(url.isValid(), "OpenFileInfo *OpenFileInfoManager::open(const QUrl&)", "URL is not valid");

    OpenFileInfo *result = contains(url);
    if (result == nullptr) {
        /// file not yet open
        result = new OpenFileInfo(this, url);
        connect(result, &OpenFileInfo::flagsChanged, this, &OpenFileInfoManager::flagsChanged);
        d->openFileInfoList << result;
    } /// else: file was already open, re-use and return existing OpenFileInfo pointer
    result->setLastAccess();
    return result;
}

OpenFileInfo *OpenFileInfoManager::contains(const QUrl &url) const
{
    if (!url.isValid()) return nullptr; /// can only be unnamed file

    for (auto *ofi : const_cast<const OpenFileInfoManager::OpenFileInfoList &>(d->openFileInfoList)) {
        if (ofi->url() == url)
            return ofi;
    }
    return nullptr;
}

bool OpenFileInfoManager::changeUrl(OpenFileInfo *openFileInfo, const QUrl &url)
{
    OpenFileInfo *previouslyContained = contains(url);

    /// check if old url differs from new url and old url is valid
    if (previouslyContained != nullptr && previouslyContained->flags().testFlag(OpenFileInfo::StatusFlag::Open) && previouslyContained != openFileInfo) {
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Open file with same URL already exists, forcefully closing it";
        close(previouslyContained);
    }

    QUrl oldUrl = openFileInfo->url();
    openFileInfo->setUrl(url);

    if (url != oldUrl && oldUrl.isValid()) {
        /// current document was most probabily renamed (e.g. due to "Save As")
        /// add old URL to recently used files, but exclude the open files list
        OpenFileInfo *ofi = open(oldUrl); // krazy:exclude=syscalls
        OpenFileInfo::StatusFlags statusFlags = (openFileInfo->flags() & ~static_cast<int>(OpenFileInfo::StatusFlag::Open)) | OpenFileInfo::StatusFlag::RecentlyUsed;
        ofi->setFlags(statusFlags);
    }
    if (previouslyContained != nullptr) {
        /// keep Favorite flag if set in file that have previously same URL
        if (previouslyContained->flags().testFlag(OpenFileInfo::StatusFlag::Favorite))
            openFileInfo->setFlags(openFileInfo->flags() | OpenFileInfo::StatusFlag::Favorite);

        /// remove the old entry with the same url has it will be replaced by the new one
        d->openFileInfoList.remove(d->openFileInfoList.indexOf(previouslyContained));
        previouslyContained->deleteLater();
        OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::StatusFlag::Open;
        statusFlags |= OpenFileInfo::StatusFlag::RecentlyUsed;
        statusFlags |= OpenFileInfo::StatusFlag::Favorite;
        Q_EMIT flagsChanged(statusFlags);
    }

    if (openFileInfo == d->currentFileInfo)
        Q_EMIT currentChanged(openFileInfo, {});
    Q_EMIT flagsChanged(openFileInfo->flags());

    return true;
}

bool OpenFileInfoManager::close(OpenFileInfo *openFileInfo)
{
    if (openFileInfo == nullptr) {
        qCWarning(LOG_KBIBTEX_PROGRAM) << "void OpenFileInfoManager::close(OpenFileInfo *openFileInfo): Cannot close openFileInfo which is NULL";
        return false;
    }

    bool isClosing = false;
    openFileInfo->setLastAccess();

    /// remove flag "open" from file to be closed and determine which file to show instead
    OpenFileInfo *nextCurrent = (d->currentFileInfo == openFileInfo) ? nullptr : d->currentFileInfo;
    for (OpenFileInfo *ofi : const_cast<const OpenFileInfoManager::OpenFileInfoList &>(d->openFileInfoList)) {
        if (!isClosing && ofi == openFileInfo && openFileInfo->close()) {
            isClosing = true;
            /// Mark file as closed (i.e. not open)
            openFileInfo->removeFlags(OpenFileInfo::StatusFlag::Open);
            /// If file has a filename, remember as recently used
            if (openFileInfo->flags().testFlag(OpenFileInfo::StatusFlag::HasName))
                openFileInfo->addFlags(OpenFileInfo::StatusFlag::RecentlyUsed);
        } else if (nextCurrent == nullptr && ofi->flags().testFlag(OpenFileInfo::StatusFlag::Open))
            nextCurrent = ofi;
    }

    /// If the current document is to be closed,
    /// switch over to the next available one
    if (isClosing)
        setCurrentFile(nextCurrent);

    return isClosing;
}

bool OpenFileInfoManager::queryCloseAll()
{
    /// Assume that all closing operations succeed
    bool isClosing = true;
    /// For keeping track of files that get closed here
    OpenFileInfoList restoreLaterList;

    /// For each file known ...
    for (OpenFileInfo *openFileInfo : const_cast<const OpenFileInfoManager::OpenFileInfoList &>(d->openFileInfoList)) {
        /// Check only open file (ignore recently used, favorites, ...)
        if (openFileInfo->flags().testFlag(OpenFileInfo::StatusFlag::Open)) {
            if (openFileInfo->close()) {
                /// If file could be closed without user canceling the operation ...
                /// Mark file as closed (i.e. not open)
                openFileInfo->removeFlags(OpenFileInfo::StatusFlag::Open);
                /// If file has a filename, remember as recently used
                if (openFileInfo->flags().testFlag(OpenFileInfo::StatusFlag::HasName))
                    openFileInfo->addFlags(OpenFileInfo::StatusFlag::RecentlyUsed);
                /// Remember file as to be marked as open later
                restoreLaterList.append(openFileInfo);
            } else {
                /// User chose to cancel closing operation,
                /// stop everything here
                isClosing = false;
                break;
            }
        }
    }

    if (isClosing) {
        /// Closing operation was not cancelled, therefore mark
        /// all files that were open before as open now.
        /// This makes the files to be reopened when KBibTeX is
        /// restarted again (assuming that this function was
        /// called when KBibTeX is exiting).
        for (OpenFileInfo *openFileInfo : const_cast<const OpenFileInfoManager::OpenFileInfoList &>(restoreLaterList)) {
            openFileInfo->addFlags(OpenFileInfo::StatusFlag::Open);
        }

        d->writeConfig();
    }

    return isClosing;
}

OpenFileInfo *OpenFileInfoManager::currentFile() const
{
    return d->currentFileInfo;
}

void OpenFileInfoManager::setCurrentFile(OpenFileInfo *openFileInfo, const KPluginMetaData &service)
{
    bool hasChanged = d->currentFileInfo != openFileInfo;
    OpenFileInfo *previous = d->currentFileInfo;
    d->currentFileInfo = openFileInfo;

    if (d->currentFileInfo != nullptr) {
        d->currentFileInfo->addFlags(OpenFileInfo::StatusFlag::Open);
        d->currentFileInfo->setLastAccess();
    }
    if (hasChanged) {
        if (previous != nullptr) previous->setLastAccess();
        Q_EMIT currentChanged(openFileInfo, service);
    } else if (openFileInfo != nullptr && service.pluginId() != openFileInfo->currentService().pluginId())
        Q_EMIT currentChanged(openFileInfo, service);
}

OpenFileInfoManager::OpenFileInfoList OpenFileInfoManager::filteredItems(OpenFileInfo::StatusFlag required, OpenFileInfo::StatusFlags forbidden)
{
    OpenFileInfoList result;

    for (OpenFileInfoList::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (ofi->flags().testFlag(required) && (ofi->flags() & forbidden) == 0)
            result << ofi;
    }

    if (required == OpenFileInfo::StatusFlag::RecentlyUsed)
        std::sort(result.begin(), result.end(), OpenFileInfoManagerPrivate::byLRULessThan);
    else if (required == OpenFileInfo::StatusFlag::Favorite || required == OpenFileInfo::StatusFlag::Open)
        std::sort(result.begin(), result.end(), OpenFileInfoManagerPrivate::byNameLessThan);

    return result;
}

void OpenFileInfoManager::deferredListsChanged()
{
    OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::StatusFlag::Open;
    statusFlags |= OpenFileInfo::StatusFlag::RecentlyUsed;
    statusFlags |= OpenFileInfo::StatusFlag::Favorite;
    Q_EMIT flagsChanged(statusFlags);
}
