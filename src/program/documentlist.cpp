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

#include <QStringListModel>

#include <KDebug>

#include "documentlist.h"

using namespace KBibTeX::Program;

DocumentList::DocumentList(QWidget *parent)
        : QListWidget(parent)
{
    // nothing
}

DocumentList::~DocumentList()
{
    //nothing
}

void DocumentList::addOpen(KUrl &url)
{
    QList<QListWidgetItem *> matches = findItems(url.prettyUrl(), Qt::MatchExactly);
    if (matches.isEmpty())
        addItem(url.prettyUrl());

    kDebug() << "DocumentList::addOpen " << url.prettyUrl() << endl;
    emit open(url);
    // TODO
}
