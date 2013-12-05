/*****************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#ifndef KBIBTEX_GUI_STARRATING_H
#define KBIBTEX_GUI_STARRATING_H

#include "kbibtexgui_export.h"

#include <QWidget>

#include <KIconLoader>

#include "value.h"

class QLabel;
class QPaintEvent;
class QMouseEvent;

class KPushButton;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT StarRating : public QWidget
{
    Q_OBJECT
public:
    static const int defaultMaxNumberOfStars;
    static const double UnsetStarsValue;

    explicit StarRating(int maxNumberOfStars = defaultMaxNumberOfStars, QWidget *parent = NULL);

    double value() const;
    void setValue(double percent);

    void setReadOnly(bool isReadOnly);

    static void paintStars(QPainter *painter, KIconLoader::States defaultState, int numTotalStars, double percent, const QRect &inside);
    static double percentForPosition(const QPoint &pos, int numTotalStars, const QRect &inside);

signals:
    void modified();

protected:
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void leaveEvent(QEvent *);

private slots:
    void clear();
    void buttonHeight();

private:
    static const int paintMargin;
    int spacing;

    bool m_isReadOnly;
    double m_percent;
    int m_maxNumberOfStars;
    const QString m_unsetStarsText;
    QLabel *m_labelPercent;
    KPushButton *m_clearButton;
    QPoint m_mouseLocation;

    QRect starsInside() const;
};

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT StarRatingFieldInput : public StarRating
{
public:
    explicit StarRatingFieldInput(int maxNumberOfStars = defaultMaxNumberOfStars, QWidget *parent = NULL)
            : StarRating(maxNumberOfStars, parent) {
        /* nothing */
    }

    bool reset(const Value &value);
    bool apply(Value &value) const;
};

#endif // KBIBTEX_GUI_STARRATING_H
