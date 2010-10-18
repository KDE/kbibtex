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
#include <QDir>

#include <KDebug>

#include <kbibtexnamespace.h>
#include <entry.h>
#include "fileinfo.h"

static const QRegExp regExpFileExtension = QRegExp("\\.[a-z0-9]{1,4}", Qt::CaseInsensitive);

FileInfo::FileInfo()
{
    // TODO
}

QList<KUrl> FileInfo::entryUrls(const Entry *entry, const KUrl &baseUrl)
{
    QList<KUrl> result;
    if (entry == NULL || entry->isEmpty())
        return result;

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        QString plainText = PlainTextValue::text(*it, NULL);

        int pos = -1;
        while ((pos = KBibTeX::urlRegExp.indexIn(plainText, pos + 1)) != -1) {
            KUrl url(KBibTeX::urlRegExp.cap(0));
            if (url.isValid() && (!url.isLocalFile() || QFileInfo(KBibTeX::urlRegExp.cap(0)).exists()))
                result << url;
        }

        pos = -1;
        while ((pos = KBibTeX::doiRegExp.indexIn(plainText, pos + 1)) != -1) {
            KUrl url(KBibTeX::doiUrlPrefix + KBibTeX::doiRegExp.cap(0).replace("\\", ""));
            if (url.isValid())
                result << url;
        }

        QStringList fileList = plainText.split(QRegExp("[;]?[ \t\n\r]+"));
        for (QStringList::ConstIterator ptit = fileList.constBegin(); ptit != fileList.constEnd();++ptit)
            if (QFileInfo(*ptit).exists() && ((*ptit).contains(QDir::separator()) || (*ptit).contains(regExpFileExtension)))
                result << KUrl(*ptit);
    }

    if (baseUrl.isValid() && baseUrl.isLocalFile()) {
        KUrl url = baseUrl;
        url.setFileName(entry->id() + ".pdf"); // FIXME: Test more extensions
        if (QFileInfo(url.path()).exists())
            result << url;

        url = baseUrl;
        QString basename = baseUrl.fileName().replace(QRegExp("\\.[^.]{2,5}$"), "");
        url.setPath(url.path().replace(baseUrl.fileName(), basename) + QDir::separator() + basename);
        url.setFileName(entry->id() + ".pdf"); // FIXME: Test more extensions
        if (QFileInfo(url.path()).exists())
            result << url;
    }

    return result;
}
