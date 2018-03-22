/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#include <KSharedConfig>
#include <KConfigGroup>

#include "notificationhub.h"
#include "preferences.h"

class ColorLabelComboBoxModel : public QAbstractItemModel, private NotificationListener
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

    QList<ColorLabelPair> colorLabelPairs;
    QColor userColor;

    KSharedConfigPtr config;

    ColorLabelComboBoxModel(QObject *p = nullptr)
            : QAbstractItemModel(p), userColor(Qt::black), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))) {
        readConfiguration();
        NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    }

    void notificationEvent(int eventId) override {
        if (eventId == NotificationHub::EventConfigurationChanged)
            readConfiguration();
    }

    void readConfiguration() {
        KConfigGroup configGroup(config, Preferences::groupColor);
        QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
        QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultColorLabels);

        colorLabelPairs.clear();
        for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
            ColorLabelPair clp;
            clp.color = QColor(*itc);
            clp.label = i18n((*itl).toUtf8().constData());
            colorLabelPairs << clp;
        }
    }

    QModelIndex index(int row, int column, const QModelIndex &) const override {
        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex & = QModelIndex()) const override {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent == QModelIndex() ? 2 + colorLabelPairs.count() : 0;
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override {
        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == ColorRole) {
            if (index.row() == 0)
                return QColor(Qt::black);
            else if (index.row() == rowCount() - 1)
                return userColor;
            else
                return colorLabelPairs[index.row() - 1].color;
        } else if (role == Qt::FontRole && (index.row() == 0 || index.row() == rowCount() - 1)) {
            QFont font;
            font.setItalic(true);
            return font;
        } else if (role == Qt::DecorationRole && index.row() > 0 && (index.row() < rowCount() - 1 || userColor != Qt::black)) {
            QColor color = data(index, ColorRole).value<QColor>();
            return ColorLabelWidget::createSolidIcon(color);
        } else if (role == Qt::DisplayRole)
            if (index.row() == 0)
                return i18n("No color");
            else if (index.row() == rowCount() - 1)
                return i18n("User-defined color");
            else
                return colorLabelPairs[index.row() - 1].label;
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
};

class ColorLabelWidget::ColorLabelWidgetPrivate
{
public:
    ColorLabelComboBoxModel *model;

    ColorLabelWidgetPrivate(ColorLabelWidget *parent, ColorLabelComboBoxModel *m)
            : model(m)
    {
        Q_UNUSED(parent)
    }
};

ColorLabelWidget::ColorLabelWidget(QWidget *parent)
        : KComboBox(false, parent), d(new ColorLabelWidgetPrivate(this, new ColorLabelComboBoxModel(this)))
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
    setCurrentIndex(0);
}

bool ColorLabelWidget::reset(const Value &value)
{
    /// Avoid triggering signal when current index is set by the program
    disconnect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);

    QSharedPointer<VerbatimText> verbatimText;
    if (value.count() == 1 && !(verbatimText = value.first().dynamicCast<VerbatimText>()).isNull()) {
        int i = 0;
        const QColor color = QColor(verbatimText->text());
        for (; i < d->model->rowCount(); ++i)
            if (d->model->data(d->model->index(i, 0, QModelIndex()), ColorLabelComboBoxModel::ColorRole).value<QColor>() == color)
                break;

        if (i >= d->model->rowCount()) {
            d->model->userColor = color;
            i = d->model->rowCount() - 1;
        }
        setCurrentIndex(i);
    } else
        setCurrentIndex(0);

    /// Re-enable triggering signal after setting current index
    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorLabelWidget::slotCurrentIndexChanged);

    return true;
}

bool ColorLabelWidget::apply(Value &value) const
{
    QColor color = d->model->data(d->model->index(currentIndex(), 0, QModelIndex()), ColorLabelComboBoxModel::ColorRole).value<QColor>();
    value.clear();
    if (color != Qt::black)
        value.append(QSharedPointer<VerbatimText>(new VerbatimText(color.name())));
    return true;
}

void ColorLabelWidget::setReadOnly(bool isReadOnly)
{
    setEnabled(!isReadOnly);
}

void ColorLabelWidget::slotCurrentIndexChanged(int index)
{
    if (index == count() - 1) {
        QColor dialogColor = d->model->userColor;
        if (QColorDialog::getColor(dialogColor, this) == QColorDialog::Accepted)
            d->model->setColor(dialogColor);
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
