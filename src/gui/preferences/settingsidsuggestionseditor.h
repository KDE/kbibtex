/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#ifndef KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
#define KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H

#include "kbibtexgui_export.h"

#include <QWidget>
#include <QLayout>

#include <KDialog>

#include "idsuggestions.h"
#include "entry.h"


/**
 * @author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT IdSuggestionsEditDialog : public KDialog
{
    Q_OBJECT

public:
    virtual ~IdSuggestionsEditDialog();

    static QString editSuggestion(const Entry *previewEntry, const QString &suggestion, QWidget* parent);
protected:
    explicit IdSuggestionsEditDialog(QWidget* parent = 0, Qt::WFlags flags = 0);
};

class IdSuggestionsEditWidget : public QWidget, public IdSuggestions
{
    Q_OBJECT

public:
    explicit IdSuggestionsEditWidget(const Entry *previewEntry, QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~IdSuggestionsEditWidget();

    void setFormatString(const QString &formatString);
    QString formatString() const;

private slots:
    void updatePreview();

private:
    class IdSuggestionsEditWidgetPrivate;
    IdSuggestionsEditWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
