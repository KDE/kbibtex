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
#ifndef KBIBTEX_GUI_FIELDLINEEDIT_H
#define KBIBTEX_GUI_FIELDLINEEDIT_H

#include <kbibtexgui_export.h>

#include <KIcon>

#include <value.h>
#include <menulineedit.h>
#include <kbibtexnamespace.h>

class QMenu;
class QSignalMapper;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FieldLineEdit : public MenuLineEdit
{
    Q_OBJECT

public:

    FieldLineEdit(KBibTeX::TypeFlags typeFlags = KBibTeX::tfSource, bool isMultiLine = false, QWidget *parent = NULL);

    KBibTeX::TypeFlag typeFlag();
    KBibTeX::TypeFlag setTypeFlag(KBibTeX::TypeFlag typeFlag);
    KBibTeX::TypeFlag setTypeFlags(KBibTeX::TypeFlags typeFlags);

    void setValue(const Value& value);
    void applyTo(Value& value) const;

public slots:
    void reset();

protected:

    KBibTeX::TypeFlags m_typeFlags;
    KBibTeX::TypeFlag m_typeFlag;

    Value m_originalValue;

    KIcon iconForTypeFlag(KBibTeX::TypeFlag typeFlag);

    void loadValue(const Value& value);

private:
    bool m_incompleteRepresentation;

    QMenu *m_menuTypes;
    QSignalMapper *m_menuTypesSignalMapper;

    void setupMenu();
    void updateGUI();

private slots:
    void slotTypeChanged(int);
};

#endif // KBIBTEX_GUI_FIELDLINEEDIT_H
