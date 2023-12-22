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

#include "starrating.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QPushButton>
#include <QDebug>

#include <KLocalizedString>

const int StarRatingPainter::numberOfStars = 8;

StarRatingPainter::StarRatingPainter()
{
    setHalfStepsEnabled(true);
    setMaxRating(numberOfStars * 2 /** times two because of setHalfStepsEnabled(true) */);
    setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    setLayoutDirection(Qt::LeftToRight);
}

void StarRatingPainter::paint(QPainter *painter, const QRect &rect, double percent, double hoverPercent)
{
    const int rating {percent >= 0.0 ? static_cast<int>(percent *numberOfStars / 50.0 + 0.5) : 0};
    const int hoverRating {hoverPercent >= 0.0 ? static_cast<int>(hoverPercent *numberOfStars / 50.0 + 0.5) : 0};
    KRatingPainter::paint(painter, rect, rating, hoverRating);
}

double StarRatingPainter::roundToNearestHalfStarPercent(double percent)
{
    const double result {percent >= 0.0 ? static_cast<int>(percent *numberOfStars / 50.0 + 0.5) * 50.0 / numberOfStars : -1.0};
    return result;
}

class StarRating::Private
{
private:
    StarRating *p;

public:
    static const int paintMargin;
    StarRatingPainter ratingPainter;

    bool isReadOnly;
    double percent;
    int starHeight;
    int spacing;
    static const QString unsetStarsText;
    QLabel *labelPercent;
    QPushButton *clearButton;
    QPoint mouseLocation;

    Private(StarRating *parent)
            : p(parent), isReadOnly(false), percent(-1.0)
    {
        QHBoxLayout *layout = new QHBoxLayout(p);
        spacing = qMax(layout->spacing(), 8);
        layout->setContentsMargins(0, 0, 0, 0);

        labelPercent = new QLabel(p);
        layout->addWidget(labelPercent, 0, Qt::AlignRight | Qt::AlignVCenter);
        const QFontMetrics fm(labelPercent->fontMetrics());
#if QT_VERSION >= 0x050b00
        labelPercent->setFixedWidth(fm.horizontalAdvance(unsetStarsText));
#else // QT_VERSION >= 0x050b00
        labelPercent->setFixedWidth(fm.width(unsetStarsText));
#endif // QT_VERSION >= 0x050b00
        labelPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelPercent->setText(unsetStarsText);
        labelPercent->installEventFilter(parent);

        layout->addStretch(1);

        clearButton = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-rtl")), QString(), p);
        layout->addWidget(clearButton, 0, Qt::AlignRight | Qt::AlignVCenter);
        connect(clearButton, &QPushButton::clicked, p, &StarRating::clear);
        clearButton->installEventFilter(parent);

        starHeight = qMin(labelPercent->height() * 4 / 3, clearButton->height());
        parent->setMinimumHeight(starHeight);
    }

    inline QRect starsInside() const
    {
        return QRect(QPoint(labelPercent->width() + spacing, (p->height() - starHeight) / 2), QSize(p->width() - 2 * spacing - clearButton->width() - labelPercent->width(), starHeight));
    }

    double percentFromPosition(const QRect &inside, const QPoint &pos)
    {
        const int selectedStars = ratingPainter.ratingFromPosition(inside, pos);
        const double percent {qMax(0, qMin(StarRatingPainter::numberOfStars * 2, selectedStars)) * 50.0 / StarRatingPainter::numberOfStars};
        return percent;
    }
};

const int StarRating::Private::paintMargin = 2;
const QString StarRating::Private::unsetStarsText {i18n("Not set")};

StarRating::StarRating(QWidget *parent)
        : QWidget(parent), d(new Private(this))
{
    QTimer::singleShot(250, this, &StarRating::buttonHeight);

    setMouseTracking(true);
}

StarRating::~StarRating()
{
    delete d;
}

void StarRating::paintEvent(QPaintEvent *ev)
{
    QWidget::paintEvent(ev);
    QPainter p(this);

    const QRect r = d->starsInside();
    const double hoverPercent {d->mouseLocation.isNull() ? -1.0 : d->percentFromPosition(r, d->mouseLocation)};
    const double labelPercent {hoverPercent >= 0.0 ? hoverPercent : d->percent};

    if (labelPercent >= 0.0) {
        d->ratingPainter.paint(&p, d->starsInside(), d->percent, hoverPercent);
        if (StarRatingPainter::numberOfStars < 10)
            d->labelPercent->setText(QString::number(labelPercent * StarRatingPainter::numberOfStars / 100.0, 'f', 1));
        else
            d->labelPercent->setText(QString::number(labelPercent * StarRatingPainter::numberOfStars / 100));
    } else {
        p.setOpacity(0.5);
        d->ratingPainter.paint(&p, d->starsInside(), -1.0);
        d->labelPercent->setText(d->unsetStarsText);
    }

    ev->accept();
}

void StarRating::mouseReleaseEvent(QMouseEvent *ev)
{
    QWidget::mouseReleaseEvent(ev);

    if (!d->isReadOnly && ev->button() == Qt::LeftButton) {
        d->mouseLocation = QPoint();
        const double newPercent = d->percentFromPosition(d->starsInside(), ev->pos());
        setValue(newPercent);
        Q_EMIT modified();
        ev->accept();
    }
}

void StarRating::mouseMoveEvent(QMouseEvent *ev)
{
    QWidget::mouseMoveEvent(ev);

    if (!d->isReadOnly) {
        d->mouseLocation = ev->pos();
        if (d->mouseLocation.x() < d->labelPercent->width() || d->mouseLocation.x() > width() - d->clearButton->width())
            d->mouseLocation = QPoint();
        update();
        ev->accept();
    }
}

void StarRating::leaveEvent(QEvent *ev)
{
    QWidget::leaveEvent(ev);

    if (!d->isReadOnly) {
        d->mouseLocation = QPoint();
        update();
        ev->accept();
    }
}

bool StarRating::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != d->labelPercent && obj != d->clearButton)
        return false;

    if ((event->type() == QEvent::MouseMove || event->type() == QEvent::Enter) && d->mouseLocation != QPoint()) {
        d->mouseLocation = QPoint();
        update();
    }
    return false;
}

double StarRating::value() const
{
    return d->percent;
}

void StarRating::setValue(double percent)
{
    if (d->isReadOnly) return; ///< disallow modifications if read-only

    if (percent >= 0.0 && percent <= 100.0) {
        // Round given percent value to a value matching the number of stars,
        // in steps of half-stars
        d->percent = StarRatingPainter::roundToNearestHalfStarPercent(percent);
        update();
    }
}

void StarRating::unsetValue() {
    if (d->isReadOnly) return; ///< disallow modifications if read-only

    d->mouseLocation = QPoint();
    d->percent = -1.0;
    update();
}

void StarRating::setReadOnly(bool isReadOnly)
{
    d->isReadOnly = isReadOnly;
    d->clearButton->setEnabled(!isReadOnly);
    setMouseTracking(!isReadOnly);
}

void StarRating::clear()
{
    if (d->isReadOnly) return; ///< disallow modifications if read-only

    unsetValue();
    Q_EMIT modified();
}

void StarRating::buttonHeight()
{
    const QSizePolicy sp = d->clearButton->sizePolicy();
    // Allow clear button to take as much vertical space as available
    d->clearButton->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);

    // Update this widget's height behaviour
    d->starHeight = qMin(d->labelPercent->height() * 4 / 3, d->clearButton->height());
    setMinimumHeight(d->starHeight);
}

bool StarRatingFieldInput::reset(const Value &value)
{
    bool result = false;
    const QString text = PlainTextValue::text(value);
    if (text.isEmpty()) {
        unsetValue();
        result = true;
    } else {
        const double number = text.toDouble(&result);
        if (result && number >= 0.0 && number <= 100.0) {
            setValue(number);
            result = true;
        } else {
            // Some value provided that cannot be interpreted or is out of range
            unsetValue();
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

bool StarRatingFieldInput::validate(QWidget **, QString &) const
{
    // Star rating widget has always a valid value (even no rating is valid)
    return true;
}
