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
#include <QMenu>

#include <KLineEdit>
#include <KComboBox>
#include <KLocale>
#include <KPushButton>
#include <KAction>

#include "settingsidsuggestionseditor.h"

class TokenWidget : public QGroupBox
{
protected:
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
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex((int)info.caseChange); /// enum has numbers assigned to cases and combo box has same indices

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

        IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->itemData(comboBoxChangeCase->currentIndex()).toInt();
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QLatin1String("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QLatin1String("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QLatin1String("c"));

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
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex((int)info.caseChange); /// enum has numbers assigned to cases and combo box has same indices

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

        IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->itemData(comboBoxChangeCase->currentIndex()).toInt();
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QLatin1String("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QLatin1String("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QLatin1String("c"));

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
    QList<TokenWidget *> widgetList;
    QLabel *labelPreview;
    KPushButton *buttonAddTokenAtTop, *buttonAddTokenAtBottom;
    const Entry *previewEntry;
    QSignalMapper *signalMapperRemove, *signalMapperMoveUp, *signalMapperMoveDown;
    QScrollArea *area;

    IdSuggestionsEditWidgetPrivate(const Entry *pe, IdSuggestionsEditWidget *parent)
            : p(parent), previewEntry(pe) {
        // TODO
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        labelPreview = new QLabel(p);
        layout->addWidget(labelPreview, 0, 0, 1, 1);
        layout->setColumnStretch(0, 100);

        area = new QScrollArea(p);
        layout->addWidget(area, 1, 0, 1, 1);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        container = new QWidget(area);
        area->setWidget(container);
        area->setWidgetResizable(true);
        containerLayout = new QVBoxLayout(container);
        area->setMinimumSize(384, 256);

        buttonAddTokenAtTop = new KPushButton(KIcon("list-add"), i18n("Add at top"), container);
        containerLayout->addWidget(buttonAddTokenAtTop, 0);

        containerLayout->addStretch(1);

        buttonAddTokenAtBottom = new KPushButton(KIcon("list-add"), i18n("Add at bottom"), container);
        containerLayout->addWidget(buttonAddTokenAtBottom, 0);

        QMenu *menuAddToken = new QMenu(p);
        QSignalMapper *signalMapperAddMenu = new QSignalMapper(p);
        buttonAddTokenAtTop->setMenu(menuAddToken);
        QAction *action = menuAddToken->addAction(i18n("Title"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, -ttTitle);
        action = menuAddToken->addAction(i18n("Author"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, -ttAuthor);
        action = menuAddToken->addAction(i18n("Year"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, -ttYear);
        action = menuAddToken->addAction(i18n("Text"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, -ttText);
        connect(signalMapperAddMenu, SIGNAL(mapped(int)), p, SLOT(addToken(int)));

        menuAddToken = new QMenu(p);
        signalMapperAddMenu = new QSignalMapper(p);
        buttonAddTokenAtBottom->setMenu(menuAddToken);
        action = menuAddToken->addAction(i18n("Title"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttTitle);
        action = menuAddToken->addAction(i18n("Author"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttAuthor);
        action = menuAddToken->addAction(i18n("Year"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttYear);
        action = menuAddToken->addAction(i18n("Text"), signalMapperAddMenu, SLOT(map()));
        signalMapperAddMenu->setMapping(action, ttText);
        connect(signalMapperAddMenu, SIGNAL(mapped(int)), p, SLOT(addToken(int)));

        signalMapperMoveUp = new QSignalMapper(p);
        connect(signalMapperMoveUp, SIGNAL(mapped(QWidget *)), p, SLOT(moveUpToken(QWidget *)));
        signalMapperMoveDown = new QSignalMapper(p);
        connect(signalMapperMoveDown, SIGNAL(mapped(QWidget *)), p, SLOT(moveDownToken(QWidget *)));
        signalMapperRemove = new QSignalMapper(p);
        connect(signalMapperRemove, SIGNAL(mapped(QWidget *)), p, SLOT(removeToken(QWidget *)));

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

    void add(TokenType tokenType, bool atTop) {
        const int pos = atTop ? 1 : containerLayout->count() - 2;
        TokenWidget *tokenWidget = NULL;
        switch (tokenType) {
        case ttTitle: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = -1;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new TitleWidget(info, true, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }
        break;
        case ttAuthor: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = -1;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new AuthorWidget(info, aAll, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }
        break;
        case ttYear:
            tokenWidget = new YearWidget(4, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
            break;
        case ttText:
            tokenWidget = new TextWidget(QString(), p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }

        addManagementButtons(tokenWidget);
    }

    void reset(const QString &formatString) {
        while (!widgetList.isEmpty())
            delete widgetList.takeFirst();

        QStringList tokenList = formatString.split(QLatin1Char('|'), QString::SkipEmptyParts);
        foreach(const QString &token, tokenList) {
            TokenWidget *tokenWidget = NULL;

            if (token[0] == 'a' || token[0] == 'A' || token[0] == 'z') {
                IdSuggestions::Authors author = token[0] == 'a' ? aOnlyFirst : (token[0] == 'A' ? aAll : aNotFirst);
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new AuthorWidget(info, author, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'y') {
                tokenWidget = new YearWidget(2, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'Y') {
                tokenWidget = new YearWidget(4, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 't' || token[0] == 'T') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new TitleWidget(info, token[0] == 'T', p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == '"') {
                tokenWidget = new TextWidget(token.mid(1), p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
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

IdSuggestionsEditWidget::IdSuggestionsEditWidget(const Entry *previewEntry, QWidget *parent, Qt::WindowFlags f)
        : QWidget(parent, f), IdSuggestions(), d(new IdSuggestionsEditWidgetPrivate(previewEntry, this))
{
    d->setupGUI();
}

IdSuggestionsEditWidget::~IdSuggestionsEditWidget()
{
// TODO
}

void IdSuggestionsEditWidget::setFormatString(const QString &formatString)
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
    TokenWidget *tokenWidget = static_cast<TokenWidget *>(widget);
    int curPos = d->widgetList.indexOf(tokenWidget);
    if (curPos > 0) {
        d->widgetList.removeAt(curPos);
        const int layoutPos = d->containerLayout->indexOf(tokenWidget);
        d->containerLayout->removeWidget(tokenWidget);
        d->widgetList.insert(curPos - 1, tokenWidget);
        d->containerLayout->insertWidget(layoutPos - 1, tokenWidget, 1);
    }
}

void IdSuggestionsEditWidget::moveDownToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget *>(widget);
    int curPos = d->widgetList.indexOf(tokenWidget);
    if (curPos < d->widgetList.size() - 1) {
        d->widgetList.removeAt(curPos);
        const int layoutPos = d->containerLayout->indexOf(tokenWidget);
        d->containerLayout->removeWidget(tokenWidget);
        d->widgetList.insert(curPos + 1, tokenWidget);
        d->containerLayout->insertWidget(layoutPos + 1, tokenWidget, 1);
    }
}

void IdSuggestionsEditWidget::removeToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget *>(widget);
    d->widgetList.removeOne(tokenWidget);
    d->containerLayout->removeWidget(tokenWidget);
    tokenWidget->deleteLater();
    updatePreview();
}

void IdSuggestionsEditWidget::addToken(int cmd)
{
    if (cmd < 0) {
        d->add((IdSuggestionsEditWidgetPrivate::TokenType)(-cmd), true);
        d->area->ensureWidgetVisible(d->buttonAddTokenAtTop); // FIXME does not work as intended
    } else {
        d->add((IdSuggestionsEditWidgetPrivate::TokenType)cmd, false);
        d->area->ensureWidgetVisible(d->buttonAddTokenAtBottom); // FIXME does not work as intended
    }
    updatePreview();
}

IdSuggestionsEditDialog::IdSuggestionsEditDialog(QWidget *parent, Qt::WFlags flags)
        : KDialog(parent, flags)
{
    setCaption(i18n("Edit Id Suggestion"));
    setButtons(KDialog::Ok | KDialog::Cancel);
}

IdSuggestionsEditDialog::~IdSuggestionsEditDialog()
{
    // TODO
}

QString IdSuggestionsEditDialog::editSuggestion(const Entry *previewEntry, const QString &suggestion, QWidget *parent)
{
    QPointer<IdSuggestionsEditDialog> dlg = new IdSuggestionsEditDialog(parent);
    IdSuggestionsEditWidget *widget = new IdSuggestionsEditWidget(previewEntry, dlg);
    dlg->setMainWidget(widget);

    widget->setFormatString(suggestion);
    if (dlg->exec() == Accepted) {
        const QString formatString = widget->formatString();
        delete dlg;
        return formatString;
    }

    delete dlg;

    /// Return unmodified original suggestion
    return suggestion;
}
