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

#include <QGridLayout>
#include <QSpacerItem>
#include <QLabel>

#include "fieldwidget.h"
#include "fieldeditor.h"

using namespace KBibTeX::GUI::Widgets;

class FieldWidget::FieldWidgetPrivate
{
public:
    FieldWidget *p;
    QLabel *label;
    FieldEditor *editor;

    FieldWidgetPrivate(FieldWidget *parent)
            : p(parent) {
        QGridLayout *layout = new QGridLayout(p);
        label = new QLabel("test", p);
        layout->addWidget(label, 0, 0, 1, 2);
        QSpacerItem *space = new QSpacerItem(10, 10);
        layout->addItem(space, 1, 0);
        editor = new FieldEditor(FieldEditor::SingleLine, p);
        layout->addWidget(editor, 1, 1);
    }
};

FieldWidget::FieldWidget(QWidget* parent)
        : QWidget(parent), d(new FieldWidgetPrivate(this))
{
    // TODO
}
