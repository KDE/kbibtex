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

#include <KComboBox>
#include <KLocale>
#include <KLineEdit>

#include "filterbar.h"

using namespace KBibTeX::GUI::Widgets;

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
};

FilterBar::FilterBar(QWidget *parent)
        : QWidget(parent), d(new FilterBarPrivate(this))
{
    QLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    d->comboBoxFilterText = new KComboBox(true, this);
    layout->addWidget(d->comboBoxFilterText);
    d->comboBoxFilterText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    d->comboBoxFilterText->setEditable(true);
    dynamic_cast<KLineEdit*>(d->comboBoxFilterText->lineEdit())->setClearButtonShown(true);

    d->comboBoxCombination = new KComboBox(false, this);
    layout->addWidget(d->comboBoxCombination);
    d->comboBoxCombination->addItem(i18n("any word"));
    d->comboBoxCombination->addItem(i18n("every word"));
    d->comboBoxCombination->addItem(i18n("exact phrase"));
    d->comboBoxCombination->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    d->comboBoxField = new KComboBox(false, this);
    layout->addWidget(d->comboBoxField);
    d->comboBoxField->addItem(i18n("every field"));
    d->comboBoxField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    setEnabled(false);
}
