/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
#define KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H

#include "kbibtexgui_export.h"

#include <QWidget>
#include <QLayout>
#include <QGroupBox>

#include <KDialog>

#include "idsuggestions.h"
#include "entry.h"

class QFormLayout;
class QSpinBox;
class QLabel;
class QCheckBox;

class KLineEdit;
class KComboBox;

class QxtSpanSlider;

class IdSuggestionsEditWidget;

/**
 * @author Thomas Fischer
 */
class TokenWidget : public QGroupBox
{
    Q_OBJECT

protected:
    QGridLayout *gridLayout;
    QFormLayout *formLayout;

public:
    explicit TokenWidget(QWidget *parent);

    void addButtons(KPushButton *buttonUp, KPushButton *buttonDown, KPushButton *buttonRemove);

    virtual QString toString() const = 0;
};

/**
 * @author Thomas Fischer
 */
class AuthorWidget : public TokenWidget
{
    Q_OBJECT

private:
    QxtSpanSlider *spanSliderAuthor;
    QCheckBox *checkBoxLastAuthor;
    QLabel *labelAuthorRange;
    KComboBox *comboBoxChangeCase;
    KLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

private slots:
    void updateRangeLabel();

public:
    AuthorWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestionsEditWidget *isew, QWidget *parent);

    QString toString() const;
};

/**
 * @author Thomas Fischer
 */
class TitleWidget : public TokenWidget
{
    Q_OBJECT

private:
    QxtSpanSlider *spanSliderWords;
    QLabel *labelWordsRange;
    QCheckBox *checkBoxRemoveSmallWords;
    KComboBox *comboBoxChangeCase;
    KLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

private slots:
    void updateRangeLabel();

public:
    TitleWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, bool removeSmallWords, IdSuggestionsEditWidget *isew, QWidget *parent);

    QString toString() const;
};


/**
 * @author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT IdSuggestionsEditDialog : public KDialog
{
    Q_OBJECT

public:
    virtual ~IdSuggestionsEditDialog();

    static QString editSuggestion(const Entry *previewEntry, const QString &suggestion, QWidget *parent);
protected:
    explicit IdSuggestionsEditDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
};

class IdSuggestionsEditWidget : public QWidget, public IdSuggestions
{
    Q_OBJECT

public:
    explicit IdSuggestionsEditWidget(const Entry *previewEntry, QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual ~IdSuggestionsEditWidget();

    void setFormatString(const QString &formatString);
    QString formatString() const;

private slots:
    void updatePreview();
    void moveUpToken(QWidget *);
    void moveDownToken(QWidget *);
    void removeToken(QWidget *);
    void addToken(int);

private:
    class IdSuggestionsEditWidgetPrivate;
    IdSuggestionsEditWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSIDSUGGESTIONSEDITOR_H
