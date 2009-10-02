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
#ifndef KBIBTEX_GUI_FIELDEDITOR_H
#define KBIBTEX_GUI_FIELDEDITOR_H

#include <kbibtexgui_export.h>

#include <QStackedWidget>

#include <value.h>

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FieldEditor : public QStackedWidget
{
    Q_OBJECT
    Q_ENUMS(EditMode)
    Q_PROPERTY(EditMode editMode READ editMode WRITE setEditMode)

public:
    enum EditMode {
        SingleLine = 0, MultiLine = 1, List = 2, SourceCode = 3, EditModeMax = 4
    };

    FieldEditor(EditMode editMode = SingleLine, QWidget *parent = 0);
    ~FieldEditor();

    void setEditMode(EditMode);
    EditMode editMode();

    void setValue(const KBibTeX::IO::Value& value);
    void applyTo(KBibTeX::IO::Value& value);

public slots:
    void reset();

private:
    class FieldEditorPrivate;
    FieldEditorPrivate * const d;
};

}
}
}

#endif // KBIBTEX_GUI_FIELDEDITOR_H
