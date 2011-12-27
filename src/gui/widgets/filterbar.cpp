/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <QFontMetrics>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>

#include <KComboBox>
#include <KLocale>
#include <KLineEdit>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KIcon>

#include "filterbar.h"
#include "bibtexfields.h"

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
    QCheckBox *checkboxSearchPDFfiles;
    QTimer *filterUpdateTimer;

    FilterBarPrivate(FilterBar *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
            configGroupName(QLatin1String("Filter Bar")), maxNumStoredFilterTexts(12), filterUpdateTimer(new QTimer(parent)) {
        connect(filterUpdateTimer, SIGNAL(timeout()), p, SLOT(timerTriggered()));
    }

    void clearFilter() {
        comboBoxCombination->blockSignals(true);
        comboBoxField->blockSignals(true);

        comboBoxFilterText->lineEdit()->setText("");
        comboBoxCombination->setCurrentIndex(0);
        comboBoxField->setCurrentIndex(0);

        comboBoxCombination->blockSignals(false);
        comboBoxField->blockSignals(false);

        checkboxSearchPDFfiles->setChecked(false);
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
        result.searchPDFfiles = checkboxSearchPDFfiles->isChecked();

        return result;
    }

    void setFilter(SortFilterBibTeXFileModel::FilterQuery fq) {
        comboBoxCombination->blockSignals(true);
        comboBoxField->blockSignals(true);

        comboBoxCombination->setCurrentIndex(fq.combination == SortFilterBibTeXFileModel::AnyTerm ? 0 : (fq.terms.count() < 2 ? 2 : 1));
        comboBoxFilterText->lineEdit()->setText(fq.terms.join(" "));
        for (int idx = 0; idx < comboBoxField->count(); ++idx) {
            const QString lower = fq.field.toLower();
            if (lower == comboBoxField->itemText(idx).toLower() || comboBoxField->itemData(idx, Qt::UserRole).toString().toLower() == lower) {
                comboBoxField->setCurrentIndex(idx);
                break;
            }
        }
        checkboxSearchPDFfiles->setChecked(fq.searchPDFfiles);

        comboBoxCombination->blockSignals(false);
        comboBoxField->blockSignals(false);
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
        configGroup.writeEntry(QLatin1String("SearchPDFFiles"), checkboxSearchPDFfiles->isChecked());
        config->sync();
    }

    void resetFilterUpdateTimer() {
        filterUpdateTimer->stop();
        filterUpdateTimer->start(500);
    }
};

FilterBar::FilterBar(QWidget *parent)
        : QWidget(parent), d(new FilterBarPrivate(this))
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);
    layout->setRowStretch(2, 1);

    QLabel *label = new QLabel(i18n("Filter:"), this);
    layout->addWidget(label, 1, 0);

    d->comboBoxFilterText = new KComboBox(true, this);
    label->setBuddy(d->comboBoxFilterText);
    setFocusProxy(d->comboBoxFilterText);
    layout->addWidget(d->comboBoxFilterText, 1, 1);
    d->comboBoxFilterText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    d->comboBoxFilterText->setEditable(true);
    QFontMetrics metrics(d->comboBoxFilterText->font());
    d->comboBoxFilterText->setMinimumWidth(metrics.width(QLatin1String("AIWaiw"))*7);
    KLineEdit *lineEdit = static_cast<KLineEdit*>(d->comboBoxFilterText->lineEdit());
    lineEdit->setClearButtonShown(true);

    d->comboBoxCombination = new KComboBox(false, this);
    layout->addWidget(d->comboBoxCombination, 1, 2);
    d->comboBoxCombination->addItem(i18n("any word")); /// AnyWord=0
    d->comboBoxCombination->addItem(i18n("every word")); /// EveryWord=1
    d->comboBoxCombination->addItem(i18n("exact phrase")); /// ExactPhrase=2
    d->comboBoxCombination->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    d->comboBoxField = new KComboBox(false, this);
    layout->addWidget(d->comboBoxField, 1, 3);
    d->comboBoxField->addItem(i18n("every field"), QVariant());
    d->comboBoxField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    foreach(const FieldDescription &fd,  *BibTeXFields::self()) {
        if (fd.upperCamelCaseAlt.isEmpty())
            d->comboBoxField->addItem(fd.label, fd.upperCamelCase);
    }

    d->checkboxSearchPDFfiles = new QCheckBox(this);
    d->checkboxSearchPDFfiles->setIcon(KIcon("application-pdf"));
    layout->addWidget(d->checkboxSearchPDFfiles, 1, 4);

    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(lineeditTextChanged()));
    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(returnPressed()), this, SLOT(lineeditReturnPressed()));
    connect(lineEdit, SIGNAL(clearButtonClicked()), this, SLOT(clearFilter()));
    connect(d->comboBoxCombination, SIGNAL(currentIndexChanged(int)), this, SLOT(comboboxStatusChanged()));
    connect(d->comboBoxField, SIGNAL(currentIndexChanged(int)), this, SLOT(comboboxStatusChanged()));
    connect(d->checkboxSearchPDFfiles, SIGNAL(toggled(bool)), this, SLOT(comboboxStatusChanged()));

    /// restore history on filter texts
    /// see addCompletionString for more detailed explanation
    KConfigGroup configGroup(d->config, QLatin1String("FilterBar"));
    QStringList completionListDate = configGroup.readEntry(QLatin1String("PreviousSearches"), QStringList());
    for (QStringList::Iterator it = completionListDate.begin(); it != completionListDate.end(); ++it)
        d->comboBoxFilterText->addItem((*it).mid(12));
    d->comboBoxCombination->setCurrentIndex(configGroup.readEntry("CurrentCombination", 0));
    d->comboBoxField->setCurrentIndex(configGroup.readEntry("CurrentField", 0));
}

void FilterBar::clearFilter()
{
    d->clearFilter();
    emit filterChanged(d->filter());
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

void FilterBar::lineeditTextChanged()
{
    d->resetFilterUpdateTimer();
}

void FilterBar::comboboxStatusChanged()
{
    d->filterUpdateTimer->stop();
    d->storeComboBoxStatus();
    emit filterChanged(d->filter());
}

void FilterBar::lineeditReturnPressed()
{
    d->filterUpdateTimer->stop();
    /// only store text in auto-completion if user pressed enter
    d->addCompletionString(d->comboBoxFilterText->lineEdit()->text());
    emit filterChanged(d->filter());
}

void FilterBar::timerTriggered()
{
    /// timer used for a delayed filter update
    emit filterChanged(d->filter());
}
