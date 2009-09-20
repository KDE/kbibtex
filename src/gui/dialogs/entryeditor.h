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

#ifndef KBIBTEX_GUI_DIALOGS_ENTRYEDITOR_H
#define KBIBTEX_GUI_DIALOGS_ENTRYEDITOR_H

#include <kbibtexgui_export.h>

#include "entryviewer.h"

namespace KBibTeX
{
namespace GUI {
namespace Dialogs {

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT EntryEditor : public EntryViewer
{
public:
    EntryEditor(KBibTeX::IO::Entry *entry, QWidget *parent);
};

}
}
}

#endif // KBIBTEX_GUI_DIALOGS_ENTRYEDITOR_H
