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
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#include <QFormLayout>
#include <QCheckBox>

#include <KComboBox>
#include <KLocale>

#include "preferences.h"
#include "guihelper.h"
#include "italictextitemmodel.h"
#include "bibtexfileview.h"
#include "bibtexfilemodel.h"
#include "value.h"
#include "iconvlatex.h"
#include "file.h"
#include "filesettings.h"

FileSettings::FileSettings(QWidget *parent)
        : FileSettingsWidget(parent)
{
    m_currentFile = NULL;
    setEnabled(false);

    connect(this, SIGNAL(widgetsChanged()), this, SLOT(widgetsChangedSlot()));
}

void FileSettings::setEditor(BibTeXFileView *fileView)
{
    m_currentFile = NULL;

    if (fileView != NULL && fileView->bibTeXModel() != NULL) {
        m_currentFile = fileView->bibTeXModel()->bibTeXFile();
        if (m_currentFile != NULL)
            loadProperties(m_currentFile);
        setEnabled(true);
    } else
        setEnabled(false);
}

void FileSettings::widgetsChangedSlot()
{
    if (m_currentFile != NULL)
        saveProperties(m_currentFile);
}
