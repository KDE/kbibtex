/*****************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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

#ifndef DOCKLET_FILESETTINGS_H
#define DOCKLET_FILESETTINGS_H

#include "filesettingswidget.h"

class BibTeXEditor;
class File;


/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class FileSettings : public FileSettingsWidget
{
    Q_OBJECT

public:
    explicit FileSettings(QWidget *parent);

    void setEditor(BibTeXEditor *editor);

private slots:
    void widgetsChangedSlot();

private:
    File *m_currentFile;
    BibTeXEditor *m_editor;
};

#endif // DOCKLET_FILESETTINGS_H
