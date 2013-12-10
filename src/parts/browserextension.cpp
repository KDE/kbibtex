/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <kaction.h>

#include "part.h"

KBibTeXBrowserExtension::KBibTeXBrowserExtension(KBibTeXPart *p)
        : KParts::BrowserExtension(p), part(p)
{
    setObjectName("kbibtexpartbrowserextension");
//     connect( part->view, SIGNAL( selectionChanged( bool ) ), SLOT( onSelectionChanged( bool ) ) );
}

/*
void KBibTeXBrowserExtension::copy()
{
    part->view->copy();
}
*/

/*
void KBibTeXBrowserExtension::onSelectionChanged( bool HasSelection )
{
    emit enableAction( "copy", HasSelection );
}
*/

void KBibTeXBrowserExtension::saveState(QDataStream &stream)
{
    KParts::BrowserExtension::saveState(stream);

    // TODO
}


void KBibTeXBrowserExtension::restoreState(QDataStream &stream)
{
    KParts::BrowserExtension::restoreState(stream);

    // TODO

    part->fitActionSettings();
}

#include "browserextension.moc"
