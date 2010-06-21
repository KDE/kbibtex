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

#include <QMenu>
#include <QSignalMapper>
#include <QBuffer>

#include <KDebug>
#include <KMessageBox>
#include <KGlobalSettings>
#include <KLocale>

#include <file.h>
#include <entry.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include "fieldlistedit.h"

FieldListEdit::FieldListEdit(QWidget *parent)
        : QListWidget(parent)
{
    updateGUI();
// TODO
}

void FieldListEdit::setValue(const Value& value)
{
    m_originalValue = value;
    loadValue(m_originalValue);
}

void FieldListEdit::applyTo(Value& value) const
{
    value.clear();
    value = m_originalValue; // FIXME: just a dummy implementation
}

void FieldListEdit::reset()
{
    loadValue(m_originalValue);
}


void FieldListEdit::loadValue(const Value& value)
{
    Q_UNUSED(value)
    // TODO
}

void FieldListEdit::updateGUI()
{
    setFont(KGlobalSettings::generalFont());

    // TODO
}
