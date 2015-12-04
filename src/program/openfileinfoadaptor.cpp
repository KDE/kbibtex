/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                      2015 by Shunsuke Shimizu <grafi@grafi.jp>          *
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

#include "openfileinfoadaptor.h"

#include <KDebug>
#include <kparts/part.h>

#include "openfileinfo.h"
#include "mdiwidget.h"
#include "partwidget.h"


OpenFileInfoAdaptor::OpenFileInfoAdaptor(OpenFileInfo *p, OpenFileInfoManager *pm)
    : QDBusAbstractAdaptor(p), ofi(p), mdi(qobject_cast<MDIWidget *>(pm->parent()))
{
    QString objectPath = QLatin1String("/FileManager/") + QString::number(OpenFileInfoManagerAdaptor::openFileInfoToFileId(ofi));
    bool registerObjectResult = QDBusConnection::sessionBus().registerObject(objectPath, ofi);
    if (!registerObjectResult)
        kWarning() << "Failed to register a fileinfo to the DBus session bus";
}

bool OpenFileInfoAdaptor::flags() const
{
    return ofi->flags();
}

QString OpenFileInfoAdaptor::url() const
{
    return ofi->url().url();
}

bool OpenFileInfoAdaptor::isModified() const
{
    return ofi->isModified();
}

bool OpenFileInfoAdaptor::save() const
{
    return ofi->save();
}

void OpenFileInfoAdaptor::setFlags(uchar flags)
{
    ofi->setFlags(static_cast<OpenFileInfo::StatusFlag>(flags));
}

int OpenFileInfoAdaptor::documentId()
{
    KParts::ReadOnlyPart *part = ofi->part(mdi);
    if (part == NULL) return PartWidget::DocumentIdInvalid;
    PartWidget *w = qobject_cast<PartWidget *>(part->widget());
    if (w == NULL) return PartWidget::DocumentIdInvalid;
    return w->documentId();
}


OpenFileInfoManagerAdaptor::OpenFileInfoManagerAdaptor(OpenFileInfoManager *p)
    : QDBusAbstractAdaptor(p), ofim(p)
{
    QString objectPath = QLatin1String("/FileManager");
    bool registerObjectResult = QDBusConnection::sessionBus().registerObject(objectPath, ofim);
    if (!registerObjectResult)
        kWarning() << "Failed to register the filemanager to the DBus session bus";
}


uchar OpenFileInfoManagerAdaptor::FlagOpen() const
{
    return OpenFileInfo::Open;
}

uchar OpenFileInfoManagerAdaptor::FlagRecentlyUsed() const
{
    return OpenFileInfo::RecentlyUsed;
}

uchar OpenFileInfoManagerAdaptor::FlagFavorite() const
{
    return OpenFileInfo::Favorite;
}

uchar OpenFileInfoManagerAdaptor::FlagHasName() const
{
    return OpenFileInfo::HasName;
}


int OpenFileInfoManagerAdaptor::openFileInfoToFileId(OpenFileInfo *ofi)
{
    if (ofi)
        return ofi->fileId();
    else
        return OpenFileInfo::FileIdInvalid;
}

OpenFileInfo *OpenFileInfoManagerAdaptor::fileIdToOpenFileInfo(int fileId) const
{
    OpenFileInfoManager::OpenFileInfoList ofil = ofim->filteredItems(0, 0);
    for (OpenFileInfoManager::OpenFileInfoList::Iterator it = ofil.begin(); it != ofil.end(); ++it) {
        if ((*it)->fileId() == fileId) return *it;
    }
    return 0;
}


int OpenFileInfoManagerAdaptor::currentFileId() const
{
    return openFileInfoToFileId(ofim->currentFile());
}

void OpenFileInfoManagerAdaptor::setCurrentFileId(int fileId)
{
    OpenFileInfoManager::OpenFileInfoList ofil = ofim->filteredItems(0, 0);
    OpenFileInfo *ofi = fileIdToOpenFileInfo(fileId);
    if (ofi)
        ofim->setCurrentFile(ofi);
}

int OpenFileInfoManagerAdaptor::open(const QString &url)
{
    OpenFileInfo *ofi = 0;
    KUrl kurl = KUrl(url);
    if (kurl.isValid())
        ofi = ofim->open(kurl);
    return openFileInfoToFileId(ofi);
}

bool OpenFileInfoManagerAdaptor::close(int fileId)
{
    OpenFileInfoManager::OpenFileInfoList ofil = ofim->filteredItems(0, 0);
    OpenFileInfo *ofi = fileIdToOpenFileInfo(fileId);
    if (ofi)
        return ofim->close(ofi);
    else
        return false;
}
