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

#include <QFileInfo>

#include <KDebug>

#include <entry.h>
#include "fileinfo.h"

static const QRegExp urlRegExp("(http|s?ftp|webdav|file)s?://[^ {}\"]+", Qt::CaseInsensitive);
static const QRegExp doiRegExp("10\\.\\d{4}/[-a-z0-9.()_:]+", Qt::CaseInsensitive);
static const QString doiUrlPrefix = QLatin1String("http://dx.doi.org/");

FileInfo::FileInfo()
{
    // TODO
}

QList<KUrl> FileInfo::entryUrls(const Entry *entry)
{
    QList<KUrl> result;
    if (entry == NULL || entry->isEmpty())
        return result;

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        QString plainText = PlainTextValue::text(*it, NULL);

        int pos = -1;
        while ((pos = urlRegExp.indexIn(plainText, pos + 1)) != -1) {
            KUrl url(urlRegExp.cap(0));
            if (url.isValid() && (!url.isLocalFile() || QFileInfo(urlRegExp.cap(0)).exists()))
                result << url;
        }

        pos = -1;
        while ((pos = doiRegExp.indexIn(plainText, pos + 1)) != -1) {
            KUrl url(doiUrlPrefix + urlRegExp.cap(0));
            if (url.isValid())
                result << url;
        }

    }

    return result;
}
