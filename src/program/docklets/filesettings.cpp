/*****************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#include "filesettings.h"

#include <QFormLayout>
#include <QCheckBox>

#include <KComboBox>
#include <KLocale>

#include "preferences.h"
#include "guihelper.h"
#include "italictextitemmodel.h"
#include "bibtexeditor.h"
#include "bibtexfilemodel.h"
#include "value.h"
#include "iconvlatex.h"
#include "file.h"

FileSettings::FileSettings(QWidget *parent)
        : FileSettingsWidget(parent), m_currentFile(NULL), m_editor(NULL)
{
    setEnabled(false);

    connect(this, SIGNAL(widgetsChanged()), this, SLOT(widgetsChangedSlot()));
}

void FileSettings::setEditor(BibTeXEditor *editor)
{
    m_currentFile = NULL;
    m_editor = editor;

    if (m_editor != NULL && m_editor->bibTeXModel() != NULL) {
        m_currentFile = m_editor->bibTeXModel()->bibTeXFile();
        if (m_currentFile != NULL)
            loadProperties(m_currentFile);
        setEnabled(true);
    } else
        setEnabled(false);
}

void FileSettings::widgetsChangedSlot()
{
    if (m_currentFile != NULL) {
        saveProperties(m_currentFile);
        /// Notify main editor about change it its data
        m_editor->externalModification();
    }
}
