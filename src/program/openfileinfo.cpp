/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#include <QDebug>
#include <QApplication>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KMimeTypeTrader>
#include <KParts/Part>
#include <KParts/ReadOnlyPart>
#include <KParts/ReadWritePart>

#include "fileimporterpdf.h"
#include "logging_program.h"

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
    KService::Ptr internalServicePtr;
    QWidget *internalWidgetParent;
    QDateTime lastAccessDateTime;
    StatusFlags flags;
    OpenFileInfoManager *openFileInfoManager;
    QString mimeType;
    QUrl url;

    OpenFileInfoPrivate(OpenFileInfoManager *openFileInfoManager, const QUrl &url, const QString &mimeType, OpenFileInfo *p)
        :  m_counter(-1), p(p), part(nullptr), internalServicePtr(KService::Ptr()), internalWidgetParent(nullptr), flags(nullptr) {
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

    KParts::ReadOnlyPart *createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr()) {
        if (!p->flags().testFlag(OpenFileInfo::Open)) {
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Cannot create part for a file which is not open";
            return nullptr;
        }

        Q_ASSERT_X(internalWidgetParent == nullptr || internalWidgetParent == newWidgetParent, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr())", "internal widget should be either NULL or the same one as supplied as \"newWidgetParent\"");

        /** use cached part for this parent if possible */
        if (internalWidgetParent == newWidgetParent && (newServicePtr == KService::Ptr() || internalServicePtr == newServicePtr)) {
            Q_ASSERT_X(part != nullptr, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr())", "Part is NULL");
            return part;
        } else if (part != nullptr) {
            KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(part);
            if (rwp != nullptr)
                rwp->closeUrl(true);
            part->deleteLater();
            part = nullptr;
        }

        /// reset to invalid values in case something goes wrong
        internalServicePtr = KService::Ptr();
        internalWidgetParent = nullptr;

        if (!newServicePtr) {
            /// no valid KService has been passed
            /// try to find a read-write part to open file
            newServicePtr = p->defaultService();
        }
        if (!newServicePtr) {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "PATH=" << getenv("PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "LD_LIBRARY_PATH=" << getenv("LD_LIBRARY_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "XDG_DATA_DIRS=" << getenv("XDG_DATA_DIRS");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "QT_PLUGIN_PATH=" << getenv("QT_PLUGIN_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "KDEDIRS=" << getenv("KDEDIRS");
            qCCritical(LOG_KBIBTEX_PROGRAM) << "Cannot find service to handle mimetype " << mimeType << endl;
            return nullptr;
        }

        QString errorString;
        part = newServicePtr->createInstance<KParts::ReadWritePart>(newWidgetParent, (QObject *)newWidgetParent, QVariantList(), &errorString);
        if (part == nullptr) {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "PATH=" << getenv("PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "LD_LIBRARY_PATH=" << getenv("LD_LIBRARY_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "XDG_DATA_DIRS=" << getenv("XDG_DATA_DIRS");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "QT_PLUGIN_PATH=" << getenv("QT_PLUGIN_PATH");
            qCDebug(LOG_KBIBTEX_PROGRAM) << "KDEDIRS=" << getenv("KDEDIRS");
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Could not instantiate read-write part for service" << newServicePtr->name() << "(mimeType=" << mimeType << ", library=" << newServicePtr->library() << ", error msg=" << errorString << ")";
            /// creating a read-write part failed, so maybe it is read-only (like Okular's PDF viewer)?
            part = newServicePtr->createInstance<KParts::ReadOnlyPart>(newWidgetParent, (QObject *)newWidgetParent, QVariantList(), &errorString);
        }
        if (part == nullptr) {
            /// still cannot create part, must be error
            qCCritical(LOG_KBIBTEX_PROGRAM) << "Could not instantiate part for service" << newServicePtr->name() << "(mimeType=" << mimeType << ", library=" << newServicePtr->library() << ", error msg=" << errorString << ")";
            return nullptr;
        }

        if (url.isValid()) {
            /// open URL in part
            part->openUrl(url);
            /// update document list widget accordingly
            p->addFlags(OpenFileInfo::RecentlyUsed);
            p->addFlags(OpenFileInfo::HasName);
        } else {
            /// initialize part with empty document
            part->openUrl(QUrl());
        }
        p->addFlags(OpenFileInfo::Open);

        internalServicePtr = newServicePtr;
        internalWidgetParent = newWidgetParent;

        Q_ASSERT_X(part != nullptr, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr())", "Creation of part failed, is NULL"); /// test should not be necessary, but just to be save ...
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
    addFlags(OpenFileInfo::HasName);
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

/// Clazy warns: "Missing reference on non-trivial type" for argument 'servicePtr',
/// but type 'KService::Ptr' is actually a pointer (QExplicitlySharedDataPointer).
KParts::ReadOnlyPart *OpenFileInfo::part(QWidget *parent, KService::Ptr servicePtr)
{
    return d->createPart(parent, servicePtr);
}

OpenFileInfo::StatusFlags OpenFileInfo::flags() const
{
    return d->flags;
}

void OpenFileInfo::setFlags(StatusFlags statusFlags)
{
    /// disallow files without name or valid url to become favorites
    if (!d->url.isValid() || !d->flags.testFlag(HasName)) statusFlags &= ~Favorite;
    /// files that got opened are by definition recently used files
    if (!d->url.isValid() && d->flags.testFlag(Open)) statusFlags &= RecentlyUsed;

    bool hasChanged = d->flags != statusFlags;
    d->flags = statusFlags;
    if (hasChanged)
        emit flagsChanged(statusFlags);
}

void OpenFileInfo::addFlags(StatusFlags statusFlags)
{
    /// disallow files without name or valid url to become favorites
    if (!d->url.isValid() || !d->flags.testFlag(HasName)) statusFlags &= ~Favorite;

    bool hasChanged = (~d->flags & statusFlags) > 0;
    d->flags |= statusFlags;
    if (hasChanged)
        emit flagsChanged(statusFlags);
}

void OpenFileInfo::removeFlags(StatusFlags statusFlags)
{
    bool hasChanged = (d->flags & statusFlags) > 0;
    d->flags &= ~statusFlags;
    if (hasChanged)
        emit flagsChanged(statusFlags);
}

QDateTime OpenFileInfo::lastAccess() const
{
    return d->lastAccessDateTime;
}

void OpenFileInfo::setLastAccess(const QDateTime &dateTime)
{
    d->lastAccessDateTime = dateTime;
    emit flagsChanged(OpenFileInfo::RecentlyUsed);
}

KService::List OpenFileInfo::listOfServices()
{
    const QString mt = mimeType();
    /// First, try to locate KPart that can both read and write the queried MIME type
    KService::List result = KMimeTypeTrader::self()->query(mt, QStringLiteral("KParts/ReadWritePart"));
    if (result.isEmpty()) {
        /// Second, if no 'writing' KPart was found, try to locate KPart that can at least read the queried MIME type
        result = KMimeTypeTrader::self()->query(mt, QStringLiteral("KParts/ReadOnlyPart"));
        if (result.isEmpty()) {
            /// If not even a 'reading' KPart was found, something is off, so warn the user and stop here
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Could not find any KPart that reads or writes mimetype" << mt;
            return result;
        }
    }

    /// Always include KBibTeX's KPart in list of services:
    /// First, check if KBibTeX's KPart is already in list as returned by
    /// KMimeTypeTrader::self()->query(..)
    bool listIncludesKBibTeXPart = false;
    for (KService::List::ConstIterator it = result.constBegin(); it != result.constEnd(); ++it) {
        qCDebug(LOG_KBIBTEX_PROGRAM) << "Found library for" << mt << ":" << (*it)->library();
        listIncludesKBibTeXPart |= (*it)->library() == QStringLiteral("kbibtexpart");
    }
    /// Then, if KBibTeX's KPart is not in the list, try to located it by desktop name
    if (!listIncludesKBibTeXPart) {
        KService::Ptr kbibtexpartByDesktopName = KService::serviceByDesktopName(QStringLiteral("kbibtexpart"));
        if (kbibtexpartByDesktopName != nullptr) {
            result << kbibtexpartByDesktopName;
            qCDebug(LOG_KBIBTEX_PROGRAM) << "Adding library for" << mt << ":" << kbibtexpartByDesktopName->library();
        } else {
            qCDebug(LOG_KBIBTEX_PROGRAM) << "Could not locate KBibTeX's KPart neither by MIME type search, nor by desktop name";
        }
    }

    return result;
}

KService::Ptr OpenFileInfo::defaultService()
{
    const QString mt = mimeType();
    KService::Ptr result;
    if (mt == QStringLiteral("application/pdf") || mt == QStringLiteral("text/x-bibtex")) {
        /// If either a BibTeX file or a PDF file is to be opened, enforce using
        /// KBibTeX's part over anything else.
        /// KBibTeX has a FileImporterPDF which allows it to load .pdf file
        /// that got generated with KBibTeX and contain the original
        /// .bib file as an 'attachment'.
        /// This importer does not work with any other .pdf files!!!
        result = KService::serviceByDesktopName(QStringLiteral("kbibtexpart"));
    }
    if (result == nullptr) {
        /// First, try to locate KPart that can both read and write the queried MIME type
        result = KMimeTypeTrader::self()->preferredService(mt, QStringLiteral("KParts/ReadWritePart"));
        if (result == nullptr) {
            /// Second, if no 'writing' KPart was found, try to locate KPart that can at least read the queried MIME type
            result = KMimeTypeTrader::self()->preferredService(mt, QStringLiteral("KParts/ReadOnlyPart"));
            if (result == nullptr && mt == QStringLiteral("text/x-bibtex"))
                /// Third, if MIME type is for BibTeX files, try loading KBibTeX part via desktop name
                result = KService::serviceByDesktopName(QStringLiteral("kbibtexpart"));
        }
    }
    if (result != nullptr)
        qCDebug(LOG_KBIBTEX_PROGRAM) << "Using service" << result->name() << "(" << result->comment() << ") for mime type" << mt << "through library" << result->library();
    else
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Could not find service for mime type" << mt;
    return result;
}

KService::Ptr OpenFileInfo::currentService()
{
    return d->internalServicePtr;
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
        for (OpenFileInfoManager::OpenFileInfoList::Iterator it = openFileInfoList.begin(); it != openFileInfoList.end(); ++it) {
            OpenFileInfo *ofi = *it;
            delete ofi;
        }
    }

    static bool byNameLessThan(const OpenFileInfo *left, const OpenFileInfo *right) {
        return left->shortCaption() < right->shortCaption();
    }

    static  bool byLRULessThan(const OpenFileInfo *left, const OpenFileInfo *right) {
        return left->lastAccess() > right->lastAccess(); /// reverse sorting!
    }

    void readConfig() {
        readConfig(OpenFileInfo::RecentlyUsed, configGroupNameRecentlyUsed, maxNumRecentlyUsedFiles);
        readConfig(OpenFileInfo::Favorite, configGroupNameFavorites, maxNumFavoriteFiles);
        readConfig(OpenFileInfo::Open, configGroupNameOpen, maxNumOpenFiles);
    }

    void writeConfig() {
        writeConfig(OpenFileInfo::RecentlyUsed, configGroupNameRecentlyUsed, maxNumRecentlyUsedFiles);
        writeConfig(OpenFileInfo::Favorite, configGroupNameFavorites, maxNumFavoriteFiles);
        writeConfig(OpenFileInfo::Open, configGroupNameOpen, maxNumOpenFiles);
    }

    void readConfig(OpenFileInfo::StatusFlag statusFlag, const QString &configGroupName, int maxNumFiles) {
        KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));

        bool isFirst = true;
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
            ofi->addFlags(OpenFileInfo::HasName);
            ofi->setLastAccess(QDateTime::fromString(cg.readEntry(QString(QStringLiteral("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ""), OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
            if (isFirst) {
                isFirst = false;
                if (statusFlag == OpenFileInfo::Open)
                    p->setCurrentFile(ofi);
            }
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

OpenFileInfoManager *OpenFileInfoManager::singleton = nullptr;

OpenFileInfoManager::OpenFileInfoManager(QObject *parent)
        : QObject(parent), d(new OpenFileInfoManagerPrivate(this))
{
    QTimer::singleShot(300, this, &OpenFileInfoManager::delayedReadConfig);
}

OpenFileInfoManager *OpenFileInfoManager::instance() {
    if (singleton == nullptr)
        singleton = new OpenFileInfoManager(QCoreApplication::instance());
    return singleton;
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
    if (previouslyContained != nullptr && previouslyContained->flags().testFlag(OpenFileInfo::Open) && previouslyContained != openFileInfo) {
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Open file with same URL already exists, forcefully closing it" << endl;
        close(previouslyContained);
    }

    QUrl oldUrl = openFileInfo->url();
    openFileInfo->setUrl(url);

    if (url != oldUrl && oldUrl.isValid()) {
        /// current document was most probabily renamed (e.g. due to "Save As")
        /// add old URL to recently used files, but exclude the open files list
        OpenFileInfo *ofi = open(oldUrl); // krazy:exclude=syscalls
        OpenFileInfo::StatusFlags statusFlags = (openFileInfo->flags() & (~OpenFileInfo::Open)) | OpenFileInfo::RecentlyUsed;
        ofi->setFlags(statusFlags);
    }
    if (previouslyContained != nullptr) {
        /// keep Favorite flag if set in file that have previously same URL
        if (previouslyContained->flags().testFlag(OpenFileInfo::Favorite))
            openFileInfo->setFlags(openFileInfo->flags() | OpenFileInfo::Favorite);

        /// remove the old entry with the same url has it will be replaced by the new one
        d->openFileInfoList.remove(d->openFileInfoList.indexOf(previouslyContained));
        previouslyContained->deleteLater();
        OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::Open;
        statusFlags |= OpenFileInfo::RecentlyUsed;
        statusFlags |= OpenFileInfo::Favorite;
        emit flagsChanged(statusFlags);
    }

    if (openFileInfo == d->currentFileInfo)
        emit currentChanged(openFileInfo, KService::Ptr());
    emit flagsChanged(openFileInfo->flags());

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
            openFileInfo->removeFlags(OpenFileInfo::Open);
            /// If file has a filename, remember as recently used
            if (openFileInfo->flags().testFlag(OpenFileInfo::HasName))
                openFileInfo->addFlags(OpenFileInfo::RecentlyUsed);
        } else if (nextCurrent == nullptr && ofi->flags().testFlag(OpenFileInfo::Open))
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
        if (openFileInfo->flags().testFlag(OpenFileInfo::Open)) {
            if (openFileInfo->close()) {
                /// If file could be closed without user canceling the operation ...
                /// Mark file as closed (i.e. not open)
                openFileInfo->removeFlags(OpenFileInfo::Open);
                /// If file has a filename, remember as recently used
                if (openFileInfo->flags().testFlag(OpenFileInfo::HasName))
                    openFileInfo->addFlags(OpenFileInfo::RecentlyUsed);
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
            openFileInfo->addFlags(OpenFileInfo::Open);
        }

        d->writeConfig();
    }

    return isClosing;
}

OpenFileInfo *OpenFileInfoManager::currentFile() const
{
    return d->currentFileInfo;
}

/// Clazy warns: "Missing reference on non-trivial type" for argument 'servicePtr',
/// but type 'KService::Ptr' is actually a pointer (QExplicitlySharedDataPointer).
void OpenFileInfoManager::setCurrentFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr)
{
    bool hasChanged = d->currentFileInfo != openFileInfo;
    OpenFileInfo *previous = d->currentFileInfo;
    d->currentFileInfo = openFileInfo;

    if (d->currentFileInfo != nullptr) {
        d->currentFileInfo->addFlags(OpenFileInfo::Open);
        d->currentFileInfo->setLastAccess();
    }
    if (hasChanged) {
        if (previous != nullptr) previous->setLastAccess();
        emit currentChanged(openFileInfo, servicePtr);
    } else if (openFileInfo != nullptr && servicePtr != openFileInfo->currentService())
        emit currentChanged(openFileInfo, servicePtr);
}

OpenFileInfoManager::OpenFileInfoList OpenFileInfoManager::filteredItems(OpenFileInfo::StatusFlags required, OpenFileInfo::StatusFlags forbidden)
{
    OpenFileInfoList result;

    for (OpenFileInfoList::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if ((ofi->flags() & required) == required && (ofi->flags() & forbidden) == 0)
            result << ofi;
    }

    if (required == OpenFileInfo::RecentlyUsed)
        qSort(result.begin(), result.end(), OpenFileInfoManagerPrivate::byLRULessThan);
    else if (required == OpenFileInfo::Favorite || required == OpenFileInfo::Open)
        qSort(result.begin(), result.end(), OpenFileInfoManagerPrivate::byNameLessThan);

    return result;
}

void OpenFileInfoManager::deferredListsChanged()
{
    OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::Open;
    statusFlags |= OpenFileInfo::RecentlyUsed;
    statusFlags |= OpenFileInfo::Favorite;
    emit flagsChanged(statusFlags);
}

void OpenFileInfoManager::delayedReadConfig()
{
    d->readConfig();
}
