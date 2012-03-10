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

#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSignalMapper>

#include <KLineEdit>
#include <KComboBox>
#include <KLocale>
#include <KPushButton>
#include <KAction>

#include "settingsidsuggestionseditor.h"
#include <QMenu>

class TokenWidget : public QGroupBox
{
protected:
    enum CaseChange {ccNoChange, ccToUpper, ccToLower};
    QGridLayout *gridLayout;
    QFormLayout *formLayout;

public:
    TokenWidget(QWidget *parent)
            : QGroupBox(parent) {
        gridLayout = new QGridLayout(this);
        formLayout = new QFormLayout();
        gridLayout->addLayout(formLayout, 0, 0, 4, 1);
        gridLayout->setColumnStretch(0, 100);
    }

    void addButtons(KPushButton *buttonUp, KPushButton *buttonDown, KPushButton *buttonRemove) {
        gridLayout->setColumnMinimumWidth(1, 32);
        gridLayout->setColumnStretch(1, 1);
        gridLayout->setColumnStretch(2, 1);

        gridLayout->addWidget(buttonUp, 0, 2, 1, 1);
        buttonUp->setParent(this);
        gridLayout->addWidget(buttonDown, 1, 2, 1, 1);
        buttonDown->setParent(this);
        gridLayout->addWidget(buttonRemove, 2, 2, 1, 1);
        buttonRemove->setParent(this);
    }

    virtual QString toString() const = 0;
};

class AuthorWidget : public TokenWidget
{
private:
    KComboBox *comboBoxWhichAuthor;
    KComboBox *comboBoxChangeCase;
    KLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

public:
    AuthorWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestions::Authors author, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Authors"));

        comboBoxWhichAuthor = new KComboBox(false, this);
        comboBoxWhichAuthor->addItem(i18n("All authors"), IdSuggestions::aAll);
        comboBoxWhichAuthor->addItem(i18n("Only first author"), IdSuggestions::aOnlyFirst);
        comboBoxWhichAuthor->addItem(i18n("All except first author"), IdSuggestions::aNotFirst);
        formLayout->addRow(i18n("Authors:"), comboBoxWhichAuthor);
        comboBoxWhichAuthor->setCurrentIndex(comboBoxWhichAuthor->findData(author));

        comboBoxChangeCase = new KComboBox(false, this);
        comboBoxChangeCase->addItem(i18n("No change"), ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), ccToLower);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        if (info.toLower)
            comboBoxChangeCase->setCurrentIndex(comboBoxChangeCase->findData(ccToLower));
        else if (info.toUpper)
            comboBoxChangeCase->setCurrentIndex(comboBoxChangeCase->findData(ccToUpper));
        else
            comboBoxChangeCase->setCurrentIndex(comboBoxChangeCase->findData(ccNoChange));

        lineEditTextInBetween = new KLineEdit(this);
        formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
        lineEditTextInBetween->setText(info.inBetween);

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(comboBoxWhichAuthor, SIGNAL(currentIndexChanged(int)), isew, SLOT(updatePreview()));
        connect(comboBoxChangeCase, SIGNAL(currentIndexChanged(int)), isew, SLOT(updatePreview()));
        connect(lineEditTextInBetween, SIGNAL(textEdited(QString)), isew, SLOT(updatePreview()));
        connect(spinBoxLength, SIGNAL(valueChanged(int)), isew, SLOT(updatePreview()));
    }

    QString toString() const {
        IdSuggestions::Authors author = (IdSuggestions::Authors)comboBoxWhichAuthor->itemData(comboBoxWhichAuthor->currentIndex()).toInt();
        QString result = author == IdSuggestions::aAll ? QLatin1String("A") : (author == IdSuggestions::aOnlyFirst ? QLatin1String("a") : QLatin1String("z"));

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        CaseChange caseChange = (CaseChange)comboBoxChangeCase->itemData(comboBoxChangeCase->currentIndex()).toInt();
        if (caseChange == ccToLower)
            result.append(QLatin1String("l"));
        else if (caseChange == ccToUpper)
            result.append(QLatin1String("u"));

        const QString text = lineEditTextInBetween->text();
        if (!text.isEmpty())
            result.append(QLatin1String("\"")).append(text);

        return result;
    }
};

class YearWidget : public TokenWidget
{
private:
    KComboBox *comboBoxDigits;

public:
    YearWidget(int digits, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Year"));

        comboBoxDigits = new KComboBox(false, this);
        comboBoxDigits->addItem(i18n("2 digits"), 2);
        comboBoxDigits->addItem(i18n("4 digits"), 4);
        formLayout->addRow(i18n("Digits:"), comboBoxDigits);
        comboBoxDigits->setCurrentIndex(comboBoxDigits->findData(digits));

        connect(comboBoxDigits, SIGNAL(currentIndexChanged(int)), isew, SLOT(updatePreview()));
    }

    QString toString() const {
        const int year = comboBoxDigits->itemData(comboBoxDigits->currentIndex()).toInt();
        QString result = year == 4 ? QLatin1String("Y") : QLatin1String("y");

        return result;
    }
};

class TitleWidget : public TokenWidget
{
private:
    QCheckBox *checkBoxRemoveSmallWords;
    KComboBox *comboBoxChangeCase;
    KLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

public:
    TitleWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, bool removeSmallWords, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Title"));

        checkBoxRemoveSmallWords = new QCheckBox(i18n("Remove"), this);
        formLayout->addRow(i18n("Small words:"), checkBoxRemoveSmallWords);
        checkBoxRemoveSmallWords->setChecked(removeSmallWords);

        comboBoxChangeCase = new KComboBox(false, this);
        comboBoxChangeCase->addItem(i18n("No change"), ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), ccToLower);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        if (info.toLower)
            comboBoxChangeCase->setCurrentIndex(comboBoxChangeCase->findData(ccToLower));
        else if (info.toUpper)
            comboBoxChangeCase->setCurrentIndex(comboBoxChangeCase->findData(ccToUpper));
        else
            comboBoxChangeCase->setCurrentIndex(comboBoxChangeCase->findData(ccNoChange));

        lineEditTextInBetween = new KLineEdit(this);
        formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
        lineEditTextInBetween->setText(info.inBetween);

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(checkBoxRemoveSmallWords, SIGNAL(toggled(bool)), isew, SLOT(updatePreview()));
        connect(comboBoxChangeCase, SIGNAL(currentIndexChanged(int)), isew, SLOT(updatePreview()));
        connect(lineEditTextInBetween, SIGNAL(textEdited(QString)), isew, SLOT(updatePreview()));
        connect(spinBoxLength, SIGNAL(valueChanged(int)), isew, SLOT(updatePreview()));
    }

    QString toString() const {
        QString result = checkBoxRemoveSmallWords->isChecked() ? QLatin1String("T") : QLatin1String("t");

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        CaseChange caseChange = (CaseChange)comboBoxChangeCase->itemData(comboBoxChangeCase->currentIndex()).toInt();
        if (caseChange == ccToLower)
            result.append(QLatin1String("l"));
        else if (caseChange == ccToUpper)
            result.append(QLatin1String("u"));

        const QString text = lineEditTextInBetween->text();
        if (!text.isEmpty())
            result.append(QLatin1String("\"")).append(text);

        return result;
    }
};

class TextWidget : public TokenWidget
{
private:
    KLineEdit *lineEditText;

public:
    TextWidget(const QString &text, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Text"));

        lineEditText = new KLineEdit(this);
        formLayout->addRow(i18n("Text:"), lineEditText);
        lineEditText->setText(text);

        connect(lineEditText, SIGNAL(textEdited(QString)), isew, SLOT(updatePreview()));
    }

    QString toString() const {
        QString result = QLatin1String("\"") + lineEditText->text();
        return result;
    }
};

class IdSuggestionsEditWidget::IdSuggestionsEditWidgetPrivate
{
private:
    IdSuggestionsEditWidget *p;
public:
    enum TokenType {ttTitle = 0, ttAuthor = 1, ttYear = 2, ttText = 3};

    QWidget *container;
    QBoxLayout *containerLayout;
    QList<TokenWidget*> widgetList;
    QLabel *labelPreview;
    KPushButton *buttonAddToken;
    const Entry *previewEntry;
    QSignalMapper *signalMapperRemove, *signalMapperMoveUp, *signalMapperMoveDown, *signalMapperAddMenu;

    IdSuggestionsEditWidgetPrivate(const Entry *pe, IdSuggestionsEditWidget *parent)
            : p(parent), previewEntry(pe) {
        // TODO
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        labelPreview = new QLabel(p);
        layout->addWidget(labelPreview, 0, 0, 1, 1);
        layout->setColumnStretch(0, 100);

        buttonAddToken = new KPushButton(KIcon("list-add"), i18n("Add"), p);
        layout->addWidget(buttonAddToken, 0, 1, 1, 1);
        layout->setColumnStretch(1, 1);
        QMenu *menuAddToken = new QMenu(buttonAddToken);
        signalMapperAddMenu = new QSignalMapper(p);
        buttonAddToken->setMenu(menuAddToken);
        QAction *action = menuAddToken->addAction(i18n("Title"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttTitle);
        action = menuAddToken->addAction(i18n("Author"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttAuthor);
        action = menuAddToken->addAction(i18n("Year"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttYear);
        action = menuAddToken->addAction(i18n("Text"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttText);

        QScrollArea *area = new QScrollArea(p);
        layout->addWidget(area, 1, 0, 1, 2);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        container = new QWidget(area);
        area->setWidget(container);
        area->setWidgetResizable(true);
        containerLayout = new QVBoxLayout(container);
        area->setMinimumSize(384, 256);

        signalMapperMoveUp = new QSignalMapper(p);
        connect(signalMapperMoveUp, SIGNAL(mapped(QWidget*)), p, SLOT(moveUpToken(QWidget*)));
        signalMapperMoveDown = new QSignalMapper(p);
        connect(signalMapperMoveDown, SIGNAL(mapped(QWidget*)), p, SLOT(moveDownToken(QWidget*)));
        signalMapperRemove = new QSignalMapper(p);
        connect(signalMapperRemove, SIGNAL(mapped(QWidget*)), p, SLOT(removeToken(QWidget*)));

        connect(signalMapperAddMenu, SIGNAL(mapped(int)), p, SLOT(addToken(int)));
    }

    void addManagementButtons(TokenWidget *tokenWidget) {
        if (tokenWidget != NULL) {
            KPushButton *buttonUp = new KPushButton(KIcon("go-up"), QLatin1String(""), tokenWidget);
            KPushButton *buttonDown = new KPushButton(KIcon("go-down"), QLatin1String(""), tokenWidget);
            KPushButton *buttonRemove = new KPushButton(KIcon("list-remove"), QLatin1String(""), tokenWidget);
            tokenWidget->addButtons(buttonUp, buttonDown, buttonRemove);
            connect(buttonUp, SIGNAL(clicked(bool)), signalMapperMoveUp, SLOT(map()));
            signalMapperMoveUp->setMapping(buttonUp, tokenWidget);
            connect(buttonDown, SIGNAL(clicked(bool)), signalMapperMoveDown, SLOT(map()));
            signalMapperMoveDown->setMapping(buttonDown, tokenWidget);
            connect(buttonRemove, SIGNAL(clicked(bool)), signalMapperRemove, SLOT(map()));
            signalMapperRemove->setMapping(buttonRemove, tokenWidget);
        }
    }

    void add(TokenType tokenType) {
        TokenWidget *tokenWidget = NULL;
        switch (tokenType) {
        case ttTitle: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = -1;
            info.toLower = info.toUpper = false;
            tokenWidget = new TitleWidget(info, true, p, container);
            widgetList << tokenWidget;
            containerLayout->addWidget(tokenWidget, 1);

        }
        break;
        case ttAuthor: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = -1;
            info.toLower = info.toUpper = false;
            tokenWidget = new AuthorWidget(info, aAll, p, container);
            widgetList << tokenWidget;
            containerLayout->addWidget(tokenWidget, 1);
        }
        break;
        case ttYear:
            tokenWidget = new YearWidget(4, p, container);
            widgetList << tokenWidget;
            containerLayout->addWidget(tokenWidget, 1);
            break;
        case ttText:
            tokenWidget = new TextWidget(QString(), p, container);
            widgetList << tokenWidget;
            containerLayout->addWidget(tokenWidget, 1);
        }

        addManagementButtons(tokenWidget);
    }

    void reset(const QString &formatString) {
        while (!widgetList.isEmpty())
            delete widgetList.takeFirst();

        QStringList tokenList = formatString.split(QLatin1Char('|'));
        foreach(const QString &token, tokenList) {
            TokenWidget *tokenWidget = NULL;

            if (token[0] == 'a' || token[0] == 'A' || token[0] == 'z') {
                IdSuggestions::Authors author = token[0] == 'a' ? aOnlyFirst : (token[0] == 'A' ? aAll : aNotFirst);
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new AuthorWidget(info, author, p, container);
                widgetList << tokenWidget;
                containerLayout->addWidget(tokenWidget, 1);
            } else if (token[0] == 'y') {
                tokenWidget = new YearWidget(2, p, container);
                widgetList << tokenWidget;
                containerLayout->addWidget(tokenWidget, 1);
            } else if (token[0] == 'Y') {
                tokenWidget = new YearWidget(4, p, container);
                widgetList << tokenWidget;
                containerLayout->addWidget(tokenWidget, 1);
            } else if (token[0] == 't' || token[0] == 'T') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new TitleWidget(info, token[0] == 'T', p, container);
                widgetList << tokenWidget;
                containerLayout->addWidget(tokenWidget, 1);
            } else if (token[0] == '"') {
                tokenWidget = new TextWidget(token.mid(1), p, container);
                widgetList << tokenWidget;
                containerLayout->addWidget(tokenWidget, 1);
            }

            addManagementButtons(tokenWidget);
        }

        p->updatePreview();
    }

    QString apply() {
        QStringList result;

        foreach(TokenWidget *widget, widgetList) {
            result << widget->toString();
        }

        return result.join(QLatin1String("|"));
    }
};

IdSuggestionsEditWidget::IdSuggestionsEditWidget(const Entry *previewEntry, QWidget* parent, Qt::WindowFlags f)
        : QWidget(parent, f), IdSuggestions(), d(new IdSuggestionsEditWidgetPrivate(previewEntry, this))
{
    d->setupGUI();
}

IdSuggestionsEditWidget::~IdSuggestionsEditWidget()
{
// TODO
}

void IdSuggestionsEditWidget::setFormatString(const QString& formatString)
{
    d->reset(formatString);
}

QString IdSuggestionsEditWidget::formatString() const
{
    return d->apply();
}

void IdSuggestionsEditWidget::updatePreview()
{
    const QString formatString = d->apply();
    d->labelPreview->setText(formatId(*d->previewEntry, formatString));
    d->labelPreview->setToolTip(i18n("<qt>Structure:<ul><li>%1</li></ul>Example: %2</qt>", formatStrToHuman(formatString).join(QLatin1String("</li><li>")), formatId(*d->previewEntry, formatString)));
}

void IdSuggestionsEditWidget::moveUpToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget*>(widget);
    int curPos = d->widgetList.indexOf(tokenWidget);
    if (curPos > 0) {
        d->widgetList.removeAt(curPos);
        d->containerLayout->removeWidget(tokenWidget);
        d->widgetList.insert(curPos - 1, tokenWidget);
        d->containerLayout->insertWidget(curPos - 1, tokenWidget, 1);
    }
}

void IdSuggestionsEditWidget::moveDownToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget*>(widget);
    int curPos = d->widgetList.indexOf(tokenWidget);
    if (curPos < d->widgetList.size() - 1) {
        d->widgetList.removeAt(curPos);
        d->containerLayout->removeWidget(tokenWidget);
        d->widgetList.insert(curPos + 1, tokenWidget);
        d->containerLayout->insertWidget(curPos + 1, tokenWidget, 1);
    }
}

void IdSuggestionsEditWidget::removeToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget*>(widget);
    d->widgetList.removeOne(tokenWidget);
    d->containerLayout->removeWidget(tokenWidget);
    tokenWidget->deleteLater();
}

void IdSuggestionsEditWidget::addToken(int cmd)
{
    d->add((IdSuggestionsEditWidgetPrivate::TokenType)cmd);
}

IdSuggestionsEditDialog::IdSuggestionsEditDialog(QWidget* parent, Qt::WFlags flags)
        : KDialog(parent, flags)
{
    setCaption(i18n("Edit Id Suggestion"));
    setButtons(KDialog::Ok | KDialog::Cancel);
}

IdSuggestionsEditDialog::~IdSuggestionsEditDialog()
{
    // TODO
}

QString IdSuggestionsEditDialog::editSuggestion(const Entry *previewEntry, const QString& suggestion, QWidget* parent)
{
    IdSuggestionsEditDialog dlg(parent);
    IdSuggestionsEditWidget widget(previewEntry, &dlg);
    dlg.setMainWidget(&widget);


    widget.setFormatString(suggestion);
    if (dlg.exec() == Accepted)
        return widget.formatString();

    /// Return unmodified original suggestion
    return suggestion;
}
