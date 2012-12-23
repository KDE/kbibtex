/*****************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
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
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#ifndef KBIBTEX_GUI_STARRATING_H
#define KBIBTEX_GUI_STARRATING_H

#include "kbibtexgui_export.h"

#include <QWidget>

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
    static const int UnsetStarsValue;

    StarRating(int maxNumberOfStars = defaultMaxNumberOfStars, QWidget *parent = NULL);

    int value() const;
    void setValue(int percent);

    void setReadOnly(bool isReadOnly);

signals:
    void modified();

protected:
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *);

private slots:
    void clear();

private:
    bool m_isReadOnly;
    int m_percent;
    int m_maxNumberOfStars;
    const QString m_unsetStarsText;
    QLabel *m_labelPercent;
    KPushButton *m_clearButton;
};

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT StarRatingFieldInput : public StarRating
{
public:
    StarRatingFieldInput(int maxNumberOfStars = defaultMaxNumberOfStars, QWidget *parent = NULL)
            : StarRating(maxNumberOfStars, parent) {
        /* nothing */
    }

    bool reset(const Value &value);
    bool apply(Value &value) const;
};

#endif // KBIBTEX_GUI_STARRATING_H
