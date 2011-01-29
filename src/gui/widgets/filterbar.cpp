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

#include <KComboBox>
#include <KLocale>
#include <KLineEdit>

#include "filterbar.h"
#include "bibtexfields.h"

class FilterBar::FilterBarPrivate
{
private:
    FilterBar *p;

public:
    KComboBox *comboBoxFilterText;
    KComboBox *comboBoxCombination;
    KComboBox *comboBoxField;

    FilterBarPrivate(FilterBar *parent)
            :   p(parent) {
        // nothing
    }

    void clearFilter() {
        comboBoxFilterText->lineEdit()->setText("");
        comboBoxCombination->setCurrentIndex(0);
        comboBoxField->setCurrentIndex(0);
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

        return result;
    }

    void setFilter(SortFilterBibTeXFileModel::FilterQuery fq) {
        comboBoxCombination->setCurrentIndex(fq.combination == SortFilterBibTeXFileModel::AnyTerm ? 0 : (fq.terms.count() < 2 ? 2 : 1));
        comboBoxFilterText->lineEdit()->setText(fq.terms.join(" "));
        for (int idx = 0; idx < comboBoxField->count();++idx)
            if (fq.field == comboBoxField->itemText(idx) || comboBoxField->itemData(idx, Qt::UserRole).toString() == fq.field) {
                comboBoxField->setCurrentIndex(idx);
                break;
            }
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
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>(d->comboBoxFilterText->lineEdit());
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

    BibTeXFields *bibTeXFiles = BibTeXFields::self();
    for (BibTeXFields::Iterator it = bibTeXFiles->begin(); it != bibTeXFiles->end(); ++it)
        if ((*it).upperCamelCaseAlt.isEmpty())
            d->comboBoxField->addItem((*it).label, (*it).upperCamelCase);

    connect(d->comboBoxFilterText->lineEdit(), SIGNAL(returnPressed()), this, SLOT(widgetsChanged()));
    connect(lineEdit, SIGNAL(clearButtonClicked()), this, SLOT(clearFilter()));
    connect(d->comboBoxCombination, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetsChanged()));
    connect(d->comboBoxField, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetsChanged()));
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

void FilterBar::widgetsChanged()
{
    emit filterChanged(d->filter());
}
