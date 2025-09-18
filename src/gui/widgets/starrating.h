/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_STARRATING_H
#define KBIBTEX_GUI_STARRATING_H

#include <QWidget>

#include <KRatingPainter>

#include <Value>

#include "kbibtexgui_export.h"

class QLabel;
class QPaintEvent;
class QMouseEvent;
class QPushButton;

class KBIBTEXGUI_EXPORT StarRatingPainter : public KRatingPainter
{
public:
    static const int numberOfStars;

    explicit StarRatingPainter();

    void paint(QPainter *painter, const QRect &rect, double percent, double hoverPercent = -1.0);

    static double roundToNearestHalfStarPercent(double percent);
};

/**
 * A widget which shows a number of stars in a horizontal row.
 * A floating-point value between 0.0 and n (n=number of stars) can be
 * assigned to this widget; based on this value, the corresponding
 * number of stars on the left side will be colored golden, the stars
 * on the right side will be shown in grey.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT StarRating : public QWidget
{
    Q_OBJECT

public:
    /**
     * Create a star rating widget with a given number of stars.
     *
     * @param maxNumberOfStars number of stars (recommended value is 8)
     * @param parent parent widget
     */
    explicit StarRating(QWidget *parent = nullptr);
    ~StarRating();

    /**
     * Get the current rating in percent (i.e >=0.0 and <=100.0).
     * If no rating has been set (e.g. by a previous call of @see unsetValue),
     * the return value will be negative.
     * @return either percent between 0.0 and 100.0, or a negative value
     */
    double value() const;

    /**
     * Set the rating in percent (valid only >=0.0 and <=100.0).
     * @param percent value between 0.0 and 100.0
     */
    void setValue(double percent);

    /**
     * Remove any value assigned to this widget.
     * No stars will be highlighted and some "no value set" text
     * will be shown.
     * @see value will return a negative value.
     */
    void unsetValue();

    /**
     * Set this widget in read-only or read-writeable mode.
     * @param isReadOnly @c true if widget is to be read-only, @c false if modifiable
     */
    void setReadOnly(bool isReadOnly);

Q_SIGNALS:
    void modified();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void clear();
    void buttonHeight();

private:
    class Private;
    Private *const d;
};

/**
 * A specialization of @see StarRating mimicking a FieldInput widget.
 * As part of this specialization, @see apply and @see reset functions
 * to write to or read from a Value object.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT StarRatingFieldInput : public StarRating
{
    Q_OBJECT

public:
    explicit StarRatingFieldInput(QWidget *parent = nullptr)
            : StarRating(parent) {
        /* nothing */
    }

    /**
     * Set this widget's state based on the provided Value object
     * @param value Value object to evaluate
     * @return @c true if the Value could be interpreted into a star rating, @c false otherwise
     */
    bool reset(const Value &value);

    /**
     * Write this widget's value into the provided Value object.
     * @param value Value object to modify
     * @return @c true if the apply operation succeeded, @c false otherwise (should not happen)
     */
    bool apply(Value &value) const;

    bool validate(QWidget **widgetWithIssue, QString &message) const;
};

#endif // KBIBTEX_GUI_STARRATING_H
