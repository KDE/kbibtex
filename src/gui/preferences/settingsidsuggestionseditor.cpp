/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "settingsidsuggestionseditor.h"

#include <limits>

#include <QTimer>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QAction>
#include <QDialogButtonBox>

#include <KLocalizedString>
#include <KIconLoader>

#include "widgets/rangewidget.h"

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
    explicit TokenWidget(QWidget *parent)
            : QGroupBox(parent)
    {
        gridLayout = new QGridLayout(this);
        formLayout = new QFormLayout();
        gridLayout->addLayout(formLayout, 0, 0, 4, 1);
        gridLayout->setColumnStretch(0, 100);
    }

    void addButtons(QPushButton *buttonUp, QPushButton *buttonDown, QPushButton *buttonRemove)
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

    virtual QString toString() const = 0;
};


/**
 * @author Thomas Fischer
 */
class AuthorWidget : public TokenWidget
{
    Q_OBJECT

private:
    RangeWidget *rangeWidgetAuthor;
    QCheckBox *checkBoxLastAuthor;
    QLabel *labelAuthorRange;
    QComboBox *comboBoxChangeCase;
    QLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

private slots:
    void updateRangeLabel()
    {
        const int lower = rangeWidgetAuthor->lowerValue();
        const int upper = rangeWidgetAuthor->upperValue();
        const int max = rangeWidgetAuthor->maximum();

        labelAuthorRange->setText(IdSuggestions::formatAuthorRange(lower, upper == max ? std::numeric_limits<int>::max() : upper, checkBoxLastAuthor->isChecked()));
    }

public:
    AuthorWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent)
    {
        setTitle(i18n("Authors"));

        QBoxLayout *boxLayout = new QVBoxLayout();
        boxLayout->setMargin(0);
        formLayout->addRow(i18n("Author Range:"), boxLayout);
        static const QStringList authorRange {i18n("First author"), i18n("Second author"), i18n("Third author"), i18n("Fourth author"), i18n("Fifth author"), i18n("Sixth author"), i18n("Seventh author"), i18n("Eighth author"), i18n("Ninth author"), i18n("Tenth author"), i18n("|Last author")};
        rangeWidgetAuthor = new RangeWidget(authorRange, this);
        boxLayout->addWidget(rangeWidgetAuthor);
        rangeWidgetAuthor->setLowerValue(info.startWord);
        rangeWidgetAuthor->setUpperValue(qMin(authorRange.size() - 1, info.endWord));

        checkBoxLastAuthor = new QCheckBox(i18n("... and last author"), this);
        boxLayout->addWidget(checkBoxLastAuthor);

        labelAuthorRange = new QLabel(this);
        boxLayout->addWidget(labelAuthorRange);
#if QT_VERSION >= 0x050b00
        const int maxWidth = qMax(labelAuthorRange->fontMetrics().horizontalAdvance(i18n("From first author to author %1 and last author", 88)), labelAuthorRange->fontMetrics().horizontalAdvance(i18n("From author %1 to author %2 and last author", 88, 88)));
#else // QT_VERSION >= 0x050b00
        const int maxWidth = qMax(labelAuthorRange->fontMetrics().width(i18n("From first author to author %1 and last author", 88)), labelAuthorRange->fontMetrics().width(i18n("From author %1 to author %2 and last author", 88, 88)));
#endif // QT_VERSION >= 0x050b00
        labelAuthorRange->setMinimumWidth(maxWidth);

        comboBoxChangeCase = new QComboBox(this);
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex(static_cast<int>(info.caseChange)); /// enum has numbers assigned to cases and combo box has same indices

        lineEditTextInBetween = new QLineEdit(this);
        formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
        lineEditTextInBetween->setText(info.inBetween);

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(rangeWidgetAuthor, &RangeWidget::lowerValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(rangeWidgetAuthor, &RangeWidget::upperValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(rangeWidgetAuthor, &RangeWidget::lowerValueChanged, this, &AuthorWidget::updateRangeLabel);
        connect(rangeWidgetAuthor, &RangeWidget::upperValueChanged, this, &AuthorWidget::updateRangeLabel);
        connect(checkBoxLastAuthor, &QCheckBox::toggled, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(checkBoxLastAuthor, &QCheckBox::toggled, this, &AuthorWidget::updateRangeLabel);
        connect(comboBoxChangeCase, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
        connect(lineEditTextInBetween, &QLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);

        updateRangeLabel();
    }

    QString toString() const override
    {
        QString result = QStringLiteral("A");

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        const IdSuggestions::CaseChange caseChange = static_cast<IdSuggestions::CaseChange>(comboBoxChangeCase->currentIndex());
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QStringLiteral("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QStringLiteral("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QStringLiteral("c"));

        if (rangeWidgetAuthor->lowerValue() > 0 || rangeWidgetAuthor->upperValue() < rangeWidgetAuthor->maximum())
            result.append(QString(QStringLiteral("w%1%2")).arg(rangeWidgetAuthor->lowerValue()).arg(rangeWidgetAuthor->upperValue() < rangeWidgetAuthor->maximum() ? QString::number(rangeWidgetAuthor->upperValue()) : QStringLiteral("I")));
        if (checkBoxLastAuthor->isChecked())
            result.append(QStringLiteral("L"));

        const QString text = lineEditTextInBetween->text();
        if (!text.isEmpty())
            result.append(QStringLiteral("\"")).append(text);

        return result;
    }
};


/**
 * @author Thomas Fischer
 */
class YearWidget : public TokenWidget
{
    Q_OBJECT

private:
    QComboBox *comboBoxDigits;

public:
    YearWidget(int digits, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Year"));

        comboBoxDigits = new QComboBox(this);
        comboBoxDigits->addItem(i18n("2 digits"), 2);
        comboBoxDigits->addItem(i18n("4 digits"), 4);
        formLayout->addRow(i18n("Digits:"), comboBoxDigits);
        comboBoxDigits->setCurrentIndex(comboBoxDigits->findData(digits));

        connect(comboBoxDigits, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const override {
        const int year = comboBoxDigits->itemData(comboBoxDigits->currentIndex()).toInt();
        QString result = year == 4 ? QStringLiteral("Y") : QStringLiteral("y");

        return result;
    }
};

/**
 * @author Thomas Fischer
 */
class VolumeWidget : public TokenWidget
{
    Q_OBJECT

private:
    QLabel *labelCheckmark;

public:
    VolumeWidget(IdSuggestionsEditWidget *, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Volume"));

        labelCheckmark = new QLabel(this);
        labelCheckmark->setPixmap(KIconLoader::global()->loadMimeTypeIcon(QStringLiteral("dialog-ok-apply"), KIconLoader::Small));
        formLayout->addRow(i18n("Volume:"), labelCheckmark);
    }

    QString toString() const override {
        return QStringLiteral("v");
    }
};


/**
 * @author Thomas Fischer
 */
class PageNumberWidget : public TokenWidget
{
    Q_OBJECT

private:
    QLabel *labelCheckmark;

public:
    PageNumberWidget(IdSuggestionsEditWidget *, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Page Number"));

        labelCheckmark = new QLabel(this);
        labelCheckmark->setPixmap(KIconLoader::global()->loadMimeTypeIcon(QStringLiteral("dialog-ok-apply"), KIconLoader::Small));
        formLayout->addRow(i18n("First page's number:"), labelCheckmark);
    }

    QString toString() const override {
        return QStringLiteral("p");
    }
};


/**
 * @author Thomas Fischer
 */
class TitleWidget : public TokenWidget
{
    Q_OBJECT

private:
    RangeWidget *rangeWidgetAuthor;
    QLabel *labelWordsRange;
    QCheckBox *checkBoxRemoveSmallWords;
    QComboBox *comboBoxChangeCase;
    QLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

private slots:
    void updateRangeLabel()
    {
        const int lower = rangeWidgetAuthor->lowerValue();
        const int upper = rangeWidgetAuthor->upperValue();
        const int max = rangeWidgetAuthor->maximum();

        if (lower == 0 && upper == 0)
            labelWordsRange->setText(i18n("First word only"));
        else if (lower == 1 && upper >= max)
            labelWordsRange->setText(i18n("All but first word"));
        else if (lower == 0 && upper >= max)
            labelWordsRange->setText(i18n("From first to last word"));
        else if (lower > 0 && upper >= max)
            labelWordsRange->setText(i18n("From word %1 to last word", lower + 1));
        else if (lower == 0 && upper < max)
            labelWordsRange->setText(i18n("From first word to word %1", upper + 1));
        else
            labelWordsRange->setText(i18n("From word %1 to word %2", lower + 1, upper + 1));
    }

public:
    TitleWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, bool removeSmallWords, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent)
    {
        setTitle(i18n("Title"));

        QBoxLayout *boxLayout = new QVBoxLayout();
        boxLayout->setMargin(0);
        formLayout->addRow(i18n("Word Range:"), boxLayout);
        static const QStringList wordRange {i18n("First word"), i18n("Second word"), i18n("Third word"), i18n("Fourth word"), i18n("Fifth word"), i18n("Sixth word"), i18n("Seventh word"), i18n("Eighth word"), i18n("Ninth word"), i18n("Tenth word"), i18n("|Last word")};
        rangeWidgetAuthor = new RangeWidget(wordRange, this);
        boxLayout->addWidget(rangeWidgetAuthor);
        rangeWidgetAuthor->setLowerValue(info.startWord);
        rangeWidgetAuthor->setUpperValue(qMin(rangeWidgetAuthor->maximum(), info.endWord));

        labelWordsRange = new QLabel(this);
        boxLayout->addWidget(labelWordsRange);
#if QT_VERSION >= 0x050b00
        const int a = qMax(labelWordsRange->fontMetrics().horizontalAdvance(i18n("From first to last word")), labelWordsRange->fontMetrics().horizontalAdvance(i18n("From word %1 to last word", 88)));
        const int b = qMax(labelWordsRange->fontMetrics().horizontalAdvance(i18n("From first word to word %1", 88)), labelWordsRange->fontMetrics().horizontalAdvance(i18n("From word %1 to word %2", 88, 88)));
#else // QT_VERSION >= 0x050b00
        const int a = qMax(labelWordsRange->fontMetrics().width(i18n("From first to last word")), labelWordsRange->fontMetrics().width(i18n("From word %1 to last word", 88)));
        const int b = qMax(labelWordsRange->fontMetrics().width(i18n("From first word to word %1", 88)), labelWordsRange->fontMetrics().width(i18n("From word %1 to word %2", 88, 88)));
#endif // QT_VERSION >= 0x050b00
        labelWordsRange->setMinimumWidth(qMax(a, b));

        checkBoxRemoveSmallWords = new QCheckBox(i18n("Remove"), this);
        formLayout->addRow(i18n("Small words:"), checkBoxRemoveSmallWords);
        checkBoxRemoveSmallWords->setChecked(removeSmallWords);

        comboBoxChangeCase = new QComboBox(this);
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex(static_cast<int>(info.caseChange)); /// enum has numbers assigned to cases and combo box has same indices

        lineEditTextInBetween = new QLineEdit(this);
        formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
        lineEditTextInBetween->setText(info.inBetween);

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(rangeWidgetAuthor, &RangeWidget::lowerValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(rangeWidgetAuthor, &RangeWidget::upperValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(rangeWidgetAuthor, &RangeWidget::lowerValueChanged, this, &TitleWidget::updateRangeLabel);
        connect(rangeWidgetAuthor, &RangeWidget::upperValueChanged, this, &TitleWidget::updateRangeLabel);
        connect(checkBoxRemoveSmallWords, &QCheckBox::toggled, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(comboBoxChangeCase, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
        connect(lineEditTextInBetween, &QLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);

        updateRangeLabel();
    }

    QString toString() const override
    {
        QString result = checkBoxRemoveSmallWords->isChecked() ? QStringLiteral("T") : QStringLiteral("t");

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        const IdSuggestions::CaseChange caseChange = static_cast<IdSuggestions::CaseChange>(comboBoxChangeCase->currentIndex());
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QStringLiteral("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QStringLiteral("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QStringLiteral("c"));

        if (rangeWidgetAuthor->lowerValue() > 0 || rangeWidgetAuthor->upperValue() < rangeWidgetAuthor->maximum())
            result.append(QString(QStringLiteral("w%1%2")).arg(rangeWidgetAuthor->lowerValue()).arg(rangeWidgetAuthor->upperValue() < rangeWidgetAuthor->maximum() ? QString::number(rangeWidgetAuthor->upperValue()) : QStringLiteral("I")));

        const QString text = lineEditTextInBetween->text();
        if (!text.isEmpty())
            result.append(QStringLiteral("\"")).append(text);

        return result;
    }
};


/**
 * @author Thomas Fischer
 */
class JournalWidget : public TokenWidget
{
    Q_OBJECT

private:
    QCheckBox *checkBoxRemoveSmallWords;
    QComboBox *comboBoxChangeCase;
    QLineEdit *lineEditTextInBetween;
    QSpinBox *spinBoxLength;

public:
    JournalWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, bool removeSmallWords, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent)
    {
        setTitle(i18n("Journal"));

        QBoxLayout *boxLayout = new QVBoxLayout();
        boxLayout->setMargin(0);

        checkBoxRemoveSmallWords = new QCheckBox(i18n("Remove"), this);
        formLayout->addRow(i18n("Small words:"), checkBoxRemoveSmallWords);
        checkBoxRemoveSmallWords->setChecked(removeSmallWords);

        comboBoxChangeCase = new QComboBox(this);
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex(static_cast<int>(info.caseChange)); /// enum has numbers assigned to cases and combo box has same indices

        lineEditTextInBetween = new QLineEdit(this);
        formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
        lineEditTextInBetween->setText(info.inBetween);

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(checkBoxRemoveSmallWords, &QCheckBox::toggled, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(comboBoxChangeCase, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
        connect(lineEditTextInBetween, &QLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
        connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const override
    {
        QString result = checkBoxRemoveSmallWords->isChecked() ? QStringLiteral("J") : QStringLiteral("j");

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        const IdSuggestions::CaseChange caseChange = static_cast<IdSuggestions::CaseChange>(comboBoxChangeCase->currentIndex());
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QStringLiteral("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QStringLiteral("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QStringLiteral("c"));

        const QString text = lineEditTextInBetween->text();
        if (!text.isEmpty())
            result.append(QStringLiteral("\"")).append(text);

        return result;
    }
};


/**
 * @author Erik Quaeghebeur
 */
class TypeWidget : public TokenWidget
{
    Q_OBJECT

private:
    QComboBox *comboBoxChangeCase;
    QSpinBox *spinBoxLength;

public:
    TypeWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent)
    {
        setTitle(i18n("Type"));

        QBoxLayout *boxLayout = new QVBoxLayout();
        boxLayout->setMargin(0);

        comboBoxChangeCase = new QComboBox(this);
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex(static_cast<int>(info.caseChange)); /// enum has numbers assigned to cases and combo box has same indices

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(comboBoxChangeCase, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
        connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const override
    {
        QString result = QStringLiteral("e");

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        const IdSuggestions::CaseChange caseChange = static_cast<IdSuggestions::CaseChange>(comboBoxChangeCase->currentIndex());
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QStringLiteral("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QStringLiteral("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QStringLiteral("c"));

        return result;
    }
};


/**
 * @author Thomas Fischer
 */
class TextWidget : public TokenWidget
{
    Q_OBJECT

private:
    QLineEdit *lineEditText;

public:
    TextWidget(const QString &text, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Text"));

        lineEditText = new QLineEdit(this);
        formLayout->addRow(i18n("Text:"), lineEditText);
        lineEditText->setText(text);

        connect(lineEditText, &QLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const override {
        QString result = QStringLiteral("\"") + lineEditText->text();
        return result;
    }
};

class IdSuggestionsEditWidget::IdSuggestionsEditWidgetPrivate
{
private:
    IdSuggestionsEditWidget *p;
public:
    enum TokenType {ttTitle, ttAuthor, ttYear, ttJournal, ttType, ttText, ttVolume, ttPageNumber};
    enum Location {locAtTop, locAtBottom};

    QWidget *container;
    QBoxLayout *containerLayout;
    QList<TokenWidget *> widgetList;
    QLabel *labelPreview;
    QPushButton *buttonAddTokenAtTop, *buttonAddTokenAtBottom;
    const Entry *previewEntry;
    QScrollArea *area;

    IdSuggestionsEditWidgetPrivate(const Entry *pe, IdSuggestionsEditWidget *parent)
            : p(parent), previewEntry(pe) {
        setupGUI();
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

        buttonAddTokenAtTop = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add at top"), container);
        containerLayout->addWidget(buttonAddTokenAtTop, 0);

        containerLayout->addStretch(1);

        buttonAddTokenAtBottom = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add at bottom"), container);
        containerLayout->addWidget(buttonAddTokenAtBottom, 0);

        QMenu *menuAddToken = new QMenu(p);
        buttonAddTokenAtTop->setMenu(menuAddToken);
        QAction *action = menuAddToken->addAction(i18n("Title"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttTitle, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Author"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttAuthor, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Year"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttYear, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Journal"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttJournal, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Type"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttType, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Volume"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttVolume, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Page Number"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttPageNumber, locAtTop);
        });
        action = menuAddToken->addAction(i18n("Text"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttText, locAtTop);
        });

        menuAddToken = new QMenu(p);
        buttonAddTokenAtBottom->setMenu(menuAddToken);
        action = menuAddToken->addAction(i18n("Title"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttTitle, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Author"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttAuthor, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Year"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttYear, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Journal"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttJournal, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Type"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttType, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Volume"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttVolume, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Page Number"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttPageNumber, locAtBottom);
        });
        action = menuAddToken->addAction(i18n("Text"));
        connect(action, &QAction::triggered, p, [this]() {
            addToken(ttText, locAtBottom);
        });
    }

    void addManagementButtons(TokenWidget *tokenWidget) {
        if (tokenWidget != nullptr) {
            QPushButton *buttonUp = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), QString(), tokenWidget);
            QPushButton *buttonDown = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down")), QString(), tokenWidget);
            QPushButton *buttonRemove = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), QString(), tokenWidget);
            tokenWidget->addButtons(buttonUp, buttonDown, buttonRemove);
            connect(buttonUp, &QPushButton::clicked, p, [this, tokenWidget]() {
                moveUpToken(tokenWidget);
            });
            connect(buttonDown, &QPushButton::clicked, p, [this, tokenWidget]() {
                moveDownToken(tokenWidget);
            });
            connect(buttonRemove, &QPushButton::clicked, p, [this, tokenWidget]() {
                removeToken(tokenWidget);
            });
        }
    }

    void add(const TokenType tokenType, const Location location) {
        TokenWidget *tokenWidget = nullptr;
        switch (tokenType) {
        case ttTitle: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = std::numeric_limits<int>::max();
            info.startWord = 0;
            info.endWord = std::numeric_limits<int>::max();
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new TitleWidget(info, true, p, container);
        }
        break;
        case ttAuthor: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = std::numeric_limits<int>::max();
            info.startWord = 0;
            info.endWord = std::numeric_limits<int>::max();
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new AuthorWidget(info, p, container);
        }
        break;
        case ttYear:
            tokenWidget = new YearWidget(4, p, container);
            break;
        case ttJournal: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = 1;
            info.startWord = 0;
            info.endWord = std::numeric_limits<int>::max();
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new JournalWidget(info, true, p, container);
        }
        break;
        case ttType: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = std::numeric_limits<int>::max();
            info.startWord = 0;
            info.endWord = std::numeric_limits<int>::max();
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new TypeWidget(info, p, container);
        }
        break;
        case ttText:
            tokenWidget = new TextWidget(QString(), p, container);
            break;
        case ttVolume:
            tokenWidget = new VolumeWidget(p, container);
            break;
        case ttPageNumber:
            tokenWidget = new PageNumberWidget(p, container);
            break;
        }

        if (tokenWidget != nullptr) {
            const int pos = location == locAtTop ? 1 : containerLayout->count() - 2;
            if (location == locAtTop)
                widgetList.prepend(tokenWidget);
            else if (location == locAtBottom)
                widgetList.append(tokenWidget);
            containerLayout->insertWidget(pos, tokenWidget, 1);
            addManagementButtons(tokenWidget);
        }
    }

    void reset(const QString &formatString) {
        while (!widgetList.isEmpty())
            delete widgetList.takeFirst();

        const QStringList tokenList = formatString.split(QStringLiteral("|"), QString::SkipEmptyParts);
        for (const QString &token : tokenList) {
            TokenWidget *tokenWidget = nullptr;

            if (token[0] == 'a' || token[0] == 'A' || token[0] == 'z') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                /// Support deprecated 'a' and 'z' cases
                if (token[0] == 'a')
                    info.startWord = info.endWord = 0;
                else if (token[0] == 'z') {
                    info.startWord = 1;
                    info.endWord = std::numeric_limits<int>::max();
                }
                tokenWidget = new AuthorWidget(info, p, container);
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
                tokenWidget = new TitleWidget(info, token[0].isUpper(), p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'j' || token[0] == 'J') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new JournalWidget(info, token[0].isUpper(), p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'e') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new TypeWidget(info, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'v') {
                tokenWidget = new VolumeWidget(p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'p') {
                tokenWidget = new PageNumberWidget(p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == '"') {
                tokenWidget = new TextWidget(token.mid(1), p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            }

            if (tokenWidget != nullptr)
                addManagementButtons(tokenWidget);
        }

        p->updatePreview();
    }

    QString apply() {
        QStringList result;
        result.reserve(widgetList.size());
        for (TokenWidget *widget : const_cast<const QList<TokenWidget *> &>(widgetList))
            result << widget->toString();

        return result.join(QStringLiteral("|"));
    }

    void addToken(const IdSuggestionsEditWidgetPrivate::TokenType cmd, const IdSuggestionsEditWidgetPrivate::Location location)
    {
        if (location == IdSuggestionsEditWidgetPrivate::locAtTop) {
            add(cmd, IdSuggestionsEditWidgetPrivate::locAtTop);
            QTimer::singleShot(50, p, [this]() {
                area->ensureWidgetVisible(buttonAddTokenAtTop);
            });
        } else if (location == IdSuggestionsEditWidgetPrivate::locAtBottom) {
            add(cmd, IdSuggestionsEditWidgetPrivate::locAtBottom);
            QTimer::singleShot(50, p, [this]() {
                area->ensureWidgetVisible(buttonAddTokenAtBottom);
            });
        }
        p->updatePreview();
    }

    void moveUpToken(TokenWidget *tokenWidget)
    {
        const int curPos = widgetList.indexOf(tokenWidget);
        if (curPos > 0) {
            widgetList.removeAt(curPos);
            const int layoutPos = containerLayout->indexOf(tokenWidget);
            containerLayout->removeWidget(tokenWidget);
            widgetList.insert(curPos - 1, tokenWidget);
            containerLayout->insertWidget(layoutPos - 1, tokenWidget, 1);
            p->updatePreview();
        }
    }

    void moveDownToken(TokenWidget *tokenWidget)
    {
        const int curPos = widgetList.indexOf(tokenWidget);
        if (curPos < widgetList.size() - 1) {
            widgetList.removeAt(curPos);
            const int layoutPos = containerLayout->indexOf(tokenWidget);
            containerLayout->removeWidget(tokenWidget);
            widgetList.insert(curPos + 1, tokenWidget);
            containerLayout->insertWidget(layoutPos + 1, tokenWidget, 1);
            p->updatePreview();
        }
    }

    void removeToken(TokenWidget *tokenWidget)
    {
        widgetList.removeOne(tokenWidget);
        containerLayout->removeWidget(tokenWidget);
        tokenWidget->deleteLater();
        p->updatePreview();
    }
};


IdSuggestionsEditWidget::IdSuggestionsEditWidget(const Entry *previewEntry, QWidget *parent, Qt::WindowFlags f)
        : QWidget(parent, f), IdSuggestions(), d(new IdSuggestionsEditWidgetPrivate(previewEntry, this))
{
    /// nothing
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
    d->labelPreview->setToolTip(i18n("<qt>Structure:<ul><li>%1</li></ul>Example: %2</qt>", formatStrToHuman(formatString).join(QStringLiteral("</li><li>")), formatId(*d->previewEntry, formatString)));
}

IdSuggestionsEditDialog::IdSuggestionsEditDialog(QWidget *parent, Qt::WindowFlags flags)
        : QDialog(parent, flags)
{
    setWindowTitle(i18n("Edit Id Suggestion"));
}

IdSuggestionsEditDialog::~IdSuggestionsEditDialog()
{
    /// nothing
}

QString IdSuggestionsEditDialog::editSuggestion(const Entry *previewEntry, const QString &suggestion, QWidget *parent)
{
    QPointer<IdSuggestionsEditDialog> dlg = new IdSuggestionsEditDialog(parent);
    QBoxLayout *boxLayout = new QVBoxLayout(dlg);
    IdSuggestionsEditWidget *widget = new IdSuggestionsEditWidget(previewEntry, dlg);
    boxLayout->addWidget(widget);
    QDialogButtonBox *dbb = new QDialogButtonBox(dlg);
    dbb->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    boxLayout->addWidget(dbb);
    connect(dbb->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);
    connect(dbb->button(QDialogButtonBox::Cancel), &QPushButton::clicked, dlg.data(), &QDialog::reject);

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

#include "settingsidsuggestionseditor.moc"
