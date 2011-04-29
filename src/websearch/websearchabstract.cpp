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

#include <KStandardDirs>
#include <kio/netaccess.h>

#include "websearchabstract.h"

const QString WebSearchAbstract::queryKeyFreeText = QLatin1String("free");
const QString WebSearchAbstract::queryKeyTitle = QLatin1String("title");
const QString WebSearchAbstract::queryKeyAuthor = QLatin1String("author");
const QString WebSearchAbstract::queryKeyYear = QLatin1String("year");

const int WebSearchAbstract::resultNoError = 0;
const int WebSearchAbstract::resultCancelled = 0; /// may get redefined in the future!
const int WebSearchAbstract::resultUnspecifiedError = 1;

KIcon WebSearchAbstract::icon() const
{
    QString fileName = favIconUrl();
    fileName = fileName.replace(QRegExp("[^-a-z0-9_]", Qt::CaseInsensitive), "");
    fileName.prepend(KStandardDirs::locateLocal("cache", "favicons/"));

    if (!QFileInfo(fileName).exists()) {
        if (!KIO::NetAccess::file_copy(KUrl(favIconUrl()), KUrl(fileName), NULL))
            return KIcon();
    }

    return KIcon(fileName);
}

QStringList WebSearchAbstract::splitRespectingQuotationMarks(const QString &text)
{
    int p1 = 0, p2, max = text.length();
    QStringList result;

    while (p1 < max) {
        while (text[p1] == ' ') ++p1;
        p2 = p1;
        if (text[p2] == '"') {
            ++p2;
            while (p2 < max && text[p2] != '"')  ++p2;
        } else
            while (p2 < max && text[p2] != ' ') ++p2;
        result << text.mid(p1, p2 - p1 + 1);
        p1 = p2 + 1;
    }
    return result;
}
