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

#ifndef KBIBTEX_GUI_DIALOGS_MACROEDITOR_H
#define KBIBTEX_GUI_DIALOGS_MACROEDITOR_H

#include <kbibtexgui_export.h>

#include <QTabWidget>

class Macro;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT MacroEditor : public QTabWidget
{
    Q_OBJECT
public:
    MacroEditor(const Macro *macro, QWidget *parent);
    MacroEditor(Macro *macro, QWidget *parent);
    void setReadOnly(bool isReadOnly = true);

public slots:
    void apply();
    void reset();

private slots:
    void tabChanged();

private:
    class MacroEditorPrivate;
    MacroEditorPrivate *d;
};

#endif // KBIBTEX_GUI_DIALOGS_MACROEDITOR_H
