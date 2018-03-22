/*****************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#include "rangewidget.h"

#include <QComboBox>
#include <QLineEdit>
#include <QBoxLayout>
#include <QStringListModel>
#include <QLabel>

#include <KLocalizedString>

class RangeWidget::Private
{
public:
    enum TextAlternative {LowerAlternativ, UpperAlternative};

    const QStringList values;
    int lowerValue, upperValue;
    QComboBox *lowerComboBox, *upperComboBox;

    Private(const QStringList &_values, RangeWidget *parent)
            : values(_values), lowerValue(0), upperValue(_values.size() - 1)
    {
        Q_UNUSED(parent)

        QBoxLayout *layout = new QHBoxLayout(parent);
        layout->setMargin(0);

        lowerComboBox = new QComboBox(parent);
        layout->addWidget(lowerComboBox, 1, Qt::AlignCenter);
        lowerComboBox->setModel(new QStringListModel(lowerComboBox));

        QLabel *label = new QLabel(QChar(0x22ef), parent);
        layout->addWidget(label, 0, Qt::AlignCenter);

        upperComboBox = new QComboBox(parent);
        layout->addWidget(upperComboBox, 1, Qt::AlignCenter);
        upperComboBox->setModel(new QStringListModel(upperComboBox));

        layout->addStretch(100); ///< left-align this widget's child widgets

        adjustComboBoxes();

        connect(lowerComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), parent, &RangeWidget::lowerComboBoxChanged);
        connect(upperComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), parent, &RangeWidget::upperComboBoxChanged);
    }

    QStringList stringListRange(const QStringList &originalList, int min, int max, const TextAlternative textAlternative) {
        if (originalList.isEmpty()) return QStringList();

        QStringList result;
        for (int i = qMin(originalList.size() - 1, qMin(max, qMax(0, min))); i <= qMin(originalList.size() - 1, qMax(min, qMax(0, max))); ++i) {
            const QStringList alternatives = originalList[i].split(QStringLiteral("|"));
            Q_ASSERT_X(alternatives.size() >= 1 && alternatives.size() <= 2, "RangeWidget::Private::stringListRange", "Either one or two alternatives must be given");
            const int alternativeIndex = alternatives.size() == 1 || textAlternative == LowerAlternativ ? 0 : 1;
            if (!alternatives[alternativeIndex].isEmpty())
                result.append(alternatives[alternativeIndex]);
        }

        return result;
    }

    void adjustComboBoxes() {
        const int minimum = 0, maximum = values.size() - 1;
        Q_ASSERT_X(minimum <= lowerValue, "RangeWidget::Private::adjustSpinBoxes", "minimum<=lowerValue");
        Q_ASSERT_X(lowerValue <= upperValue, "RangeWidget::Private::adjustSpinBoxes", "lowerValue<=upperValue");
        Q_ASSERT_X(upperValue <= maximum, "RangeWidget::Private::adjustSpinBoxes", "upperValue<=maximum");

        /// Disable signals being emitted when the combo boxes get updated
        const bool previousBlockSignalsValueLower = lowerComboBox->blockSignals(true);
        const bool previousBlockSignalsValueUpper = upperComboBox->blockSignals(true);

        /// Compute a temporary QStringList containing only values from minimum to current upper value
        const QStringList lowerValues = stringListRange(values, minimum, upperValue, LowerAlternativ);
        qobject_cast<QStringListModel *>(lowerComboBox->model())->setStringList(lowerValues);
        lowerComboBox->setCurrentIndex(lowerValue);

        /// Compute a temporary QStringList containing only values from current lower value to maximum
        const QStringList upperValues = stringListRange(values, lowerValue, maximum, UpperAlternative);
        qobject_cast<QStringListModel *>(upperComboBox->model())->setStringList(upperValues);
        upperComboBox->setCurrentIndex(upperValue - lowerValue);

        /// Re-enable signal for the combo boxes
        lowerComboBox->blockSignals(previousBlockSignalsValueLower);
        upperComboBox->blockSignals(previousBlockSignalsValueUpper);
    }
};

RangeWidget::RangeWidget(const QStringList &values, QWidget *parent)
        : QWidget(parent), d(new Private(values, this))
{
    /// nothing
}

int RangeWidget::maximum() const
{
    return d->values.size() - 1;
}

void RangeWidget::setLowerValue(int newLowerValue)
{
    newLowerValue = qMin(qMax(qMin(newLowerValue, d->values.size() - 1), 0), d->upperValue);
    if (newLowerValue != d->lowerValue) {
        d->lowerValue = newLowerValue;
        emit lowerValueChanged(d->lowerValue);
        d->adjustComboBoxes();
    }
}

int RangeWidget::lowerValue() const
{
    return d->lowerValue;
}

void RangeWidget::setUpperValue(int newUpperValue)
{
    newUpperValue = qMax(qMax(qMin(newUpperValue, d->values.size() - 1), 0), d->lowerValue);
    if (newUpperValue != d->upperValue) {
        d->upperValue = newUpperValue;
        emit upperValueChanged(d->upperValue);
        d->adjustComboBoxes();
    }
}

int RangeWidget::upperValue() const
{
    return d->upperValue;
}

void RangeWidget::lowerComboBoxChanged(int spinboxLowerValue)
{
    const int newLowerValue = spinboxLowerValue;
    if (newLowerValue != d->lowerValue) {
        d->lowerValue = newLowerValue;
        emit lowerValueChanged(d->lowerValue);
        d->adjustComboBoxes();
    }
}

void RangeWidget::upperComboBoxChanged(int spinboxUpperValue)
{
    const int newUpperValue = spinboxUpperValue + d->lowerValue;
    if (newUpperValue != d->upperValue) {
        d->upperValue = newUpperValue;
        emit upperValueChanged(d->upperValue);
        d->adjustComboBoxes();
    }
}
