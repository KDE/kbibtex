/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <QString>
#include <QLatin1String>
#include <QTimer>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KUrl>
#include <kparts/part.h>

#include "openfileinfo.h"

const QString OpenFileInfo::propertyEncoding = QLatin1String("encoding");
const QString OpenFileInfo::mimetypeBibTeX = QLatin1String("text/x-bibtex");

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

    KUrl url;
    QMap<QString, QString> properties;
    QMap<QWidget*, KParts::ReadWritePart*> partPerParent;
    QDateTime lastAccessDateTime;
    StatusFlags flags;
    OpenFileInfoManager *openFileInfoManager;
    QString mimeType;

    OpenFileInfoPrivate(OpenFileInfo *p)
            :  m_counter(-1), p(p), flags(0), mimeType(QString::null) {
        // nothing
    }

    ~OpenFileInfoPrivate() {
        QList<KParts::ReadWritePart*> list = partPerParent.values();
        for (QList<KParts::ReadWritePart*>::Iterator it = list.begin(); it != list.end(); ++it) {
            KParts::ReadWritePart* part = *it;
            part->closeUrl(true);
            delete part;
        }
    }

    KParts::ReadWritePart* createPart(QWidget *parent) {
        /** use cached part for this parent if possible */
        if (partPerParent.contains(parent))
            return partPerParent[parent];

        KParts::ReadWritePart *part = NULL;
        if (mimeType.isNull() && url.isValid()) {
            KMimeType::Ptr mimeTypePtr = KMimeType::findByUrl(url);
            mimeType = mimeTypePtr->name();
        } else if (mimeType.isNull())
            mimeType = OpenFileInfo::mimetypeBibTeX;

        KService::List list = KMimeTypeTrader::self()->query(mimeType, QString::fromLatin1("KParts/ReadWritePart"));
        for (KService::List::Iterator it = list.begin(); it != list.end(); ++it) {
            kDebug() << "service name is " << (*it)->name() << endl;
            if (part == NULL || (*it)->name().contains("BibTeX", Qt::CaseInsensitive))
                part = (*it)->createInstance<KParts::ReadWritePart>((QWidget*)parent, (QObject*)parent);
        }

        if (part == NULL) {
            kError() << "Cannot find part for mimetype " << mimeType << endl;
            return NULL;
        }

        if (url.isValid()) {
            part->openUrl(url);
            p->addFlags(OpenFileInfo::RecentlyUsed);
        } else {
            part->openUrl(KUrl());
        }

        partPerParent[parent] = part;

        p->addFlags(OpenFileInfo::Open);

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
const QString OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat = QLatin1String("yyyy-MM-dd-hh-mm-ss-zzz");
const QString OpenFileInfo::OpenFileInfoPrivate::keyLastAccess = QLatin1String("LastAccess");
const QString OpenFileInfo::OpenFileInfoPrivate::keyURL = QLatin1String("URL");

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const KUrl &url)
        : d(new OpenFileInfoPrivate(this))
{
    d->url = url;
    d->openFileInfoManager = openFileInfoManager;
}

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType)
        : d(new OpenFileInfoPrivate(this))
{
    d->mimeType = mimeType;
    d->openFileInfoManager = openFileInfoManager;
}

OpenFileInfo::~OpenFileInfo()
{
    delete d;
}

void OpenFileInfo::setUrl(const KUrl& url)
{
    Q_ASSERT(url.isValid());
    d->url = url;
}

KUrl OpenFileInfo::url() const
{
    return d->url;
}

void OpenFileInfo::setProperty(const QString &key, const QString &value)
{
    d->properties[key] = value;
}

QString OpenFileInfo::property(const QString &key) const
{
    return d->properties[key];
}

int OpenFileInfo::counter()
{
    return d->counter();
}

QString OpenFileInfo::caption()
{
    if (d->url.isValid())
        return d->url.fileName();
    else
        return i18n("Unnamed-%1", counter());
}

QString OpenFileInfo::fullCaption()
{
    if (d->url.isValid())
        return d->url.prettyUrl();
    else
        return caption();
}

KParts::ReadWritePart* OpenFileInfo::part(QWidget *parent)
{
    return d->createPart(parent);
}

OpenFileInfo::StatusFlags OpenFileInfo::flags() const
{
    return d->flags;
}

void OpenFileInfo::setFlags(StatusFlags statusFlags)
{
    bool hasChanged = d->flags != statusFlags;
    d->flags = statusFlags;
    if (hasChanged)
        d->openFileInfoManager->flagsChangedInternal(statusFlags);
}

void OpenFileInfo::addFlags(StatusFlags statusFlags)
{
    bool hasChanged = (~d->flags & statusFlags) > 0;
    d->flags |= statusFlags;
    if (hasChanged)
        d->openFileInfoManager->flagsChangedInternal(statusFlags);
}

void OpenFileInfo::removeFlags(StatusFlags statusFlags)
{
    bool hasChanged = (d->flags & statusFlags) > 0;
    d->flags &= ~statusFlags;
    if (hasChanged)
        d->openFileInfoManager->flagsChangedInternal(statusFlags);
}

QDateTime OpenFileInfo::lastAccess() const
{
    return d->lastAccessDateTime;
}

void OpenFileInfo::setLastAccess(const QDateTime& dateTime)
{
    d->lastAccessDateTime = dateTime;
}

class OpenFileInfoManager::OpenFileInfoManagerPrivate
{
public:
    OpenFileInfoManager *p;

    QList<OpenFileInfo*> openFileInfoList;
    const QString configGroupNameRecentlyUsed;
    const QString configGroupNameFavorites;
    const int maxNumRecentlyUsedFiles, maxNumFiles;
    OpenFileInfo *currentFileInfo;
    QDateTime *lruDateTimeList;

    OpenFileInfoManagerPrivate(OpenFileInfoManager *parent)
            : p(parent), configGroupNameRecentlyUsed("DocumentList-RecentlyUsed"), configGroupNameFavorites("DocumentList-Favorites"), maxNumRecentlyUsedFiles(8), maxNumFiles(256), currentFileInfo(NULL) {
        lruDateTimeList = new QDateTime[maxNumRecentlyUsedFiles];
    }

    ~OpenFileInfoManagerPrivate() {
        for (QList<OpenFileInfo*>::Iterator it = openFileInfoList.begin(); it != openFileInfoList.end(); ++it) {
            OpenFileInfo *ofi = *it;
            delete ofi;
        }
        delete[] lruDateTimeList;
    }

    void readConfig() {
        readConfig(OpenFileInfo::RecentlyUsed, configGroupNameRecentlyUsed);
        readConfig(OpenFileInfo::Favorite, configGroupNameFavorites);
    }

    void writeConfig() {
        writeConfig(OpenFileInfo::RecentlyUsed, configGroupNameRecentlyUsed);
        writeConfig(OpenFileInfo::Favorite, configGroupNameFavorites);
    }

    void readConfig(OpenFileInfo::StatusFlags statusFlags, const QString& configGroupName) {
        KSharedConfig::Ptr config = KGlobal::config();

        KConfigGroup cg(config, configGroupName);
        for (int i = 0; i < maxNumFiles; ++i) {
            KUrl fileUrl = KUrl(cg.readEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ""));
            if (!fileUrl.isValid()) break;
            OpenFileInfo *ofi = p->contains(fileUrl);
            if (ofi == NULL) {
                ofi = p->open(fileUrl);
            }
            ofi->addFlags(statusFlags);
            QString encoding = cg.readEntry(QString("%1-%2").arg(OpenFileInfo::propertyEncoding).arg(i), "");
            ofi->setLastAccess(QDateTime::fromString(cg.readEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ""), OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
            ofi->setProperty(OpenFileInfo::propertyEncoding, encoding);
        }
    }

    void writeConfig(OpenFileInfo::StatusFlags statusFlags, const QString& configGroupName) {
        KSharedConfig::Ptr config = KGlobal::config();
        KConfigGroup cg(config, configGroupName);
        QList<OpenFileInfo*> list = p->filteredItems(statusFlags);

        if (statusFlags & OpenFileInfo::RecentlyUsed) {
            lruListReset();
            for (QList<OpenFileInfo*>::Iterator it = list.begin(); it != list.end(); ++it) {
                OpenFileInfo *ofi = *it;
                lruListAdd(ofi->lastAccess());
            }
        }

        int i = 0;
        for (QList<OpenFileInfo*>::Iterator it = list.begin(); i < maxNumFiles && it != list.end(); ++it) {
            OpenFileInfo *ofi = *it;

            if ((statusFlags & OpenFileInfo::RecentlyUsed) && ofi->lastAccess() < lruDateTimeList[maxNumRecentlyUsedFiles-1])
                continue;

            cg.writeEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ofi->url().prettyUrl());
            cg.writeEntry(QString("%1-%2").arg(OpenFileInfo::propertyEncoding).arg(i), ofi->property(OpenFileInfo::propertyEncoding));
            cg.writeEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ofi->lastAccess().toString(OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));

            ++i;
        }
        config->sync();
    }

    void lruListReset() {
        static const QDateTime veryOldDate = QDateTime::fromString("1970-01-01", "yyyy-MM-dd");
        for (int i = maxNumRecentlyUsedFiles - 1; i >= 0; --i)
            lruDateTimeList[i] = veryOldDate;
    }

    void lruListAdd(const QDateTime& dateTime) {
        int p;
        for (p = 0; p < maxNumRecentlyUsedFiles; ++p) {
            if (lruDateTimeList[p] < dateTime) break;
        }

        if (p < maxNumRecentlyUsedFiles) {
            for (int i = maxNumRecentlyUsedFiles - 2; i >= p; --i)
                lruDateTimeList[i+1] = lruDateTimeList[i];
            lruDateTimeList[p] = dateTime;
        }
    }
};

OpenFileInfoManager *OpenFileInfoManager::singletonOpenFileInfoManager = NULL;

OpenFileInfoManager* OpenFileInfoManager::getOpenFileInfoManager()
{
    if (singletonOpenFileInfoManager == NULL)
        singletonOpenFileInfoManager = new OpenFileInfoManager();
    return singletonOpenFileInfoManager;
}

OpenFileInfoManager::OpenFileInfoManager()
        : d(new OpenFileInfoManagerPrivate(this))
{
    d->readConfig();
}

OpenFileInfoManager::~OpenFileInfoManager()
{
    d->writeConfig();
    delete d;
}

OpenFileInfo *OpenFileInfoManager::createNew(const QString& mimeType)
{
    kDebug() << "mimeType=" << mimeType;
    OpenFileInfo *result = new OpenFileInfo(this, mimeType);
    d->openFileInfoList << result;
    result->setLastAccess(QDateTime::currentDateTime());
    return result;
}

OpenFileInfo *OpenFileInfoManager::open(const KUrl & url)
{
    Q_ASSERT(url.isValid());

    OpenFileInfo *result = contains(url);
    if (result == NULL) {
        /// file not yet open
        result = new OpenFileInfo(this, url);
        d->openFileInfoList << result;
    }
    result->setLastAccess(QDateTime::currentDateTime());
    return result;
}

OpenFileInfo *OpenFileInfoManager::contains(const KUrl&url) const
{
    if (!url.isValid()) return NULL; /// can only be unnamed file

    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (ofi->url().equals(url))
            return ofi;
    }
    return NULL;
}

void OpenFileInfoManager::changeUrl(OpenFileInfo *openFileInfo, const KUrl & url)
{
    OpenFileInfo *previouslyContained = contains(url);

    /// check if old url differs from new url and old url is valid
    if (previouslyContained != NULL && contains(url) != openFileInfo) {
        kWarning() << "Cannot change URL, open file with same URL already exists" << endl;
    }

    KUrl oldUrl = openFileInfo->url();

    openFileInfo->setUrl(url);
    if (openFileInfo == d->currentFileInfo)
        emit currentChanged(openFileInfo);
    emit listsChanged(openFileInfo->flags());

    if (!url.equals(oldUrl) && oldUrl.isValid()) {
        /// current document was most probabily renamed (e.g. due to "Save As")
        /// add old URL to recently used files, but exclude the open files list
        OpenFileInfo *ofi = open(oldUrl);
        OpenFileInfo::StatusFlags statusFlags = (openFileInfo->flags() & (~OpenFileInfo::Open)) | OpenFileInfo::RecentlyUsed;
        ofi->setFlags(statusFlags);
        ofi->setProperty(OpenFileInfo::propertyEncoding, openFileInfo->property(OpenFileInfo::propertyEncoding));
    }
}

void OpenFileInfoManager::close(OpenFileInfo *openFileInfo)
{
    bool isClosing = false;

    OpenFileInfo *nextCurrent = (d->currentFileInfo == openFileInfo) ? NULL : d->currentFileInfo;

    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (!isClosing && ofi == openFileInfo) {
            isClosing = true;
            openFileInfo->removeFlags(OpenFileInfo::Open);
        } else if (nextCurrent == NULL && ofi->flags().testFlag(OpenFileInfo::Open))
            nextCurrent = ofi;
    }

    setCurrentFile(nextCurrent);

    if (isClosing)
        emit closing(openFileInfo);
}

OpenFileInfo *OpenFileInfoManager::currentFile() const
{
    return d->currentFileInfo;
}

void OpenFileInfoManager::setCurrentFile(OpenFileInfo *openFileInfo)
{
    bool hasChanged = d->currentFileInfo != openFileInfo;
    d->currentFileInfo = openFileInfo;

    if (d->currentFileInfo != NULL) {
        d->currentFileInfo->addFlags(OpenFileInfo::Open);
        d->currentFileInfo->setLastAccess(QDateTime::currentDateTime());
    }
    if (hasChanged)
        emit currentChanged(openFileInfo);
}

QList<OpenFileInfo*> OpenFileInfoManager::filteredItems(OpenFileInfo::StatusFlags required, OpenFileInfo::StatusFlags forbidden)
{
    QList<OpenFileInfo*> result;

    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if ((ofi->flags()&required) == required && (ofi->flags()&forbidden) == 0)
            result << ofi;
    }

    return result;
}

void OpenFileInfoManager::flagsChangedInternal(OpenFileInfo::StatusFlags statusFlags)
{
    emit listsChanged(statusFlags);
}

void OpenFileInfoManager::deferredListsChanged()
{
    kDebug() << "deferredListsChanged" << endl;
    OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::Open;
    statusFlags |= OpenFileInfo::RecentlyUsed;
    statusFlags |= OpenFileInfo::Favorite;
    emit listsChanged(statusFlags);
}
