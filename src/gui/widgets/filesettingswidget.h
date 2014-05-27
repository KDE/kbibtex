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

#ifndef GUI_FILESETTINGSWIDGET_H
#define GUI_FILESETTINGSWIDGET_H

#include "kbibtexgui_export.h"

#include <QWidget>

#include "value.h"

class QCheckBox;

class KComboBox;

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
    void loadProperties();
    void saveProperties();
    void resetToDefaults();

signals:
    void widgetsChanged();

private:
    KComboBox *m_comboBoxEncodings;
    KComboBox *m_comboBoxStringDelimiters;
    KComboBox *m_comboBoxQuoteComment;
    KComboBox *m_comboBoxKeywordCasing;
    QCheckBox *m_checkBoxProtectCasing;
    KComboBox *m_comboBoxPersonNameFormatting;
    KComboBox *m_comboBoxListSeparator;

    const Person dummyPerson;
    File *m_file;

    void setupGUI();
};

#endif // GUI_FILESETTINGSWIDGET_H
