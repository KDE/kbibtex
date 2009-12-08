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

#include <KMessageBox>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMimeTypeTrader>
#include <KUrl>
#include <kparts/part.h>
#include <kio/netaccess.h>

#include "openfileinfo.h"

using namespace KBibTeX::Program;

const QString OpenFileInfo::propertyEncoding = QLatin1String("encoding");
const QString OpenFileInfo::mimetypeBibTeX = QLatin1String("text/x-bibtex");

class OpenFileInfo::OpenFileInfoPrivate
{
private:
    static unsigned int globalCounter;

public:
    OpenFileInfo *p;

    QString mimeType;
    KUrl url;
    QMap<QString, QString> properties;
    QMap<QWidget*, KParts::ReadWritePart*> partPerParent;
    StatusFlags flags;
    OpenFileInfoManager *openFileInfoManager;
    unsigned int counter;

    OpenFileInfoPrivate(OpenFileInfo *p)
            : p(p), flags(0), counter(0) {
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
            p->setFlags(OpenFileInfo::RecentlyUsed);
        } else {
            ++globalCounter;
            counter = globalCounter;
            part->openUrl(KUrl());
        }

        partPerParent[parent] = part;

        p->setFlags(OpenFileInfo::Open);

        return part;
    }

};

unsigned int OpenFileInfo::OpenFileInfoPrivate::globalCounter = 0;

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType, const KUrl &url)
        : d(new OpenFileInfoPrivate(this))
{
    d->mimeType = mimeType;
    d->url = url;
    d->openFileInfoManager = openFileInfoManager;
}

OpenFileInfo::~OpenFileInfo()
{
    delete d;
}

void OpenFileInfo::setUrl(const KUrl& url)
{
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

unsigned int OpenFileInfo::counter()
{
    return d->counter;
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
    bool hasChanged = (~d->flags & statusFlags) > 0;
    d->flags |= statusFlags;
    if (hasChanged)
        d->openFileInfoManager->flagsChangedInternal(statusFlags);
}

void OpenFileInfo::clearFlags(StatusFlags statusFlags)
{
    bool hasChanged = (d->flags & statusFlags) > 0;
    d->flags &= ~statusFlags;
    if (hasChanged)
        d->openFileInfoManager->flagsChangedInternal(statusFlags);
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

    OpenFileInfoManagerPrivate(OpenFileInfoManager *parent)
            : p(parent), configGroupNameRecentlyUsed("DocumentList-RecentlyUsed"), configGroupNameFavorites("DocumentList-Favorites"), maxNumRecentlyUsedFiles(8), maxNumFiles(256), currentFileInfo(NULL) {
        // nothing
    }

    ~OpenFileInfoManagerPrivate() {
        for (QList<OpenFileInfo*>::Iterator it = openFileInfoList.begin(); it != openFileInfoList.end(); ++it) {
            OpenFileInfo *ofi = *it;
            delete ofi;
        }
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
        for (int i = 0; i < maxNumRecentlyUsedFiles; ++i) {
            KUrl fileUrl = KUrl(cg.readEntry(QString("URL-%1").arg(i), ""));
            if (!fileUrl.isValid()) break;
            OpenFileInfo *ofi = p->contains(fileUrl);
            if (ofi == NULL) {
                QString mimeType = KIO::NetAccess::mimetype(fileUrl, 0);
                ofi = p->create(mimeType, fileUrl);
            }
            ofi->setFlags(statusFlags);
            QString encoding = cg.readEntry(QString("Encoding-%1").arg(i), "");
            ofi->setProperty(OpenFileInfo::propertyEncoding, encoding);
        }
    }

    void writeConfig(OpenFileInfo::StatusFlags statusFlags, const QString& configGroupName) {
        KSharedConfig::Ptr config = KGlobal::config();
        KConfigGroup cg(config, configGroupName);
        QList<OpenFileInfo*> list = p->filteredItems(statusFlags);
        int i = 0;
        int max = statusFlags & OpenFileInfo::RecentlyUsed ? maxNumRecentlyUsedFiles : maxNumFiles;
        for (QList<OpenFileInfo*>::Iterator it = list.begin(); i < max && it != list.end(); ++it, ++i) {
            OpenFileInfo *ofi = *it;
            cg.writeEntry(QString("URL-%1").arg(i), ofi->url().prettyUrl());
            cg.writeEntry(QString("Encoding-%1").arg(i), ofi->property(OpenFileInfo::propertyEncoding));
        }
        config->sync();
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

OpenFileInfo *OpenFileInfoManager::create(const QString &mimeType, const KUrl & url)
{
    OpenFileInfo *result = contains(url);
    if (result == NULL) {
        result = new OpenFileInfo(this, mimeType, url);
        d->openFileInfoList << result;
    }
    return result;
}

OpenFileInfo *OpenFileInfoManager::contains(const KUrl&url) const
{
    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (ofi->url().equals(url))
            return ofi;
    }
    return NULL;
}

void OpenFileInfoManager::close(OpenFileInfo *openFileInfo)
{
    bool closing = false;

    OpenFileInfo *nextCurrent = (d->currentFileInfo == openFileInfo) ? NULL : d->currentFileInfo;

    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (ofi == openFileInfo) {
            d->openFileInfoList.erase(it);
            delete ofi;
            closing = true;
            break;
        } else if (nextCurrent == NULL && ofi->flags().testFlag(OpenFileInfo::Open))
            nextCurrent = ofi;
    }

    if (closing)
        emit listsChanged(OpenFileInfo::Open);
    setCurrentFile(nextCurrent);
}

OpenFileInfo *OpenFileInfoManager::currentFile() const
{
    return d->currentFileInfo;
}

void OpenFileInfoManager::setCurrentFile(OpenFileInfo *openFileInfo)
{
    bool hasChanged = d->currentFileInfo != openFileInfo;
    d->currentFileInfo = openFileInfo;
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
