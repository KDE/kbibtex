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
#ifndef KBIBTEX_GUI_FIELDLINEEDIT_H
#define KBIBTEX_GUI_FIELDLINEEDIT_H

#include <kbibtexgui_export.h>

#include <KIcon>

#include "value.h"
#include "menulineedit.h"

class QMenu;
class QSignalMapper;

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class FieldLineEdit : public MenuLineEdit
{
    Q_OBJECT

public:
    enum TypeFlag {
        Text = 0x1,
        Reference = 0x2,
        Person = 0x4,
        Keyword = 0x8,
        Source = 0x100
    };
    Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

    FieldLineEdit(TypeFlags typeFlags, QWidget *parent = NULL);

    TypeFlag setTypeFlag(TypeFlag typeFlag);

    void setValue(const KBibTeX::IO::Value& value);
    void applyTo(KBibTeX::IO::Value& value) const;

public slots:
    void reset();

protected:

    TypeFlags m_typeFlags;
    TypeFlag m_typeFlag;

    KBibTeX::IO::Value m_originalValue;

    KIcon iconForTypeFlag(TypeFlag typeFlag);

    void loadValue(const KBibTeX::IO::Value& value);

private:
    bool m_incompleteRepresentation;

    QMenu *m_menuTypes;
    QSignalMapper *m_menuTypesSignalMapper;

    void setupMenu();
    void updateGUI();

private slots:
    void slotTypeChanged(int);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FieldLineEdit::TypeFlags)

}
}
}

#endif // KBIBTEX_GUI_FIELDLINEEDIT_H
