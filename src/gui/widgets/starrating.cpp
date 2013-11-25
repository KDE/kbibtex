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
#include <QTimer>

#include <KLocale>
#include <KPushButton>
#include <KDebug>

const int StarRating::defaultMaxNumberOfStars = 8;
const double StarRating::UnsetStarsValue = -1.0;

StarRating::StarRating(int maxNumberOfStars, QWidget *parent)
        : QWidget(parent), m_isReadOnly(false), m_percent(-1), m_maxNumberOfStars(maxNumberOfStars), m_unsetStarsText(i18n("Not set"))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    spacing = qMax(layout->spacing(), 8);
    layout->setContentsMargins(0, 0, 0, 0);

    m_labelPercent = new QLabel(this);
    layout->addWidget(m_labelPercent, 0, Qt::AlignRight | Qt::AlignVCenter);
    QFontMetrics fm(m_labelPercent->fontMetrics());
    m_labelPercent->setFixedWidth(fm.width(m_unsetStarsText));
    m_labelPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelPercent->setText(m_unsetStarsText);

    layout->addStretch(1);

    m_clearButton = new KPushButton(KIcon("edit-clear-locationbar-rtl"), QString::null, this);
    layout->addWidget(m_clearButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    QTimer::singleShot(250, this, SLOT(buttonHeight()));

    setMouseTracking(true);
}

QRect StarRating::starsInside() const
{
    const int starRectHeight = qMin(m_labelPercent->height() * 3 / 2, m_clearButton->height());
    return QRect(QPoint(m_labelPercent->width() + spacing, (height() - starRectHeight) / 2), QSize(width() - 2 * spacing - m_clearButton->width() - m_labelPercent->width(), starRectHeight));
}

void StarRating::paintEvent(QPaintEvent *ev)
{
    QWidget::paintEvent(ev);
    QPainter p(this);

    const QRect r = starsInside();
    double percent = m_mouseLocation.isNull() ? m_percent : percentForPosition(m_mouseLocation, m_maxNumberOfStars, r);

    if (percent >= 0.0) {
        paintStars(&p, KIconLoader::DefaultState, m_maxNumberOfStars, percent, starsInside());
        if (m_maxNumberOfStars < 10)
            m_labelPercent->setText(QString::number(percent * m_maxNumberOfStars / 100.0, 'f', 1));
        else
            m_labelPercent->setText(QString::number(percent * m_maxNumberOfStars / 100));
    } else {
        p.setOpacity(0.7);
        paintStars(&p, KIconLoader::DisabledState, m_maxNumberOfStars, 0.0, starsInside());
    }

    ev->accept();
}

void StarRating::mouseReleaseEvent(QMouseEvent *ev)
{
    QWidget::mouseReleaseEvent(ev);

    if (!m_isReadOnly) {
        double newPercent = percentForPosition(ev->pos(), m_maxNumberOfStars, starsInside());
        setValue(newPercent);
        emit modified();
        ev->accept();
    }
}

void StarRating::mouseMoveEvent(QMouseEvent *ev)
{
    QWidget::mouseMoveEvent(ev);

    if (!m_isReadOnly) {
        m_mouseLocation = ev->pos();
        if (m_mouseLocation.x() < m_labelPercent->width() || m_mouseLocation.x() > width() - m_clearButton->width())
            m_mouseLocation = QPoint();
        update();
        ev->accept();
    }
}

void StarRating::leaveEvent(QEvent *)
{
    m_mouseLocation = QPoint();
    update();
}

double StarRating::value() const
{
    return m_percent;
}

void StarRating::setValue(double percent)
{
    if (percent >= 0.0 && percent <= 100.0) {
        if (m_maxNumberOfStars < 10)
            m_labelPercent->setText(QString::number(percent * m_maxNumberOfStars / 100.0, 'f', 1));
        else
            m_labelPercent->setText(QString::number(percent * m_maxNumberOfStars / 100));
        m_percent = percent;
    } else if (percent < 0.0) {
        m_labelPercent->setText(m_unsetStarsText);
        m_percent = percent;
    }
}

void StarRating::setReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
    m_clearButton->setEnabled(!isReadOnly);
    setMouseTracking(!isReadOnly);
}

void StarRating::clear()
{
    if (!m_isReadOnly) {
        setValue(UnsetStarsValue);
        emit modified();
    }
}

void StarRating::buttonHeight()
{
    QSizePolicy sp = m_clearButton->sizePolicy();
    m_clearButton->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);
}

const int StarRating::paintMargin = 2;

void StarRating::paintStars(QPainter *painter, KIconLoader::States defaultState, int numTotalStars, double percent, const QRect &inside)
{
    painter->save();

    const int starSize = qMin(inside.height() - 2 * paintMargin, (inside.width() - 2 * paintMargin) / numTotalStars);
    QPixmap starPixmap = KIconLoader::global()->loadIcon(QLatin1String("rating"), KIconLoader::Small, starSize, defaultState);

    int numActiveStars = percent * numTotalStars / 100;
    int coloredPartWidth = (percent * numTotalStars / 100 - numActiveStars) * starSize;

    const int y = inside.top() + (inside.height() - starSize) / 2;
    int x = inside.left() + paintMargin;
    int i = 0;

    /// draw active (colored) stars
    for (; i < numActiveStars; ++i, x += starSize)
        painter->drawPixmap(x, y, starPixmap);

    if (coloredPartWidth > 0) {
        /// one star is partially colored
        painter->drawPixmap(x, y, starPixmap, 0, 0, coloredPartWidth, 0);
    }

    starPixmap = KIconLoader::global()->loadIcon(QLatin1String("rating"), KIconLoader::Small, starSize, KIconLoader::DisabledState);

    if (coloredPartWidth > 0) {
        /// one star is partially grey
        painter->drawPixmap(x + coloredPartWidth, y, starPixmap, coloredPartWidth, 0, starSize - coloredPartWidth, 0);
        x += starSize;
        ++i;
    }

    /// draw inactive (grey) stars
    for (; i < numTotalStars; ++i, x += starSize)
        painter->drawPixmap(x, y, starPixmap);

    painter->restore();
}

double StarRating::percentForPosition(const QPoint &pos, int numTotalStars, const QRect &inside)
{
    const int starSize = qMin(inside.height() - 2 * paintMargin, (inside.width() - 2 * paintMargin) / numTotalStars);
    const int width = starSize * numTotalStars;
    const int x = pos.x() - paintMargin - inside.left();
    double percent = x * 100.0 / width;
    return qMax(qreal(0.0), qMin(qreal(100.0), percent));
}

bool StarRatingFieldInput::reset(const Value &value)
{
    bool result = false;
    const QString text = PlainTextValue::text(value);
    if (text.isEmpty()) {
        setValue(UnsetStarsValue);
        result = true;
    } else {
        const double number = text.toDouble(&result);
        if (result && number >= 0.0 && number <= 100.0)
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
    const double percent = value();
    if (percent >= 0.0 && percent <= 100)
        v.append(QSharedPointer<PlainText>(new PlainText(QString::number(percent, 'f', 2))));
    return true;
}
