/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "colorlabelwidget.h"

#include <QAbstractItemModel>
#include <QFontMetrics>
#include <QPainter>
#include <QColorDialog>

#include <KLocalizedString>

#include <NotificationHub>
#include <Preferences>

static const QColor NoColor = Qt::black;

class ColorLabelComboBoxModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ColorLabelComboBoxModelRole {
        /// Color of a color-label pair
        ColorRole = Qt::UserRole + 1721
    };

    struct ColorLabelPair {
        QColor color;
        QString label;
    };

    QColor userColor;

    ColorLabelComboBoxModel(QObject *p = nullptr)
            : QAbstractItemModel(p), userColor(NoColor) {
        /// nothing
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override {
        return parent == QModelIndex() ? createIndex(row, column) : QModelIndex();
    }

    QModelIndex parent(const QModelIndex & = QModelIndex()) const override {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent == QModelIndex() ? 2 + Preferences::instance().colorCodes().count() : 0;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent == QModelIndex() ? 1 : 0;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == ColorRole) {
            const int cci = index.row() - 1;
            if (index.row() == 0 || cci < 0 || cci >= Preferences::instance().colorCodes().count())
                return NoColor;
            else if (index.row() == rowCount() - 1)
                return userColor;
            else
                return Preferences::instance().colorCodes().at(cci).first;
        } else if (role == Qt::FontRole && (index.row() == 0 || index.row() == rowCount() - 1)) {
            /// Set first item's text ("No color") and last item's text ("User-defined color") in italics
            QFont font;
            font.setItalic(true);
            return font;
        } else if (role == Qt::DecorationRole && index.row() > 0 && (index.row() < rowCount() - 1 || userColor != NoColor)) {
            /// For items that have a color to choose, draw a little square in this chosen color
            QColor color = data(index, ColorRole).value<QColor>();
            return ColorLabelWidget::createSolidIcon(color);
        } else if (role == Qt::DisplayRole)
            if (index.row() == 0)
                return i18n("No color");
            else if (index.row() == rowCount() - 1)
                return i18n("User-defined color");
            else
                return Preferences::instance().colorCodes().at(index.row() - 1).second;
        else
            return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (section != 0 || orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QVariant();

        return i18n("Color & Label");
    }

    void setColor(const QColor &newColor) {
        userColor = newColor;
        const QModelIndex idx = index(rowCount() - 1, 0, QModelIndex());
        emit dataChanged(idx, idx);
    }

    void reset() {
        beginResetModel();
        endResetModel();
    }
};

class ColorLabelWidget::ColorLabelWidgetPrivate : private NotificationListener
{
private:
    ColorLabelWidget *parent;

public:
    ColorLabelComboBoxModel *model;

    ColorLabelWidgetPrivate(ColorLabelWidget *_parent, ColorLabelComboBoxModel *m)
            : parent(_parent), model(m)
    {
        Q_UNUSED(parent)
        NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    }

    void notificationEvent(int eventId) override {
        if (eventId == NotificationHub::EventConfigurationChanged) {
            /// Avoid triggering signal when current index is set by the program
            disconnect(parent, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), parent, &ColorLabelWidget::slotCurrentIndexChanged);

            const QColor currentColor = parent->currentColor();
            model->reset();
            model->userColor = NoColor;
            selectColor(currentColor);

            /// Re-enable triggering signal after setting current index
            connect(parent, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), parent, &ColorLabelWidget::slotCurrentIndexChanged);
        }
    }

    int selectColor(const QString &color)
    {
        return selectColor(QColor(color));
    }

    int selectColor(const QColor &color)
    {
        int rowIndex = 0;
        if (color != NoColor) {
            /// Find row that matches given color
            for (rowIndex = 0; rowIndex < model->rowCount(); ++rowIndex)
                if (model->data(model->index(rowIndex, 0, QModelIndex()), ColorLabelComboBoxModel::ColorRole).value<QColor>() == color)
                    break;

            if (rowIndex >= model->rowCount()) {
                /// Color was not in the list of known colors, so set user color to given color
                model->userColor = color;
                rowIndex = model->rowCount() - 1;
            }
        }
        return rowIndex;
    }
};

ColorLabelWidget::ColorLabelWidget(QWidget *parent)
        : QComboBox(parent), d(new ColorLabelWidgetPrivate(this, new ColorLabelComboBoxModel(this)))
{
    setModel(d->model);
    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);
}

ColorLabelWidget::~ColorLabelWidget()
{
    delete d;
}

void ColorLabelWidget::clear()
{
    /// Avoid triggering signal when current index is set by the program
    disconnect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);

    d->model->userColor = NoColor;
    setCurrentIndex(0); ///< index 0 should be "no color"

    /// Re-enable triggering signal after setting current index
    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);
}

QColor ColorLabelWidget::currentColor() const
{
    return d->model->data(d->model->index(currentIndex(), 0, QModelIndex()), ColorLabelComboBoxModel::ColorRole).value<QColor>();
}

bool ColorLabelWidget::reset(const Value &value)
{
    /// Avoid triggering signal when current index is set by the program
    disconnect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);

    QSharedPointer<VerbatimText> verbatimText;
    int rowIndex = 0;
    if (value.count() == 1 && !(verbatimText = value.first().dynamicCast<VerbatimText>()).isNull()) {
        /// Create QColor instance based on given textual representation
        rowIndex = d->selectColor(verbatimText->text());
    }
    setCurrentIndex(rowIndex);

    /// Re-enable triggering signal after setting current index
    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);

    return true;
}

bool ColorLabelWidget::apply(Value &value) const
{
    const QColor color = currentColor();
    value.clear();
    if (color != NoColor)
        value.append(QSharedPointer<VerbatimText>(new VerbatimText(color.name())));
    return true;
}

bool ColorLabelWidget::validate(QWidget **, QString &) const
{
    return true;
}

void ColorLabelWidget::setReadOnly(bool isReadOnly)
{
    setEnabled(!isReadOnly);
}

void ColorLabelWidget::slotCurrentIndexChanged(int index)
{
    if (index == count() - 1) {
        const QColor initialColor = d->model->userColor;
        const QColor newColor = QColorDialog::getColor(initialColor, this);
        if (newColor.isValid())
            d->model->setColor(newColor);
    }

    emit modified();
}

QPixmap ColorLabelWidget::createSolidIcon(const QColor &color)
{
    QFontMetrics fm = QFontMetrics(QFont());
    int h = fm.height() - 4;
    QPixmap pm(h, h);
    QPainter painter(&pm);
    painter.setPen(color);
    painter.setBrush(QBrush(color));
    painter.drawRect(0, 0, h, h);
    return pm;
}

#include "colorlabelwidget.moc"
