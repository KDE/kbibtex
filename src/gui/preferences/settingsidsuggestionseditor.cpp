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
#include <QSpinBox>
#include <QCheckBox>
#include <QSignalMapper>
#include <QMenu>

#include <KLineEdit>
#include <KComboBox>
#include <KLocale>
#include <KPushButton>
#include <KAction>

#include <3rdparty/libqxt/gui/qxtspanslider.h>

#include "settingsidsuggestionseditor.h"


TokenWidget::TokenWidget(QWidget *parent)
        : QGroupBox(parent)
{
    gridLayout = new QGridLayout(this);
    formLayout = new QFormLayout();
    gridLayout->addLayout(formLayout, 0, 0, 4, 1);
    gridLayout->setColumnStretch(0, 100);
}

void TokenWidget::addButtons(KPushButton *buttonUp, KPushButton *buttonDown, KPushButton *buttonRemove)
{
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


AuthorWidget::AuthorWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestions::Authors author, IdSuggestionsEditWidget *isew, QWidget *parent)
        : TokenWidget(parent)
{
    setTitle(i18n("Authors"));

    QBoxLayout *boxLayout = new QVBoxLayout();
    boxLayout->setMargin(0);
    formLayout->addRow(i18n("Author Range:"), boxLayout);
    spanSliderAuthor = new QxtSpanSlider(Qt::Horizontal, this);
    boxLayout->addWidget(spanSliderAuthor);
    spanSliderAuthor->setRange(0, 9);
    spanSliderAuthor->setHandleMovementMode(QxtSpanSlider::NoCrossing);
    switch (author) {
    case IdSuggestions::aAll:
        if (info.startWord > 0 || info.endWord < 0xffff) {
            spanSliderAuthor->setLowerValue(info.startWord);
            spanSliderAuthor->setUpperValue(qMin(spanSliderAuthor->maximum(), info.endWord));
        } else {
            spanSliderAuthor->setLowerValue(0);
            spanSliderAuthor->setUpperValue(spanSliderAuthor->maximum());
        }
        break;
    case IdSuggestions::aNotFirst:
        spanSliderAuthor->setLowerValue(1);
        spanSliderAuthor->setUpperValue(spanSliderAuthor->maximum());
        break;
    case IdSuggestions::aOnlyFirst:
        spanSliderAuthor->setLowerValue(0);
        spanSliderAuthor->setUpperValue(0);
        break;
    }

    labelAuthorRange = new QLabel(this);
    boxLayout->addWidget(labelAuthorRange);
    const int a = qMax(labelAuthorRange->fontMetrics().width(i18n("From first to last author")), labelAuthorRange->fontMetrics().width(i18n("From author %1 to last author", 88)));
    const int b = qMax(labelAuthorRange->fontMetrics().width(i18n("From first author to author %1", 88)), labelAuthorRange->fontMetrics().width(i18n("From author %1 to author %2", 88, 88)));
    labelAuthorRange->setMinimumWidth(qMax(a, b));

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

    connect(spanSliderAuthor, SIGNAL(lowerValueChanged(int)), isew, SLOT(updatePreview()));
    connect(spanSliderAuthor, SIGNAL(upperValueChanged(int)), isew, SLOT(updatePreview()));
    connect(spanSliderAuthor, SIGNAL(lowerValueChanged(int)), this, SLOT(updateRangeLabel()));
    connect(spanSliderAuthor, SIGNAL(upperValueChanged(int)), this, SLOT(updateRangeLabel()));
    connect(comboBoxChangeCase, SIGNAL(currentIndexChanged(int)), isew, SLOT(updatePreview()));
    connect(lineEditTextInBetween, SIGNAL(textEdited(QString)), isew, SLOT(updatePreview()));
    connect(spinBoxLength, SIGNAL(valueChanged(int)), isew, SLOT(updatePreview()));

    updateRangeLabel();
}

QString AuthorWidget::toString() const
{
    QString result = QLatin1String("A");

    if (spinBoxLength->value() > 0)
        result.append(QString::number(spinBoxLength->value()));

    IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->currentIndex();
    if (caseChange == IdSuggestions::ccToLower)
        result.append(QLatin1String("l"));
    else if (caseChange == IdSuggestions::ccToUpper)
        result.append(QLatin1String("u"));
    else if (caseChange == IdSuggestions::ccToCamelCase)
        result.append(QLatin1String("c"));

    if (spanSliderAuthor->lowerValue() > spanSliderAuthor->minimum() || spanSliderAuthor->upperValue() < spanSliderAuthor->maximum())
        result.append(QString(QLatin1String("w%1%2")).arg(spanSliderAuthor->lowerValue()).arg(spanSliderAuthor->upperValue() < spanSliderAuthor->maximum() ? QString::number(spanSliderAuthor->upperValue()) : QLatin1String("I")));

    const QString text = lineEditTextInBetween->text();
    if (!text.isEmpty())
        result.append(QLatin1String("\"")).append(text);

    return result;
}

void AuthorWidget::updateRangeLabel()
{
    const int lower = spanSliderAuthor->lowerValue();
    const int upper = spanSliderAuthor->upperValue();
    const int min = spanSliderAuthor->minimum();
    const int max = spanSliderAuthor->maximum();

    if (lower == min && upper == min)
        labelAuthorRange->setText(i18n("First author only"));
    else if (lower == min + 1 && upper == max)
        labelAuthorRange->setText(i18n("All but first author"));
    else if (lower == min && upper == max)
        labelAuthorRange->setText(i18n("From first to last author"));
    else if (lower > min && upper == max)
        labelAuthorRange->setText(i18n("From author %1 to last author", lower + 1));
    else if (lower == min && upper < max)
        labelAuthorRange->setText(i18n("From first author to author %1", upper + 1));
    else
        labelAuthorRange->setText(i18n("From author %1 to author %2", lower + 1, upper + 1));
}

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

TitleWidget::TitleWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, bool removeSmallWords, IdSuggestionsEditWidget *isew, QWidget *parent)
        : TokenWidget(parent)
{
    setTitle(i18n("Title"));

    QBoxLayout *boxLayout = new QVBoxLayout();
    boxLayout->setMargin(0);
    formLayout->addRow(i18n("Word Range:"), boxLayout);
    spanSliderWords = new QxtSpanSlider(Qt::Horizontal, this);
    boxLayout->addWidget(spanSliderWords);
    spanSliderWords->setRange(0, 9);
    spanSliderWords->setHandleMovementMode(QxtSpanSlider::NoCrossing);
    if (info.startWord > 0 || info.endWord < 0xffff) {
        spanSliderWords->setLowerValue(info.startWord);
        spanSliderWords->setUpperValue(qMin(spanSliderWords->maximum(), info.endWord));
    } else {
        spanSliderWords->setLowerValue(0);
        spanSliderWords->setUpperValue(spanSliderWords->maximum());
    }

    labelWordsRange = new QLabel(this);
    boxLayout->addWidget(labelWordsRange);
    const int a = qMax(labelWordsRange->fontMetrics().width(i18n("From word to last word")), labelWordsRange->fontMetrics().width(i18n("From word %1 to last word", 88)));
    const int b = qMax(labelWordsRange->fontMetrics().width(i18n("From word author to word %1", 88)), labelWordsRange->fontMetrics().width(i18n("From word %1 to word %2", 88, 88)));
    labelWordsRange->setMinimumWidth(qMax(a, b));

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

    connect(spanSliderWords, SIGNAL(lowerValueChanged(int)), isew, SLOT(updatePreview()));
    connect(spanSliderWords, SIGNAL(upperValueChanged(int)), isew, SLOT(updatePreview()));
    connect(spanSliderWords, SIGNAL(lowerValueChanged(int)), this, SLOT(updateRangeLabel()));
    connect(spanSliderWords, SIGNAL(upperValueChanged(int)), this, SLOT(updateRangeLabel()));
    connect(checkBoxRemoveSmallWords, SIGNAL(toggled(bool)), isew, SLOT(updatePreview()));
    connect(comboBoxChangeCase, SIGNAL(currentIndexChanged(int)), isew, SLOT(updatePreview()));
    connect(lineEditTextInBetween, SIGNAL(textEdited(QString)), isew, SLOT(updatePreview()));
    connect(spinBoxLength, SIGNAL(valueChanged(int)), isew, SLOT(updatePreview()));
}

QString TitleWidget::toString() const
{
    QString result = checkBoxRemoveSmallWords->isChecked() ? QLatin1String("T") : QLatin1String("t");

    if (spinBoxLength->value() > 0)
        result.append(QString::number(spinBoxLength->value()));

    IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->currentIndex();
    if (caseChange == IdSuggestions::ccToLower)
        result.append(QLatin1String("l"));
    else if (caseChange == IdSuggestions::ccToUpper)
        result.append(QLatin1String("u"));
    else if (caseChange == IdSuggestions::ccToCamelCase)
        result.append(QLatin1String("c"));

    if (spanSliderWords->lowerValue() > spanSliderWords->minimum() || spanSliderWords->upperValue() < spanSliderWords->maximum())
        result.append(QString(QLatin1String("w%1%2")).arg(spanSliderWords->lowerValue()).arg(spanSliderWords->upperValue() < spanSliderWords->maximum() ? QString::number(spanSliderWords->upperValue()) : QLatin1String("I")));

    const QString text = lineEditTextInBetween->text();
    if (!text.isEmpty())
        result.append(QLatin1String("\"")).append(text);

    return result;
}

void TitleWidget::updateRangeLabel()
{
    const int lower = spanSliderWords->lowerValue();
    const int upper = spanSliderWords->upperValue();
    const int min = spanSliderWords->minimum();
    const int max = spanSliderWords->maximum();

    if (lower == min && upper == min)
        labelWordsRange->setText(i18n("First word only"));
    else if (lower == min + 1 && upper == max)
        labelWordsRange->setText(i18n("All but first word"));
    else if (lower == min && upper == max)
        labelWordsRange->setText(i18n("From first to last word"));
    else if (lower > min && upper == max)
        labelWordsRange->setText(i18n("From word %1 to last word", lower + 1));
    else if (lower == min && upper < max)
        labelWordsRange->setText(i18n("From first word to word %1", upper + 1));
    else
        labelWordsRange->setText(i18n("From word %1 to word %2", lower + 1, upper + 1));
}

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
            info.startWord = 0;
            info.endWord = 0x00ffffff;
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
            info.startWord = 0;
            info.endWord = 0x00ffffff;
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
    IdSuggestionsEditDialog dlg(parent);
    IdSuggestionsEditWidget widget(previewEntry, &dlg);
    dlg.setMainWidget(&widget);


    widget.setFormatString(suggestion);
    if (dlg.exec() == Accepted)
        return widget.formatString();

    /// Return unmodified original suggestion
    return suggestion;
}
