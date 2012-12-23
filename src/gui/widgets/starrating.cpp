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

#include "starrating.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>

#include <KLocale>
#include <KPushButton>
#include <KDebug>

#include "guihelper.h"

const int StarRating::defaultMaxNumberOfStars = 8;
const int StarRating::UnsetStarsValue = -1;

StarRating::StarRating(int maxNumberOfStars, QWidget *parent)
        : QWidget(parent), m_isReadOnly(false), m_percent(-1), m_maxNumberOfStars(maxNumberOfStars), m_unsetStarsText(i18n("Not set"))
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    m_labelPercent = new QLabel(this);
    layout->addWidget(m_labelPercent, 0, Qt::AlignRight | Qt::AlignVCenter);
    QFontMetrics fm(m_labelPercent->fontMetrics());
    m_labelPercent->setFixedWidth(fm.width(m_unsetStarsText));
    m_labelPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelPercent->setText(m_unsetStarsText);

    layout->addStretch(1);

    m_clearButton = new KPushButton(KIcon("edit-clear-locationbar-rtl"), QString::null, this);
    layout->addWidget(m_clearButton, 0);
    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clear()));
}

void StarRating::paintEvent(QPaintEvent *ev)
{
    QWidget::paintEvent(ev);

    if (m_percent >= 0) {
        QPainter p(this);
        const int space = qMax(layout()->spacing(), 8);
        int numActiveStars = (m_percent * m_maxNumberOfStars + 50) / 100;
        QSize size(width() - space - m_labelPercent->width(), height());
        QPoint pos(m_labelPercent->width() + space, 0);
        GUIHelper::paintStars(&p, numActiveStars, m_maxNumberOfStars, size, pos);
    } else if (m_percent == UnsetStarsValue) {
        QPainter p(this);
        p.setOpacity(0.7);
        const int space = qMax(layout()->spacing(), 8);
        QSize size(width() - space - m_labelPercent->width(), height());
        QPoint pos(m_labelPercent->width() + space, 0);
        GUIHelper::paintStars(&p, 0, m_maxNumberOfStars, size, pos);
    }

    ev->accept();
}

void StarRating::mouseReleaseEvent(QMouseEvent *ev)
{
    QWidget::mouseReleaseEvent(ev);

    if (!m_isReadOnly) {
        const int space = qMax(layout()->spacing(), 8);
        QSize size(width() - space - m_labelPercent->width(), height());
        QPoint pos(m_labelPercent->width() + space, 0);
        int newPercent = GUIHelper::starsXvalueToPercent(m_maxNumberOfStars, size, pos, ev->x());
        newPercent = qMax(0, qMin(100, newPercent));
        setValue(newPercent);
        emit modified();
    }

    ev->accept();
}

int StarRating::value() const
{
    return m_percent;
}

void StarRating::setValue(int percent)
{
    if (percent >= 0 && percent <= 100) {
        m_labelPercent->setText(QString::number(percent));
        m_percent = percent;
    } else if (UnsetStarsValue == -1) {
        m_labelPercent->setText(m_unsetStarsText);
        m_percent = percent;
    }
}

void StarRating::setReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
    m_clearButton->setEnabled(!isReadOnly);
}

void StarRating::clear()
{
    if (!m_isReadOnly) {
        setValue(UnsetStarsValue);
        emit modified();
    }
}

bool StarRatingFieldInput::reset(const Value &value)
{
    bool result = false;
    const QString text = PlainTextValue::text(value);
    if (text.isEmpty()) {
        setValue(UnsetStarsValue);
        result = true;
    } else {
        const int number = text.toInt(&result);
        if (result && number >= 0 && number <= 100)
            setValue(number);
        else {
            setValue(UnsetStarsValue);
            result = false;
        }
    }
    return result;
}

bool StarRatingFieldInput::apply(Value &v) const
{
    v.clear();
    if (value() >= 0 && value() <= 100)
        v.append(QSharedPointer<PlainText>(new PlainText(QString::number(value()))));
    return true;
}
