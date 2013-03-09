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
#include <QMenu>

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
    KPushButton *buttonOptions;
    DelayedExecutionTimer *delayedTimer;
    QActionGroup *actionGroupCombination, *actionGroupFields;
    QAction *actionSearchPDFfiles;

    FilterBarPrivate(FilterBar *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
          configGroupName(QLatin1String("Filter Bar")), maxNumStoredFilterTexts(12) {
        delayedTimer = new DelayedExecutionTimer(p);
        connect(delayedTimer, SIGNAL(triggered()), p, SLOT(publishFilter()));
    }

    ~FilterBarPrivate() {
        delete delayedTimer;
    }

    QMenu *buildOptionsMenu(QWidget *menuParent) {
        QMenu *result = new QMenu(menuParent);

        QAction *action = result->addAction(i18n("Reset filter"), p, SLOT(resetState()));
        result->addSeparator();

        QMenu *submenu = result->addMenu(i18n("Combination"));
        actionGroupCombination = new QActionGroup(submenu);
        action = submenu->addAction(i18n("Any word"), p, SLOT(stateChanged()));
        action->setCheckable(true);
        action->setChecked(true);
        action->setData(0);
        actionGroupCombination->addAction(action);
        action = submenu->addAction(i18n("Every word"), p, SLOT(stateChanged()));
        action->setCheckable(true);
        action->setData(1);
        actionGroupCombination->addAction(action);
        action = submenu->addAction(i18n("Exact phrase"), p, SLOT(stateChanged()));
        action->setCheckable(true);
        action->setData(2);
        actionGroupCombination->addAction(action);
        connect(actionGroupCombination, SIGNAL(triggered(QAction *)), delayedTimer, SLOT(trigger()));

        submenu = result->addMenu(i18n("Fields"));
        actionGroupFields = new QActionGroup(submenu);
        action = submenu->addAction(i18n("Every field"), p, SLOT(stateChanged()));
        action->setCheckable(true);
        action->setChecked(true);
        action->setData(QVariant());
        actionGroupFields->addAction(action);
        submenu->addSeparator();
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            if (fd->upperCamelCaseAlt.isEmpty()) {
                action = submenu->addAction(fd->label, p, SLOT(stateChanged()));
                action->setCheckable(true);
                action->setData(fd->upperCamelCase);
                actionGroupFields->addAction(action);
            }
        }
        connect(actionGroupFields, SIGNAL(triggered(QAction *)), delayedTimer, SLOT(trigger()));

        actionSearchPDFfiles = result->addAction(KIcon("application-pdf"), i18n("Search PDF files"), p, SLOT(stateChanged()));
        connect(actionSearchPDFfiles, SIGNAL(toggled(bool)), delayedTimer, SLOT(trigger()));
        actionSearchPDFfiles->setCheckable(true);
        actionSearchPDFfiles->setToolTip(i18n("Include PDF files in full-text search"));

        return result;
    }

    SortFilterBibTeXFileModel::FilterQuery filter() {
        SortFilterBibTeXFileModel::FilterQuery result;
        result.combination = actionGroupCombination->checkedAction()->data().toInt() == 0 ? SortFilterBibTeXFileModel::AnyTerm : SortFilterBibTeXFileModel::EveryTerm;
        result.terms.clear();
        if (actionGroupCombination->checkedAction()->data().toInt() == 2)  /// exact phrase
            result.terms << comboBoxFilterText->lineEdit()->text();
        else /// any or every word
            result.terms = comboBoxFilterText->lineEdit()->text().split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts);
        result.field = actionGroupFields->checkedAction()->data().toString();
        result.searchPDFfiles = actionSearchPDFfiles->isChecked();

        return result;
    }

    void setFilter(SortFilterBibTeXFileModel::FilterQuery fq) {
        int combinationIndex = fq.combination == SortFilterBibTeXFileModel::AnyTerm ? 0 : (fq.terms.count() < 2 ? 2 : 1);
        actionGroupCombination->blockSignals(true);
        foreach(QAction *action, actionGroupCombination->actions()) action->blockSignals(true);
        actionGroupCombination->actions()[combinationIndex]->setChecked(true);
        actionGroupCombination->blockSignals(false);
        foreach(QAction *action, actionGroupCombination->actions()) action->blockSignals(false);

        actionGroupFields->blockSignals(true);
        foreach(QAction *action, actionGroupFields->actions()) action->blockSignals(true);
        const QString lower = fq.field.toLower();
        foreach(QAction *action, actionGroupFields->actions()) {
            if (action->text().toLower() == lower || action->data().toString().toLower() == lower) {
                action->setChecked(true);
                break;
            }
        }

        actionGroupFields->blockSignals(false);
        foreach(QAction *action, actionGroupFields->actions()) action->blockSignals(false);

        actionSearchPDFfiles->blockSignals(true);
        actionSearchPDFfiles->setChecked(fq.searchPDFfiles);
        actionSearchPDFfiles->blockSignals(false);

        comboBoxFilterText->blockSignals(true);
        comboBoxFilterText->lineEdit()->setText(fq.terms.join(" "));
        comboBoxFilterText->blockSignals(false);
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

    void storeState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("CurrentCombination"), actionGroupCombination->checkedAction()->data().toInt());
        configGroup.writeEntry(QLatin1String("CurrentField"), actionGroupFields->actions().indexOf(actionGroupFields->checkedAction()));
        configGroup.writeEntry(QLatin1String("SearchPDFFiles"), actionSearchPDFfiles->isChecked());
        config->sync();
    }

    void restoreState() {
        KConfigGroup configGroup(config, configGroupName);
        actionGroupCombination->actions()[ configGroup.readEntry("CurrentCombination", 0)]->setChecked(true);
        actionGroupFields->actions()[configGroup.readEntry("CurrentField", 0)]->setChecked(true);
    }

    void resetState() {
        comboBoxFilterText->lineEdit()->clear();
        actionGroupCombination->actions()[0]->setChecked(true);
        actionGroupFields->actions()[0]->setChecked(true);
        actionSearchPDFfiles->setChecked(false);
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
    d->comboBoxFilterText->setMinimumWidth(metrics.width(QLatin1String("AIWaiw")) * 7);
    KLineEdit *lineEdit = static_cast<KLineEdit *>(d->comboBoxFilterText->lineEdit());
    lineEdit->setClearButtonShown(true);

    d->buttonOptions = new KPushButton(this);
    d->buttonOptions->setIcon(KIcon("configure"));
    d->buttonOptions->setToolTip(i18n("Configure the filter"));
    d->buttonOptions->setMenu(d->buildOptionsMenu(d->buttonOptions));
    layout->addWidget(d->buttonOptions, 1, 2);

    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(textChanged(QString)), d->delayedTimer, SLOT(trigger()));
    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(returnPressed()), this, SLOT(userPressedEnter()));

    /// restore history on filter texts
    /// see addCompletionString for more detailed explanation
    KConfigGroup configGroup(d->config, d->configGroupName);
    QStringList completionListDate = configGroup.readEntry(QLatin1String("PreviousSearches"), QStringList());
    for (QStringList::Iterator it = completionListDate.begin(); it != completionListDate.end(); ++it)
        d->comboBoxFilterText->addItem((*it).mid(12));
    d->comboBoxFilterText->lineEdit()->setText(QLatin1String(""));

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

void FilterBar::stateChanged()
{
    d->actionSearchPDFfiles->setEnabled(d->actionGroupFields->actions()[0] == d->actionGroupFields->checkedAction());
    d->storeState();
}

void FilterBar::resetState()
{
    d->resetState();
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
    QSizePolicy sp = d->buttonOptions->sizePolicy();
    d->buttonOptions->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);
}
