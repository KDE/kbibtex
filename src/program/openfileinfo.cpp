/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "openfileinfo.h"

#include <QString>
#include <QLatin1String>
#include <QTimer>
#include <QFileInfo>
#include <QWidget>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMimeTypeTrader>
#include <KUrl>
#include <kparts/part.h>
#include <KIO/NetAccess>

#include "fileimporterpdf.h"
#include "openfileinfoadaptor.h"

class OpenFileInfo::OpenFileInfoPrivate
{
private:
    static int globalCounter;
    static int fileIdCounter;
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
    KUrl url;

    int fileId;

    OpenFileInfoPrivate(OpenFileInfoManager *openFileInfoManager, const KUrl &url, const QString &mimeType, OpenFileInfo *p)
        :  m_counter(-1), p(p), part(NULL), internalServicePtr(KService::Ptr()), internalWidgetParent(NULL), flags(0), fileId(fileIdCounter++) {
        this->openFileInfoManager = openFileInfoManager;
        this->url = url;
        if (this->url.scheme().isEmpty())
            kWarning() << "No scheme specified for URL" << this->url.pathOrUrl();
        this->mimeType = mimeType;
    }

    ~OpenFileInfoPrivate() {
        if (part != NULL) {
            KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(part);
            if (rwp != NULL)
                rwp->closeUrl(true);
            delete part;
        }
    }

    KParts::ReadOnlyPart *createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr()) {
        if (!p->flags().testFlag(OpenFileInfo::Open)) {
            kWarning() << "Cannot create part for a file which is not open";
            return NULL;
        }

        Q_ASSERT_X(internalWidgetParent == NULL || internalWidgetParent == newWidgetParent, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr())", "internal widget should be either NULL or the same one as supplied as \"newWidgetParent\"");

        /** use cached part for this parent if possible */
        if (internalWidgetParent == newWidgetParent && (newServicePtr == KService::Ptr() || internalServicePtr == newServicePtr)) {
            Q_ASSERT_X(part != NULL, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr())", "Part is NULL");
            return part;
        } else if (part != NULL) {
            KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(part);
            if (rwp != NULL)
                rwp->closeUrl(true);
            part->deleteLater();
            part = NULL;
        }

        /// reset to invalid values in case something goes wrong
        internalServicePtr = KService::Ptr();
        internalWidgetParent = NULL;

        if (newServicePtr.isNull()) {
            /// no valid KService has been passed
            /// try to find a read-write part to open file
            newServicePtr = p->defaultService();
        }
        if (newServicePtr.isNull()) {
            kError() << "Cannot find service to handle mimetype " << mimeType << endl;
            return NULL;
        }

        part = newServicePtr->createInstance<KParts::ReadWritePart>(newWidgetParent, (QObject *)newWidgetParent);
        if (part == NULL) {
            /// creating a read-write part failed, so maybe it is read-only (like Okular's PDF viewer)?
            part = newServicePtr->createInstance<KParts::ReadOnlyPart>(newWidgetParent, (QObject *)newWidgetParent);
        }
        if (part == NULL) {
            /// still cannot create part, must be error
            kError() << "Cannot find part for mimetype " << mimeType << endl;
            return NULL;
        }

        if (url.isValid()) {
            /// open URL in part
            part->openUrl(url);
            /// update document list widget accordingly
            p->addFlags(OpenFileInfo::RecentlyUsed);
            p->addFlags(OpenFileInfo::HasName);
        } else {
            /// initialize part with empty document
            part->openUrl(KUrl());
        }
        p->addFlags(OpenFileInfo::Open);

        internalServicePtr = newServicePtr;
        internalWidgetParent = newWidgetParent;

        Q_ASSERT_X(part != NULL, "KParts::ReadOnlyPart *OpenFileInfo::OpenFileInfoPrivate::createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr())", "Creation of part failed, is NULL"); /// test should not be necessary, but just to be save ...
        return part;
    }

    int counter() {
        if (!url.isValid() && m_counter < 0)
            m_counter = ++globalCounter;
        else if (url.isValid())
            kWarning() << "This function should not be called if URL is valid";
        return m_counter;
    }

};

int OpenFileInfo::OpenFileInfoPrivate::globalCounter = 0;
int OpenFileInfo::OpenFileInfoPrivate::fileIdCounter = OpenFileInfo::FileIdStart;
const QString OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat = QLatin1String("yyyy-MM-dd-hh-mm-ss-zzz");
const QString OpenFileInfo::OpenFileInfoPrivate::keyLastAccess = QLatin1String("LastAccess");
const QString OpenFileInfo::OpenFileInfoPrivate::keyURL = QLatin1String("URL");

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const KUrl &url)
        : d(new OpenFileInfoPrivate(openFileInfoManager, url, FileInfo::mimeTypeForUrl(url)->name(), this))
{
    new OpenFileInfoAdaptor(this, openFileInfoManager);
}

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType)
        : d(new OpenFileInfoPrivate(openFileInfoManager, KUrl(), mimeType, this))
{
    // nothing
}

OpenFileInfo::~OpenFileInfo()
{
    delete d;
}

void OpenFileInfo::setUrl(const KUrl &url)
{
    Q_ASSERT_X(url.isValid(), "void OpenFileInfo::setUrl(const KUrl&)", "URL is not valid");
    d->url = url;
    if (d->url.scheme().isEmpty())
        kWarning() << "No scheme specified for URL" << d->url.pathOrUrl();
    d->mimeType = FileInfo::mimeTypeForUrl(url)->name();
    addFlags(OpenFileInfo::HasName);
}

KUrl OpenFileInfo::url() const
{
    return d->url;
}

bool OpenFileInfo::isModified() const
{
    KParts::ReadWritePart *rwPart = qobject_cast< KParts::ReadWritePart *>(d->part);
    if (rwPart == NULL)
        return false;
    else
        return rwPart->isModified();
}

bool OpenFileInfo::save()
{
    KParts::ReadWritePart *rwPart = qobject_cast< KParts::ReadWritePart *>(d->part);
    if (rwPart == NULL)
        return true;
    else
        return rwPart->save();
}

bool OpenFileInfo::close()
{
    if (d->part == NULL) {
        /// if there is no part, closing always "succeeds"
        return true;
    }

    KParts::ReadWritePart *rwp = qobject_cast<KParts::ReadWritePart *>(d->part);
    if (rwp == NULL || rwp->closeUrl(true)) {
        d->part->deleteLater();
        d->part = NULL;
        d->internalWidgetParent = NULL;
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
        return d->url.pathOrUrl();
    else
        return shortCaption();
}

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
    KService::List result = KMimeTypeTrader::self()->query(mimeType(), QLatin1String("KParts/ReadWritePart"));
    if (result.isEmpty())
        result = KMimeTypeTrader::self()->query(mimeType(), QLatin1String("KParts/ReadOnlyPart"));
    return result;
}

KService::Ptr OpenFileInfo::defaultService()
{
    const QString mt = mimeType();
    KService::Ptr result = KMimeTypeTrader::self()->preferredService(mt, QLatin1String("KParts/ReadWritePart"));
    if (result.isNull())
        result = KMimeTypeTrader::self()->preferredService(mt, QLatin1String("KParts/ReadOnlyPart"));
    return result;
}

KService::Ptr OpenFileInfo::currentService()
{
    return d->internalServicePtr;
}

int OpenFileInfo::fileId() const
{
    return d->fileId;
}

class OpenFileInfoManager::OpenFileInfoManagerPrivate
{
private:
    static const QString configGroupNameRecentlyUsed;
    static const QString configGroupNameFavorites;
    static const QString configGroupNameOpen;
    static const int maxNumRecentlyUsedFiles, maxNumFavoriteFiles, maxNumOpenFiles;
    QWidget *widget;

public:
    OpenFileInfoManager *p;

    OpenFileInfoManager::OpenFileInfoList openFileInfoList;
    OpenFileInfo *currentFileInfo;

    OpenFileInfoManagerPrivate(OpenFileInfoManager *parent, QWidget *w)
            : widget(w), p(parent), currentFileInfo(NULL) {
        // nothing
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
        KSharedConfigPtr config = KSharedConfig::openConfig("kbibtexrc");

        bool isFirst = true;
        KConfigGroup cg(config, configGroupName);
        for (int i = 0; i < maxNumFiles; ++i) {
            KUrl fileUrl = KUrl::fromUserInput(cg.readEntry(QString(QLatin1String("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), QString()));
            if (!fileUrl.isValid()) break;
            if (fileUrl.scheme().isEmpty())
                fileUrl.setScheme(QLatin1String("file"));

            /// For local files, test if they exist; ignore local files that do not exist
            if (fileUrl.isLocalFile()) {
                if (!QFileInfo(fileUrl.pathOrUrl()).exists())
                    continue;
            } else if (!KIO::NetAccess::exists(fileUrl, KIO::NetAccess::SourceSide, widget))
                continue;

            OpenFileInfo *ofi = p->contains(fileUrl);
            if (ofi == NULL) {
                ofi = p->open(fileUrl);
            }
            ofi->addFlags(statusFlag);
            ofi->addFlags(OpenFileInfo::HasName);
            ofi->setLastAccess(QDateTime::fromString(cg.readEntry(QString(QLatin1String("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ""), OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
            if (isFirst) {
                isFirst = false;
                if (statusFlag == OpenFileInfo::Open)
                    p->setCurrentFile(ofi);
            }
        }
    }

    void writeConfig(OpenFileInfo::StatusFlag statusFlag, const QString &configGroupName, int maxNumFiles) {
        KSharedConfigPtr config = KSharedConfig::openConfig("kbibtexrc");
        KConfigGroup cg(config, configGroupName);
        OpenFileInfoManager::OpenFileInfoList list = p->filteredItems(statusFlag);

        int i = 0;
        for (OpenFileInfoManager::OpenFileInfoList::ConstIterator it = list.constBegin(); i < maxNumFiles && it != list.constEnd(); ++it, ++i) {
            OpenFileInfo *ofi = *it;

            cg.writeEntry(QString(QLatin1String("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ofi->url().pathOrUrl());
            cg.writeEntry(QString(QLatin1String("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ofi->lastAccess().toString(OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
        }
        for (; i < maxNumFiles; ++i) {
            cg.deleteEntry(QString(QLatin1String("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i));
            cg.deleteEntry(QString(QLatin1String("%1-%2")).arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i));
        }
        config->sync();
    }
};

const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameRecentlyUsed = QLatin1String("DocumentList-RecentlyUsed");
const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameFavorites = QLatin1String("DocumentList-Favorites");
const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameOpen = QLatin1String("DocumentList-Open");
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumFavoriteFiles = 256;
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumRecentlyUsedFiles = 8;
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumOpenFiles = 16;


OpenFileInfoManager::OpenFileInfoManager(QWidget *widget)
        : QObject(widget), d(new OpenFileInfoManagerPrivate(this, widget))
{
    new OpenFileInfoManagerAdaptor(this);
    QTimer::singleShot(300, this, SLOT(delayedReadConfig()));
}

OpenFileInfoManager::~OpenFileInfoManager()
{
    delete d;
}

OpenFileInfo *OpenFileInfoManager::createNew(const QString &mimeType)
{
    OpenFileInfo *result = new OpenFileInfo(this, mimeType);
    connect(result, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)));
    d->openFileInfoList << result;
    result->setLastAccess();
    return result;
}

OpenFileInfo *OpenFileInfoManager::open(const KUrl &url)
{
    Q_ASSERT_X(url.isValid(), "OpenFileInfo *OpenFileInfoManager::open(const KUrl&)", "URL is not valid");

    OpenFileInfo *result = contains(url);
    if (result == NULL) {
        /// file not yet open
        result = new OpenFileInfo(this, url);
        connect(result, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)));
        d->openFileInfoList << result;
    } /// else: file was already open, re-use and return existing OpenFileInfo pointer
    result->setLastAccess();
    return result;
}

OpenFileInfo *OpenFileInfoManager::contains(const KUrl &url) const
{
    if (!url.isValid()) return NULL; /// can only be unnamed file

    for (OpenFileInfoList::ConstIterator it = d->openFileInfoList.constBegin(); it != d->openFileInfoList.constEnd(); ++it) {
        OpenFileInfo *ofi = *it;
        if (ofi->url().equals(url))
            return ofi;
    }
    return NULL;
}

bool OpenFileInfoManager::changeUrl(OpenFileInfo *openFileInfo, const KUrl &url)
{
    OpenFileInfo *previouslyContained = contains(url);

    /// check if old url differs from new url and old url is valid
    if (previouslyContained != NULL && previouslyContained->flags().testFlag(OpenFileInfo::Open) && previouslyContained != openFileInfo) {
        kDebug() << "Open file with same URL already exists, forcefully closing it" << endl;
        close(previouslyContained);
    }

    KUrl oldUrl = openFileInfo->url();
    openFileInfo->setUrl(url);

    if (!url.equals(oldUrl) && oldUrl.isValid()) {
        /// current document was most probabily renamed (e.g. due to "Save As")
        /// add old URL to recently used files, but exclude the open files list
        OpenFileInfo *ofi = open(oldUrl); // krazy:exclude=syscalls
        OpenFileInfo::StatusFlags statusFlags = (openFileInfo->flags() & (~OpenFileInfo::Open)) | OpenFileInfo::RecentlyUsed;
        ofi->setFlags(statusFlags);
    }
    if (previouslyContained != NULL) {
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
    Q_ASSERT_X(openFileInfo != NULL, "void OpenFileInfoManager::close(OpenFileInfo *openFileInfo)", "Cannot close openFileInfo which is NULL");
    bool isClosing = false;
    openFileInfo->setLastAccess();

    /// remove flag "open" from file to be closed and determine which file to show instead
    OpenFileInfo *nextCurrent = (d->currentFileInfo == openFileInfo) ? NULL : d->currentFileInfo;
    foreach (OpenFileInfo *ofi, d->openFileInfoList) {
        if (!isClosing && ofi == openFileInfo && openFileInfo->close()) {
            isClosing = true;
            /// Mark file as closed (i.e. not open)
            openFileInfo->removeFlags(OpenFileInfo::Open);
            /// If file has a filename, remember as recently used
            if (openFileInfo->flags().testFlag(OpenFileInfo::HasName))
                openFileInfo->addFlags(OpenFileInfo::RecentlyUsed);
        } else if (nextCurrent == NULL && ofi->flags().testFlag(OpenFileInfo::Open))
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
    foreach (OpenFileInfo *openFileInfo, d->openFileInfoList) {
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
        foreach (OpenFileInfo *openFileInfo, restoreLaterList) {
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

void OpenFileInfoManager::setCurrentFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr)
{
    bool hasChanged = d->currentFileInfo != openFileInfo;
    OpenFileInfo *previous = d->currentFileInfo;
    d->currentFileInfo = openFileInfo;

    if (d->currentFileInfo != NULL) {
        d->currentFileInfo->addFlags(OpenFileInfo::Open);
        d->currentFileInfo->setLastAccess();
    }
    if (hasChanged) {
        if (previous != NULL) previous->setLastAccess();
        emit currentChanged(openFileInfo, servicePtr);
    } else if (openFileInfo == NULL || servicePtr != openFileInfo->currentService())
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
