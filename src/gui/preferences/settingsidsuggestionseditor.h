/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
#define KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H

#include <QWidget>
#include <QLayout>
#include <QDialog>
#include <QGroupBox>

#include "kbibtexgui_export.h"

class Entry;
class TokenWidget;

/**
 * @author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT IdSuggestionsEditDialog : public QDialog
{
    Q_OBJECT

public:
    ~IdSuggestionsEditDialog() override;

    static QString editSuggestion(const Entry *previewEntry, const QString &suggestion, QWidget *parent);

protected:
    explicit IdSuggestionsEditDialog(QWidget *parent = nullptr);
};

/**
 * @author Thomas Fischer
 */
class IdSuggestionsEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IdSuggestionsEditWidget(const Entry *previewEntry, QWidget *parent = nullptr);
    ~IdSuggestionsEditWidget() override;

    void setFormatString(const QString &formatString);
    QString formatString() const;

public slots:
    void updatePreview();

private:
    class IdSuggestionsEditWidgetPrivate;
    IdSuggestionsEditWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
