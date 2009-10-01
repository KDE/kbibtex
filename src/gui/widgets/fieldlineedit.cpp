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

#include <QMenu>
#include <QSignalMapper>

#include "fieldlineedit.h"
#include "fileexporterbibtex.h"
#include "fileimporterbibtex.h"

using namespace KBibTeX::GUI::Widgets;

FieldLineEdit::FieldLineEdit(TypeFlags typeFlags, QWidget *parent)
        : MenuLineEdit(parent), m_typeFlags(typeFlags)
{
    setupMenu();

    setTypeFlag(Text);
}

FieldLineEdit::TypeFlag FieldLineEdit::setTypeFlag(FieldLineEdit::TypeFlag typeFlag)
{
    if (m_typeFlags&typeFlag) m_typeFlag = typeFlag;
    else if (m_typeFlags&Text) m_typeFlag = Text;
    else if (m_typeFlags&Reference) m_typeFlag = Reference;
    else if (m_typeFlags&Person) m_typeFlag = Person;
    else if (m_typeFlags&Keyword) m_typeFlag = Keyword;
    else if (m_typeFlags&Source) m_typeFlag = Source;

    updatePushButtonType();

    return m_typeFlag;
}

void FieldLineEdit::setValue(const KBibTeX::IO::Value& value)
{
    m_originalValue = value;
    loadValue(m_originalValue);
}

void FieldLineEdit::applyTo(KBibTeX::IO::Value& /*value*/)
{
    KBibTeX::IO::FileImporterBibTeX *importer = new KBibTeX::IO::FileImporterBibTeX();
    // TODO Write a QString to value function
    delete importer;
}

void FieldLineEdit::reset()
{
    loadValue(m_originalValue);
}

KIcon FieldLineEdit::iconForTypeFlag(TypeFlag typeFlag)
{
    switch (typeFlag) {
    case Text: return KIcon("draw-text");
    case Reference: return KIcon("emblem-symbolic-link");
    case Person: return KIcon("user-identity");
    case Keyword: return KIcon("edit-find");
    case Source: return KIcon("code-context");
    default: return KIcon();
    };
}


void FieldLineEdit::loadValue(const KBibTeX::IO::Value& value)
{
    QString text = "";
    m_incompleteRepresentation = true;

    if (value.size() > 0) {
        if (m_typeFlag == Source) {
            m_incompleteRepresentation = false;
            KBibTeX::IO::FileExporterBibTeX *exporter = new KBibTeX::IO::FileExporterBibTeX();
            text = exporter->valueToBibTeX(value);
            delete exporter;
        } else {
            m_incompleteRepresentation = value.size() > 1;
            const KBibTeX::IO::ValueItem *first = value.first();
            const KBibTeX::IO::PlainText *plainText = dynamic_cast<const KBibTeX::IO::PlainText*>(first);
            if (plainText != NULL) {
                text = plainText->text();
            } else {
                const KBibTeX::IO::Person *person = dynamic_cast<const KBibTeX::IO::Person*>(first);
                if (person != NULL) {
                    text = person->lastName();
                    QString temp = person->firstName();
                    if (!temp.isEmpty()) text.prepend(temp + " ");
                    temp = person->suffix();
                    if (!temp.isEmpty()) text.append(", " + temp);
                    temp = person->prefix();
                    if (!temp.isEmpty()) text.prepend(temp + " ");
                } else {
                    const KBibTeX::IO::MacroKey *macroKey = dynamic_cast<const KBibTeX::IO::MacroKey*>(first);
                    if (macroKey != NULL) {
                        text = macroKey->text();
                    }
                }
            }
        }
    }

    setReadOnly(m_incompleteRepresentation);
    setText(text);
}

void FieldLineEdit::setupMenu()
{
    m_menuTypes = new QMenu("Types", this);
    setMenu(m_menuTypes);

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

void FieldLineEdit::updatePushButtonType()
{
    setIcon(iconForTypeFlag(m_typeFlag));
    switch (m_typeFlag) {
    case Text: setButtonToolTip("Plain Text"); break;
    case Reference: setButtonToolTip("Reference"); break;
    case Person: setButtonToolTip("Person"); break;
    case Source: setButtonToolTip("Source Code"); break;
    default: setButtonToolTip(""); break;
    };
}

void FieldLineEdit::slotTypeChanged(int newTypeFlag)
{
    KBibTeX::IO::Value value = m_originalValue;
    if (!m_incompleteRepresentation)
        applyTo(value);

    m_typeFlag = (TypeFlag)newTypeFlag;
    updatePushButtonType();

    loadValue(value);
}
