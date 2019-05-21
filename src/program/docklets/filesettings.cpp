/*****************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#include "filesettings.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>

#include <KLocalizedString>

#include <Preferences>
#include <Value>
#include <File>
#include <GUIHelper>
#include <ItalicTextItemModel>
#include <file/FileView>
#include <models/FileModel>

FileSettings::FileSettings(QWidget *parent)
        : FileSettingsWidget(parent), m_fileView(nullptr)
{
    setEnabled(false);

    connect(this, &FileSettings::widgetsChanged, this, &FileSettings::widgetsChangedSlot);

    connect(&OpenFileInfoManager::instance(), &OpenFileInfoManager::currentChanged, this, &FileSettings::currentFileChangedSlot);
    /// Monitoring file flag changes to get notified of
    /// "Save As" operations where the file settings
    /// may get changed (requires a reload of properties)
    connect(&OpenFileInfoManager::instance(), &OpenFileInfoManager::flagsChanged, this, &FileSettings::flagsChangedSlot);
}

void FileSettings::setFileView(FileView *fileView)
{
    m_fileView = fileView;
    currentFileChangedSlot();
}

void FileSettings::widgetsChangedSlot()
{
    File *file = m_fileView != nullptr && m_fileView->fileModel() != nullptr ? m_fileView->fileModel()->bibliographyFile() : nullptr;
    if (file != nullptr) {
        saveProperties(file);
        /// Notify main view about change it its data
        m_fileView->externalModification();
    }
}

void FileSettings::currentFileChangedSlot() {
    File *file = m_fileView != nullptr && m_fileView->fileModel() != nullptr ? m_fileView->fileModel()->bibliographyFile() : nullptr;
    loadProperties(file);
    setEnabled(file != nullptr);
}

void FileSettings::flagsChangedSlot(const OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(OpenFileInfo::Open)) {
        File *file = m_fileView != nullptr && m_fileView->fileModel() != nullptr ? m_fileView->fileModel()->bibliographyFile() : nullptr;
        loadProperties(file);
    }
}
