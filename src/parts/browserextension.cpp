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

#include "browserextension.h"

#include <KParts/ReadOnlyPart>
#include <KIconLoader>

struct BrowserExtension::Private
{
    KParts::ReadOnlyPart *part;
};

BrowserExtension::BrowserExtension(KParts::ReadOnlyPart *part)
    : KParts::BrowserExtension(part), d(new Private)
{
    d->part = part;
    const QString iconPath = KIconLoader::global()->iconPath("text-x-bibtex", KIconLoader::SizeSmall);
    emit setIconUrl(QUrl::fromLocalFile(iconPath));
}

BrowserExtension::~BrowserExtension()
{
    delete d;
}
