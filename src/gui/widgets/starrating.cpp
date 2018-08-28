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

#include "starrating.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QPushButton>

#include <KLocalizedString>

class StarRating::Private
{
private:
    StarRating *p;

public:
    static const int paintMargin;

    bool isReadOnly;
    double percent;
    int maxNumberOfStars;
    int spacing;
    const QString unsetStarsText;
    QLabel *labelPercent;
    QPushButton *clearButton;
    QPoint mouseLocation;

    Private(int mnos, StarRating *parent)
            : p(parent), isReadOnly(false), percent(-1.0), maxNumberOfStars(mnos),
          unsetStarsText(i18n("Not set"))
    {
        QHBoxLayout *layout = new QHBoxLayout(p);
        spacing = qMax(layout->spacing(), 8);
        layout->setContentsMargins(0, 0, 0, 0);

        labelPercent = new QLabel(p);
        layout->addWidget(labelPercent, 0, Qt::AlignRight | Qt::AlignVCenter);
        QFontMetrics fm(labelPercent->fontMetrics());
        labelPercent->setFixedWidth(fm.width(unsetStarsText));
        labelPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelPercent->setText(unsetStarsText);
        labelPercent->installEventFilter(parent);

        layout->addStretch(1);

        clearButton = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-rtl")), QString(), p);
        layout->addWidget(clearButton, 0, Qt::AlignRight | Qt::AlignVCenter);
        connect(clearButton, &QPushButton::clicked, p, &StarRating::clear);
        clearButton->installEventFilter(parent);
    }

    QRect starsInside() const
    {
        const int starRectHeight = qMin(labelPercent->height() * 3 / 2, clearButton->height());
        return QRect(QPoint(labelPercent->width() + spacing, (p->height() - starRectHeight) / 2), QSize(p->width() - 2 * spacing - clearButton->width() - labelPercent->width(), starRectHeight));
    }

    double percentForPosition(const QPoint pos, int numTotalStars, const QRect inside)
    {
        const int starSize = qMin(inside.height() - 2 * Private::paintMargin, (inside.width() - 2 * Private::paintMargin) / numTotalStars);
        const int width = starSize * numTotalStars;
        const int x = pos.x() - Private::paintMargin - inside.left();
        const double percent = x * 100.0 / width;
        return qMax(0.0, qMin(100.0, percent));
    }
};

const int StarRating::Private::paintMargin = 2;

StarRating::StarRating(int maxNumberOfStars, QWidget *parent)
        : QWidget(parent), d(new Private(maxNumberOfStars, this))
{
    QTimer::singleShot(250, this, &StarRating::buttonHeight);

    setMouseTracking(true);
}

void StarRating::paintEvent(QPaintEvent *ev)
{
    QWidget::paintEvent(ev);
    QPainter p(this);

    const QRect r = d->starsInside();
    const double percent = d->mouseLocation.isNull() ? d->percent : d->percentForPosition(d->mouseLocation, d->maxNumberOfStars, r);

    if (percent >= 0.0) {
        paintStars(&p, KIconLoader::DefaultState, d->maxNumberOfStars, percent, d->starsInside());
        if (d->maxNumberOfStars < 10)
            d->labelPercent->setText(QString::number(percent * d->maxNumberOfStars / 100.0, 'f', 1));
        else
            d->labelPercent->setText(QString::number(percent * d->maxNumberOfStars / 100));
    } else {
        p.setOpacity(0.7);
        paintStars(&p, KIconLoader::DisabledState, d->maxNumberOfStars, 0.0, d->starsInside());
        d->labelPercent->setText(d->unsetStarsText);
    }

    ev->accept();
}

void StarRating::mouseReleaseEvent(QMouseEvent *ev)
{
    QWidget::mouseReleaseEvent(ev);

    if (!d->isReadOnly && ev->button() == Qt::LeftButton) {
        d->mouseLocation = QPoint();
        const double newPercent = d->percentForPosition(ev->pos(), d->maxNumberOfStars, d->starsInside());
        setValue(newPercent);
        emit modified();
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
        d->percent = percent;
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
    emit modified();
}

void StarRating::buttonHeight()
{
    const QSizePolicy sp = d->clearButton->sizePolicy();
    /// Allow clear button to take as much vertical space as available
    d->clearButton->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);
}

void StarRating::paintStars(QPainter *painter, KIconLoader::States defaultState, int numTotalStars, double percent, const QRect inside)
{
    painter->save(); ///< Save the current painter's state; at this function's end restored

    /// Calculate a single star's width/height
    /// so that all stars fit into the "inside" rectangle
    const int starSize = qMin(inside.height() - 2 * Private::paintMargin, (inside.width() - 2 * Private::paintMargin) / numTotalStars);

    /// First, draw active/golden/glowing stars (on the left side)

    /// Create a pixmap of a single active/golden/glowing star
    QPixmap starPixmap = KIconLoader::global()->loadIcon(QStringLiteral("rating"), KIconLoader::Small, starSize, defaultState);
    /// Calculate vertical position (same for all stars)
    const int y = inside.top() + (inside.height() - starSize) / 2;

    /// Number of full golden stars
    int numActiveStars = static_cast<int>(percent * numTotalStars / 100);
    /// Number of golden pixels of the star that is
    /// partially golden and partially grey
    int coloredPartWidth = static_cast<int>((percent * numTotalStars / 100 - numActiveStars) * starSize);

    /// Horizontal position of first star
    int x = inside.left() + Private::paintMargin;

    int i = 0; ///< start with first star
    /// Draw active (colored) stars
    for (; i < numActiveStars; ++i, x += starSize)
        painter->drawPixmap(x, y, starPixmap);

    if (coloredPartWidth > 0) {
        /// One star is partially colored, so draw star's golden left half
        painter->drawPixmap(x, y, starPixmap, 0, 0, coloredPartWidth, 0);
    }

    /// Second, draw grey/disabled stars (on the right side)
    /// To do so, replace the previously used golden star pixmal with a grey/disabled one
    starPixmap = KIconLoader::global()->loadIcon(QStringLiteral("rating"), KIconLoader::Small, starSize, KIconLoader::DisabledState);

    if (coloredPartWidth > 0) {
        /// One star is partially grey, so draw star's grey right half
        painter->drawPixmap(x + coloredPartWidth, y, starPixmap, coloredPartWidth, 0, starSize - coloredPartWidth, 0);
        x += starSize;
        ++i;
    }

    /// Draw the remaining inactive (grey) stars
    for (; i < numTotalStars; ++i, x += starSize)
        painter->drawPixmap(x, y, starPixmap);

    painter->restore(); ///< Restore the painter's state as saved at this function's beginning
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
            /// Some value provided that cannot be interpreted
            unsetValue();
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
