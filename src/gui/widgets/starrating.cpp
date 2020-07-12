/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <KLocalizedString>
#include <KRatingPainter>

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

StarRating::~StarRating()
{
    delete d;
}

void StarRating::paintEvent(QPaintEvent *ev)
{
    QWidget::paintEvent(ev);
    QPainter p(this);

    const QRect r = d->starsInside();
    const double percent = d->mouseLocation.isNull() ? d->percent : d->percentForPosition(d->mouseLocation, d->maxNumberOfStars, r);

    static KRatingPainter ratingPainter;
    ratingPainter.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    ratingPainter.setHalfStepsEnabled(false);
    ratingPainter.setMaxRating(d->maxNumberOfStars);
    ratingPainter.setLayoutDirection(qobject_cast<QWidget *>(parent())->layoutDirection());

    if (percent >= 0.0) {
        ratingPainter.paint(&p, d->starsInside(), static_cast<int>(percent / 100.0 * d->maxNumberOfStars));
        if (d->maxNumberOfStars < 10)
            d->labelPercent->setText(QString::number(percent * d->maxNumberOfStars / 100.0, 'f', 1));
        else
            d->labelPercent->setText(QString::number(percent * d->maxNumberOfStars / 100));
    } else {
        p.setOpacity(0.5);
        ratingPainter.paint(&p, d->starsInside(), static_cast<int>(percent / 100.0 * d->maxNumberOfStars));
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

bool StarRatingFieldInput::validate(QWidget **, QString &) const
{
    return true;
}
