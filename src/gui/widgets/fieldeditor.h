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

#include <QFrame>

#include <KIcon>

class QMenu;
class QSignalMapper;
class KPushButton;
class KLineEdit;

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FieldEditor : public QFrame
{
    Q_OBJECT
    Q_ENUMS(EditMode)
    Q_PROPERTY(EditMode editMode READ editMode WRITE setEditMode)

public:
    enum TypeFlag {
        Text = 0x1,
        Reference = 0x2,
        Person = 0x4,
        Source = 0x8
    };
    Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

    enum EditMode {
        SingleLine
    };

    FieldEditor(TypeFlags typeFlags, QWidget *parent);

    void setEditMode(EditMode);
    EditMode editMode();

    TypeFlag typeFlag();
    KIcon iconForTypeFlag(TypeFlag typeFlag);

signals:
    void typeChanged(TypeFlag);

protected:
    KPushButton *m_pushButtonType;
    KLineEdit *m_lineEditText;

    TypeFlags m_typeFlags;
    TypeFlag m_typeFlag;
    EditMode m_editMode;

    void setupUI();

private:
    QMenu *m_menuTypes;
    QSignalMapper *m_menuTypesSignalMapper;

    void setupMenu();
    void updatePpushButtonType();

private slots:
    void slotTypeChanged(int);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FieldEditor::TypeFlags)

}
}
}

#endif // KBIBTEX_GUI_FIELDEDITOR_H
