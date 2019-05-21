/*****************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#ifndef KBIBTEX_PROGRAM_DOCKLET_FILESETTINGS_H
#define KBIBTEX_PROGRAM_DOCKLET_FILESETTINGS_H

#include <widgets/FileSettingsWidget>
#include "openfileinfo.h"

class FileView;
class File;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class FileSettings : public FileSettingsWidget
{
    Q_OBJECT

public:
    explicit FileSettings(QWidget *parent);

    void setFileView(FileView *fileView);

private slots:
    void widgetsChangedSlot();
    void currentFileChangedSlot();
    void flagsChangedSlot(const OpenFileInfo::StatusFlags statusFlags);

private:
    FileView *m_fileView;
};

#endif // KBIBTEX_PROGRAM_DOCKLET_FILESETTINGS_H
