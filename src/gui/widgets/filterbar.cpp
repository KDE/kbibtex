/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "filterbar.h"

#include <algorithm>

#include <QLayout>
#include <QLabel>
#include <QTimer>
#include <QIcon>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>

#include <BibTeXFields>

static bool sortStringsLocaleAware(const QString &s1, const QString &s2) {
    return QString::localeAwareCompare(s1, s2) < 0;
}

class FilterBar::FilterBarPrivate
{
private:
    FilterBar *p;

public:
    KSharedConfigPtr config;
    const QString configGroupName;

    QComboBox *comboBoxFilterText;
    const int maxNumStoredFilterTexts;
    QComboBox *comboBoxCombination;
    QComboBox *comboBoxField;
    QPushButton *buttonSearchPDFfiles;
    QPushButton *buttonClearAll;
    QTimer *delayedTimer;

    FilterBarPrivate(FilterBar *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))),
          configGroupName(QStringLiteral("Filter Bar")), maxNumStoredFilterTexts(12), delayedTimer(new QTimer(parent)) {
        delayedTimer->setSingleShot(true);
        setupGUI();
        connect(delayedTimer, &QTimer::timeout, p, &FilterBar::publishFilter);
    }

    void setupGUI() {
        QBoxLayout *layout = new QHBoxLayout(p);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("Filter:"), p);
        layout->addWidget(label, 0);

        comboBoxFilterText = new QComboBox(p);
        label->setBuddy(comboBoxFilterText);
        layout->addWidget(comboBoxFilterText, 5);
        comboBoxFilterText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        comboBoxFilterText->setEditable(true);
        QFontMetrics metrics(comboBoxFilterText->font());
#if QT_VERSION >= 0x050b00
        comboBoxFilterText->setMinimumWidth(metrics.horizontalAdvance(QStringLiteral("AIWaiw")) * 7);
#else // QT_VERSION >= 0x050b00
        comboBoxFilterText->setMinimumWidth(metrics.width(QStringLiteral("AIWaiw")) * 7);
#endif // QT_VERSION >= 0x050b00
        QLineEdit *lineEdit = static_cast<QLineEdit *>(comboBoxFilterText->lineEdit());
        lineEdit->setClearButtonEnabled(true);
        lineEdit->setPlaceholderText(i18n("Filter bibliographic entries"));

        comboBoxCombination = new QComboBox(p);
        layout->addWidget(comboBoxCombination, 1);
        comboBoxCombination->addItem(i18n("any word")); /// AnyWord=0
        comboBoxCombination->addItem(i18n("every word")); /// EveryWord=1
        comboBoxCombination->addItem(i18n("exact phrase")); /// ExactPhrase=2
        comboBoxCombination->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        comboBoxField = new QComboBox(p);
        layout->addWidget(comboBoxField, 1);
        comboBoxField->addItem(i18n("any field"), QVariant());
        comboBoxField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        /// Use a hash map to get an alphabetically sorted list
        QHash<QString, QString> fielddescs;
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance()))
            if (fd.upperCamelCaseAlt.isEmpty())
                fielddescs.insert(fd.label, fd.upperCamelCase);
        /// Sort locale-aware
        QList<QString> keys = fielddescs.keys();
        std::sort(keys.begin(), keys.end(), sortStringsLocaleAware);
        for (const QString &key : const_cast<const QList<QString> &>(keys)) {
            const QString &value = fielddescs[key];
            comboBoxField->addItem(key, value);
        }

        buttonSearchPDFfiles = new QPushButton(p);
        buttonSearchPDFfiles->setIcon(QIcon::fromTheme(QStringLiteral("application-pdf")));
        buttonSearchPDFfiles->setToolTip(i18n("Include PDF files in full-text search"));
        buttonSearchPDFfiles->setCheckable(true);
        layout->addWidget(buttonSearchPDFfiles, 0);

        buttonClearAll = new QPushButton(p);
        buttonClearAll->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-rtl")));
        buttonClearAll->setToolTip(i18n("Reset filter criteria"));
        layout->addWidget(buttonClearAll, 0);

        /// restore history on filter texts
        /// see addCompletionString for more detailed explanation
        KConfigGroup configGroup(config, configGroupName);
        QStringList completionListDate = configGroup.readEntry(QStringLiteral("PreviousSearches"), QStringList());
        for (QStringList::Iterator it = completionListDate.begin(); it != completionListDate.end(); ++it)
            comboBoxFilterText->addItem((*it).mid(12));
        comboBoxFilterText->lineEdit()->setText(QString());
        comboBoxCombination->setCurrentIndex(configGroup.readEntry("CurrentCombination", 0));
        comboBoxField->setCurrentIndex(configGroup.readEntry("CurrentField", 0));

#define connectStartingDelayedTimer(instance,signal) connect((instance),(signal),p,[this](){delayedTimer->start(500);})
        connectStartingDelayedTimer(comboBoxFilterText->lineEdit(), &QLineEdit::textChanged);
        connect(comboBoxFilterText->lineEdit(), &QLineEdit::returnPressed, p, &FilterBar::userPressedEnter);
        connect(comboBoxCombination, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &FilterBar::comboboxStatusChanged);
        connect(comboBoxField, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &FilterBar::comboboxStatusChanged);
        connect(buttonSearchPDFfiles, &QPushButton::toggled, p, &FilterBar::comboboxStatusChanged);
        connectStartingDelayedTimer(comboBoxCombination, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged));
        connectStartingDelayedTimer(comboBoxField, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged));
        connectStartingDelayedTimer(buttonSearchPDFfiles, &QPushButton::toggled);
        connect(buttonClearAll, &QPushButton::clicked, p, &FilterBar::resetState);
    }

    SortFilterFileModel::FilterQuery filter() {
        SortFilterFileModel::FilterQuery result;
        result.combination = comboBoxCombination->currentIndex() == 0 ? SortFilterFileModel::FilterCombination::AnyTerm : SortFilterFileModel::FilterCombination::EveryTerm;
        result.terms.clear();
        if (comboBoxCombination->currentIndex() == 2) /// exact phrase
            result.terms << comboBoxFilterText->lineEdit()->text();
        else { /// any or every word
            static const QRegularExpression sequenceOfSpacesRegExp(QStringLiteral("\\s+"));
#if QT_VERSION >= 0x050e00
            result.terms = comboBoxFilterText->lineEdit()->text().split(sequenceOfSpacesRegExp, Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
            result.terms = comboBoxFilterText->lineEdit()->text().split(sequenceOfSpacesRegExp, QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
        }
        result.field = comboBoxField->currentIndex() == 0 ? QString() : comboBoxField->itemData(comboBoxField->currentIndex(), Qt::UserRole).toString();
        result.searchPDFfiles = buttonSearchPDFfiles->isChecked();

        return result;
    }

    void setFilter(const SortFilterFileModel::FilterQuery &fq) {
        /// Avoid triggering loops of activation
        QSignalBlocker comboBoxCombinationSignalBlocker(comboBoxCombination);
        /// Set check state for action for either "any word",
        /// "every word", or "exact phrase", respectively
        const int combinationIndex = fq.combination == SortFilterFileModel::FilterCombination::AnyTerm ? 0 : (fq.terms.count() < 2 ? 2 : 1);
        comboBoxCombination->setCurrentIndex(combinationIndex);

        /// Avoid triggering loops of activation
        QSignalBlocker comboBoxFieldSignalBlocker(comboBoxField);
        /// Find and check action that corresponds to field name ("author", ...)
        const QString lower = fq.field.toLower();
        for (int idx = comboBoxField->count() - 1; idx >= 0; --idx) {
            if (comboBoxField->itemData(idx, Qt::UserRole).toString().toLower() == lower) {
                comboBoxField->setCurrentIndex(idx);
                break;
            }
        }

        /// Avoid triggering loops of activation
        QSignalBlocker buttonSearchPDFfilesSignalBlocker(buttonSearchPDFfiles);
        /// Set flag if associated PDF files have to be searched
        buttonSearchPDFfiles->setChecked(fq.searchPDFfiles);

        /// Avoid triggering loops of activation
        QSignalBlocker comboBoxFilterTextSignalBlocker(comboBoxFilterText);
        /// Set filter text widget's content
        comboBoxFilterText->lineEdit()->setText(fq.terms.join(QStringLiteral(" ")));
    }

    bool modelContainsText(QAbstractItemModel *model, const QString &text) {
        for (int row = 0; row < model->rowCount(); ++row)
            if (model->index(row, 0, QModelIndex()).data().toString().contains(text))
                return true;
        return false;
    }

    void addCompletionString(const QString &text) {
        KConfigGroup configGroup(config, configGroupName);

        /// Previous searches are stored as a string list, where each individual
        /// string starts with 12 characters for the date and time when this
        /// search was used. Starting from the 13th character (12th, if you
        /// start counting from 0) the user's input is stored.
        /// This approach has several advantages: It does not require a more
        /// complex data structure, can easily read and written using
        /// KConfigGroup's functions, and can be sorted lexicographically/
        /// chronologically using QStringList's sort.
        /// Disadvantage is that string fragments have to be managed manually.
        QStringList completionListDate = configGroup.readEntry(QStringLiteral("PreviousSearches"), QStringList());
        for (QStringList::Iterator it = completionListDate.begin(); it != completionListDate.end();)
            if ((*it).mid(12) == text)
                it = completionListDate.erase(it);
            else
                ++it;
        completionListDate << (QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmm")) + text);

        /// after sorting, discard all but the maxNumStoredFilterTexts most
        /// recent user-entered filter texts
        completionListDate.sort();
        while (completionListDate.count() > maxNumStoredFilterTexts)
            completionListDate.removeFirst();

        configGroup.writeEntry(QStringLiteral("PreviousSearches"), completionListDate);
        config->sync();

        /// add user-entered filter text to combobox's drop-down list
        if (!text.isEmpty() && !modelContainsText(comboBoxFilterText->model(), text))
            comboBoxFilterText->addItem(text);
    }

    void storeComboBoxStatus() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("CurrentCombination"), comboBoxCombination->currentIndex());
        configGroup.writeEntry(QStringLiteral("CurrentField"), comboBoxField->currentIndex());
        configGroup.writeEntry(QStringLiteral("SearchPDFFiles"), buttonSearchPDFfiles->isChecked());
        config->sync();
    }

    void restoreState() {
        KConfigGroup configGroup(config, configGroupName);
        comboBoxCombination->setCurrentIndex(configGroup.readEntry(QStringLiteral("CurrentCombination"), 0));
        comboBoxField->setCurrentIndex(configGroup.readEntry(QStringLiteral("CurrentField"), 0));
        buttonSearchPDFfiles->setChecked(configGroup.readEntry(QStringLiteral("SearchPDFFiles"), false));
    }

    void resetState() {
        comboBoxFilterText->lineEdit()->clear();
        comboBoxCombination->setCurrentIndex(0);
        comboBoxField->setCurrentIndex(0);
        buttonSearchPDFfiles->setChecked(false);
    }
};

FilterBar::FilterBar(QWidget *parent)
        : QWidget(parent), d(new FilterBarPrivate(this))
{
    d->restoreState();

    setFocusProxy(d->comboBoxFilterText);

    QTimer::singleShot(250, this, &FilterBar::buttonHeight);
}

FilterBar::~FilterBar()
{
    delete d;
}

void FilterBar::setFilter(const SortFilterFileModel::FilterQuery &fq)
{
    d->setFilter(fq);
    emit filterChanged(fq);
}

SortFilterFileModel::FilterQuery FilterBar::filter()
{
    return d->filter();
}

void FilterBar::setPlaceholderText(const QString &msg) {
    QLineEdit *lineEdit = static_cast<QLineEdit *>(d->comboBoxFilterText->lineEdit());
    lineEdit->setPlaceholderText(msg);
}

void FilterBar::comboboxStatusChanged()
{
    d->buttonSearchPDFfiles->setEnabled(d->comboBoxField->currentIndex() == 0);
    d->storeComboBoxStatus();
}

void FilterBar::resetState()
{
    d->resetState();
    emit filterChanged(d->filter());
}

void FilterBar::userPressedEnter()
{
    /// only store text in auto-completion if user pressed enter
    d->addCompletionString(d->comboBoxFilterText->lineEdit()->text());

    publishFilter();
}

void FilterBar::publishFilter()
{
    emit filterChanged(d->filter());
}

void FilterBar::buttonHeight()
{
    QSizePolicy sp = d->buttonSearchPDFfiles->sizePolicy();
    d->buttonSearchPDFfiles->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);
    d->buttonClearAll->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);
}
