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

#include <QLayout>
#include <QApplication>
#include <QMenu>
#include <QSignalMapper>

#include <KPushButton>
#include <KLineEdit>

#include "fieldeditor.h"

using namespace KBibTeX::GUI::Widgets;

FieldEditor::FieldEditor(TypeFlags typeFlags, QWidget *parent)
        : QFrame(parent), m_typeFlags(typeFlags)
{
    if (m_typeFlags&Text) m_typeFlag = Text;
    else if (m_typeFlags&Reference) m_typeFlag = Reference;
    else if (m_typeFlags&Person) m_typeFlag = Person;

    setupUI();
    setupMenu();
}

void FieldEditor::setEditMode(EditMode editMode)
{
    m_editMode = editMode;
}

FieldEditor::EditMode FieldEditor::editMode()
{
    return m_editMode;
}

FieldEditor::TypeFlag FieldEditor::typeFlag()
{
    return m_typeFlag;
}

KIcon FieldEditor::iconForTypeFlag(TypeFlag typeFlag)
{
    switch (typeFlag) {
    case Text: return KIcon("draw-text");
    case Reference: return KIcon("emblem-symbolic-link");
    case Person: return KIcon("user-identity");
    case Source: return KIcon("code-context");
    default: return KIcon();
    };
}

void FieldEditor::setupUI()
{
    setObjectName("FieldEditor");
    setFrameShape(QFrame::StyledPanel);

    QHBoxLayout *hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(2);

    m_pushButtonType = new KPushButton(this);
    hLayout->addWidget(m_pushButtonType);
    m_pushButtonType->setObjectName("FieldEditorButton");
    updatePpushButtonType();

    m_lineEditText = new KLineEdit(this);
    hLayout->addWidget(m_lineEditText);
    m_lineEditText->setObjectName("FieldEditorText");
    m_lineEditText->setClearButtonShown(true);

    qApp->setStyleSheet("QFrame#FieldEditor { background-color: " + QPalette().color(QPalette::Base).name() + "; } KLineEdit#FieldEditorText { border-style: none; } KPushButton#FieldEditorButton { padding: 0px; margin-left:2px; margin-right:2px; text-align: left; width: " + QString::number(m_pushButtonType->height() + 1) + "; background-color: " + QPalette().color(QPalette::Base).name() + "; border-style: none; }");
}

void FieldEditor::setupMenu()
{
    m_menuTypes = new QMenu("Types", this);
    m_pushButtonType->setMenu(m_menuTypes);

    m_menuTypesSignalMapper = new QSignalMapper(this);
    connect(m_menuTypesSignalMapper, SIGNAL(mapped(int)), this, SLOT(slotTypeChanged(int)));

    if (m_typeFlags&Text) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Text), "Plain Text", m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Text);
    }
    if (m_typeFlags&Reference) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Reference), "Reference", m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Reference);
    }
    if (m_typeFlags&Person) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Person), "Person", m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Person);
    }
    if (m_typeFlags&Source) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Source), "Source Code", m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Source);
    }
}

void FieldEditor::updatePpushButtonType()
{
    m_pushButtonType->setIcon(iconForTypeFlag(m_typeFlag));
    switch (m_typeFlag) {
    case Text: m_pushButtonType->setToolTip("Plain Text"); break;
    case Reference: m_pushButtonType->setToolTip("Reference"); break;
    case Person: m_pushButtonType->setToolTip("Person"); break;
    case Source: m_pushButtonType->setToolTip("Source Code"); break;
    default: m_pushButtonType->setToolTip(""); break;
    };
}

void FieldEditor::slotTypeChanged(int newTypeFlag)
{
    m_typeFlag = (TypeFlag)newTypeFlag;
    updatePpushButtonType();
    emit typeChanged(m_typeFlag);
}
