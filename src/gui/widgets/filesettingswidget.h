/*****************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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

#ifndef KBIBTEX_GUI_FILESETTINGSWIDGET_H
#define KBIBTEX_GUI_FILESETTINGSWIDGET_H

#include <QWidget>

#include <Value>

#include "kbibtexgui_export.h"

class QCheckBox;
class QComboBox;

class File;

/**
 * @author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT FileSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileSettingsWidget(QWidget *parent);

    void loadProperties(File *file);
    void saveProperties(File *file);

public slots:
    void resetToLoadedProperties();
    void applyProperties();
    void resetToDefaults();

signals:
    void widgetsChanged();

private:
    QComboBox *m_comboBoxEncodings;
    QComboBox *m_comboBoxStringDelimiters;
    QComboBox *m_comboBoxQuoteComment;
    QComboBox *m_comboBoxKeywordCasing;
    QCheckBox *m_checkBoxProtectCasing;
    QComboBox *m_comboBoxPersonNameFormatting;
    QComboBox *m_comboBoxListSeparator;

    const Person dummyPerson;
    File *m_file;

    void setupGUI();
};

#endif // KBIBTEX_GUI_FILESETTINGSWIDGET_H
