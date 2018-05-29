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

#ifndef KBIBTEX_GUI_RANGEWIDGET_H
#define KBIBTEX_GUI_RANGEWIDGET_H

#include "kbibtexgui_export.h"

#include <QWidget>

/**
 * Allows the user to specify a minimum and a maximum out of a range of values.
 * Values are provided as a QStringList for visualization in two QComboBox lists
 * (one of minimum, one for maximum). Programmatically, the range of values
 * goes from 0 to QStringList's size - 1. The two QComboBox's values shown to
 * the user are interlinked, so that for the 'minimum' QComboBox no values above
 * the 'maximum' QComboBox's current value are available; vice versa, the
 * 'maximum' QComboBox shows no values below the 'minimum' QComboBox's current
 * value.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT RangeWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int lowerValue READ lowerValue WRITE setLowerValue NOTIFY lowerValueChanged)
    Q_PROPERTY(int upperValue READ upperValue WRITE setUpperValue NOTIFY upperValueChanged)

public:
    /**
     * @brief RangeWidget
     * @param values List of alternatives shown in the QComboBoxes
     * @param parent QWidget's parent
     */
    explicit RangeWidget(const QStringList &values, QWidget *parent = nullptr);
    ~RangeWidget();

    /**
      * Returns the maximum value of this RangeWidget.
      * Will always be the last index of the QStringList passed at construction,
      * i.e. QStringList's size - 1
      * @return maximum range value
      */
    int maximum() const;

    /**
    * Sets the lower value. The value will be normalized to fit the following
    * criteria: (1) at least as large as the minimum value (=0), (2) at most
    * as large as the maximum value, (3) not larger than the upper value.
    * @param newLowerValue new lower value of the range
    */
    void setLowerValue(int newLowerValue);

    /**
     * Returns the current lower value
     * @return lower value of the range
     */
    int lowerValue() const;

    /**
    * Sets the upper value. The value will be normalized to fit the following
    * criteria: (1) at least as large as the minimum value (=0), (2) at most
    * as large as the maximum value, (3) not smaller than the lower value.
    * @param newUpperValue new upper value of the range
    */
    void setUpperValue(int newUpperValue);

    /**
     * Returns the current upper value
     * @return upper value of the range
     */
    int upperValue() const;

signals:
    /**
     * Signal notifying about the change of the lower value. This signal
     * will not be triggered if the value is set to the same value it
     * already has.
     */
    void lowerValueChanged(int);

    /**
     * Signal notifying about the change of the upper value. This signal
     * will not be triggered if the value is set to the same value it
     * already has.
     */
    void upperValueChanged(int);

private slots:
    void lowerComboBoxChanged(int);
    void upperComboBoxChanged(int);

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_GUI_RANGEWIDGET_H
