/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "kbibtexgui_export.h"

#include <QWidget>
#include <QLayout>
#include <QDialog>
#include <QGroupBox>

#include "idsuggestions.h"
#include "entry.h"

class QFormLayout;
class QSpinBox;
class QLabel;
class QCheckBox;
class QPushButton;

class KLineEdit;
class KComboBox;

class IdSuggestionsEditWidget;
class RangeWidget;

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
    explicit IdSuggestionsEditDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
};

/**
 * @author Thomas Fischer
 */
class IdSuggestionsEditWidget : public QWidget, public IdSuggestions
{
    Q_OBJECT

public:
    explicit IdSuggestionsEditWidget(const Entry *previewEntry, QWidget *parent = nullptr, Qt::WindowFlags f = 0);
    ~IdSuggestionsEditWidget() override;

    void setFormatString(const QString &formatString);
    QString formatString() const;

public slots:
    void updatePreview();

private slots:
    void moveUpToken(QWidget *);
    void moveDownToken(QWidget *);
    void removeToken(QWidget *);
    void addToken(int);

private:
    class IdSuggestionsEditWidgetPrivate;
    IdSuggestionsEditWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
