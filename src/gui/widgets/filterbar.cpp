/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include <QLayout>
#include <QLabel>
#include <QTimer>

#include <KPushButton>
#include <KComboBox>
#include <KLocale>
#include <KLineEdit>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KIcon>

#include "filterbar.h"
#include "bibtexfields.h"
#include "delayedexecutiontimer.h"

class FilterBar::FilterBarPrivate
{
private:
    FilterBar *p;

public:
    KSharedConfigPtr config;
    const QString configGroupName;

    KComboBox *comboBoxFilterText;
    const int maxNumStoredFilterTexts;
    KComboBox *comboBoxCombination;
    KComboBox *comboBoxField;
    KPushButton *buttonSearchPDFfiles;
    KPushButton *buttonClearAll;
    DelayedExecutionTimer *delayedTimer;

    FilterBarPrivate(FilterBar *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
          configGroupName(QLatin1String("Filter Bar")), maxNumStoredFilterTexts(12) {
        delayedTimer = new DelayedExecutionTimer(p);
        connect(delayedTimer, SIGNAL(triggered()), p, SLOT(publishFilter()));
    }

    ~FilterBarPrivate() {
        delete delayedTimer;
    }

    SortFilterBibTeXFileModel::FilterQuery filter() {
        SortFilterBibTeXFileModel::FilterQuery result;
        result.combination = comboBoxCombination->currentIndex() == 0 ? SortFilterBibTeXFileModel::AnyTerm : SortFilterBibTeXFileModel::EveryTerm;
        result.terms.clear();
        if (comboBoxCombination->currentIndex() == 2) /// exact phrase
            result.terms << comboBoxFilterText->lineEdit()->text();
        else /// any or every word
            result.terms = comboBoxFilterText->lineEdit()->text().split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts);
        result.field = comboBoxField->currentIndex() == 0 ? QString::null : comboBoxField->itemData(comboBoxField->currentIndex(), Qt::UserRole).toString();
        result.searchPDFfiles = buttonSearchPDFfiles->isChecked();

        return result;
    }

    void setFilter(SortFilterBibTeXFileModel::FilterQuery fq) {
        /// Avoid triggering loops of activation
        comboBoxCombination->blockSignals(true);
        /// Set check state for action for either "any word",
        /// "every word", or "exact phrase", respectively
        const int combinationIndex = fq.combination == SortFilterBibTeXFileModel::AnyTerm ? 0 : (fq.terms.count() < 2 ? 2 : 1);
        comboBoxCombination->setCurrentIndex(combinationIndex);
        /// Reset activation block
        comboBoxCombination->blockSignals(false);


        /// Avoid triggering loops of activation
        comboBoxField->blockSignals(true);
        /// Find and check action that corresponds to field name ("author", ...)
        const QString lower = fq.field.toLower();
        for (int idx = comboBoxField->count() - 1; idx >= 0; --idx) {
            const QString lower = fq.field.toLower();
            if (comboBoxField->itemData(idx, Qt::UserRole).toString().toLower() == lower) {
                comboBoxField->setCurrentIndex(idx);
                break;
            }
        }
        /// Reset activation block
        comboBoxField->blockSignals(false);

        /// Avoid triggering loops of activation
        buttonSearchPDFfiles->blockSignals(true);
        /// Set flag if associated PDF files have to be searched
        buttonSearchPDFfiles->setChecked(fq.searchPDFfiles);
        /// Reset activation block
        buttonSearchPDFfiles->blockSignals(false);

        /// Avoid triggering loops of activation
        comboBoxFilterText->lineEdit()->blockSignals(true);
        /// Set filter text widget's content
        comboBoxFilterText->lineEdit()->setText(fq.terms.join(" "));
        /// Reset activation block
        comboBoxFilterText->lineEdit()->blockSignals(false);
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
        QStringList completionListDate = configGroup.readEntry(QLatin1String("PreviousSearches"), QStringList());
        for (QStringList::Iterator it = completionListDate.begin(); it != completionListDate.end();)
            if ((*it).mid(12) == text)
                it = completionListDate.erase(it);
            else
                ++it;
        completionListDate << (QDateTime::currentDateTime().toString("yyyyMMddhhmm") + text);

        /// after sorting, discard all but the maxNumStoredFilterTexts most
        /// recent user-entered filter texts
        completionListDate.sort();
        while (completionListDate.count() > maxNumStoredFilterTexts)
            completionListDate.removeFirst();

        configGroup.writeEntry(QLatin1String("PreviousSearches"), completionListDate);
        config->sync();

        /// add user-entered filter text to combobox's drop-down list
        if (!comboBoxFilterText->contains(text))
            comboBoxFilterText->addItem(text);
    }

    void storeComboBoxStatus() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("CurrentCombination"), comboBoxCombination->currentIndex());
        configGroup.writeEntry(QLatin1String("CurrentField"), comboBoxField->currentIndex());
        configGroup.writeEntry(QLatin1String("SearchPDFFiles"), buttonSearchPDFfiles->isChecked());
        config->sync();
    }

    void restoreState() {
        KConfigGroup configGroup(config, configGroupName);
        comboBoxCombination->setCurrentIndex(configGroup.readEntry(QLatin1String("CurrentCombination"), 0));
        comboBoxField->setCurrentIndex(configGroup.readEntry(QLatin1String("CurrentField"), 0));
        buttonSearchPDFfiles->setChecked(configGroup.readEntry(QLatin1String("SearchPDFFiles"), false));
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
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(1);

    layout->addSpacing(4);

    QLabel *label = new QLabel(i18n("Filter:"), this);
    layout->addWidget(label, 0);

    d->comboBoxFilterText = new KComboBox(true, this);
    label->setBuddy(d->comboBoxFilterText);
    setFocusProxy(d->comboBoxFilterText);
    layout->addWidget(d->comboBoxFilterText, 5);
    d->comboBoxFilterText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    d->comboBoxFilterText->setEditable(true);
    QFontMetrics metrics(d->comboBoxFilterText->font());
    d->comboBoxFilterText->setMinimumWidth(metrics.width(QLatin1String("AIWaiw")) * 7);
    KLineEdit *lineEdit = static_cast<KLineEdit *>(d->comboBoxFilterText->lineEdit());
    lineEdit->setClearButtonShown(true);

    d->comboBoxCombination = new KComboBox(false, this);
    layout->addWidget(d->comboBoxCombination, 1);
    d->comboBoxCombination->addItem(i18n("any word")); /// AnyWord=0
    d->comboBoxCombination->addItem(i18n("every word")); /// EveryWord=1
    d->comboBoxCombination->addItem(i18n("exact phrase")); /// ExactPhrase=2
    d->comboBoxCombination->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    d->comboBoxField = new KComboBox(false, this);
    layout->addWidget(d->comboBoxField, 1);
    d->comboBoxField->addItem(i18n("any field"), QVariant());
    d->comboBoxField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    foreach(const FieldDescription *fd, *BibTeXFields::self()) {
        if (fd->upperCamelCaseAlt.isEmpty())
            d->comboBoxField->addItem(fd->label, fd->upperCamelCase);
    }

    d->buttonSearchPDFfiles = new KPushButton(this);
    d->buttonSearchPDFfiles->setIcon(KIcon("application-pdf"));
    d->buttonSearchPDFfiles->setToolTip(i18n("Include PDF files in full-text search"));
    d->buttonSearchPDFfiles->setCheckable(true);
    layout->addWidget(d->buttonSearchPDFfiles, 0);

    d->buttonClearAll = new KPushButton(this);
    d->buttonClearAll->setIcon(KIcon("edit-clear-locationbar-rtl"));
    d->buttonClearAll->setToolTip(i18n("Reset filter critera"));
    layout->addWidget(d->buttonClearAll, 0);

    layout->addSpacing(4);

    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(textChanged(QString)), d->delayedTimer, SLOT(trigger()));
    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(returnPressed()), this, SLOT(userPressedEnter()));
    connect(d->comboBoxCombination, SIGNAL(currentIndexChanged(int)), this, SLOT(comboboxStatusChanged()));
    connect(d->comboBoxField, SIGNAL(currentIndexChanged(int)), this, SLOT(comboboxStatusChanged()));
    connect(d->buttonSearchPDFfiles, SIGNAL(toggled(bool)), this, SLOT(comboboxStatusChanged()));
    connect(d->comboBoxCombination, SIGNAL(currentIndexChanged(int)), d->delayedTimer, SLOT(trigger()));
    connect(d->comboBoxField, SIGNAL(currentIndexChanged(int)), d->delayedTimer, SLOT(trigger()));
    connect(d->buttonSearchPDFfiles, SIGNAL(toggled(bool)), d->delayedTimer, SLOT(trigger()));
    connect(d->buttonClearAll, SIGNAL(clicked()), this, SLOT(resetState()));

    /// restore history on filter texts
    /// see addCompletionString for more detailed explanation
    KConfigGroup configGroup(d->config, d->configGroupName);
    QStringList completionListDate = configGroup.readEntry(QLatin1String("PreviousSearches"), QStringList());
    for (QStringList::Iterator it = completionListDate.begin(); it != completionListDate.end(); ++it)
        d->comboBoxFilterText->addItem((*it).mid(12));
    d->comboBoxFilterText->lineEdit()->setText(QLatin1String(""));
    d->comboBoxCombination->setCurrentIndex(configGroup.readEntry("CurrentCombination", 0));
    d->comboBoxField->setCurrentIndex(configGroup.readEntry("CurrentField", 0));

    d->restoreState();

    QTimer::singleShot(250, this, SLOT(buttonHeight()));
}

FilterBar::~FilterBar()
{
    delete d;
}

void FilterBar::setFilter(SortFilterBibTeXFileModel::FilterQuery fq)
{
    d->setFilter(fq);
    emit filterChanged(fq);
}

SortFilterBibTeXFileModel::FilterQuery FilterBar::filter()
{
    return d->filter();
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
