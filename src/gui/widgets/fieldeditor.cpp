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

#include "fieldeditor.h"
#include "fieldlineedit.h"

using namespace KBibTeX::GUI::Widgets;

class FieldEditor::FieldEditorPrivate
{
public:
    EditMode m_editMode;
    FieldLineEdit **m_widgets;
    KBibTeX::IO::Value m_originalValue;
};

FieldEditor::FieldEditor(EditMode editMode, QWidget *parent)
        : QStackedWidget(parent), d(new FieldEditorPrivate)
{
    d->m_editMode = editMode;
    d->m_widgets = new FieldLineEdit*[EditModeMax];

    d->m_widgets[SingleLine] = new FieldLineEdit(FieldLineEdit::Text | FieldLineEdit::Source, this);
    d->m_widgets[SingleLine]->setTypeFlag(FieldLineEdit::Source);
    addWidget(d->m_widgets[SingleLine]);

    setBackgroundRole(QPalette::Base);
}

FieldEditor::~FieldEditor()
{
    delete[] d->m_widgets;
}

void FieldEditor::setEditMode(EditMode editMode)
{
    d->m_editMode = editMode;
}

FieldEditor::EditMode FieldEditor::editMode()
{
    return d->m_editMode;
}

void FieldEditor::setValue(const KBibTeX::IO::Value& value)
{
    d->m_originalValue = value;
    d->m_widgets[0]->setValue(value);
}

void FieldEditor::applyTo(KBibTeX::IO::Value& /*value*/)
{
    // TODO
}

void FieldEditor::reset()
{
    // TODO
}
