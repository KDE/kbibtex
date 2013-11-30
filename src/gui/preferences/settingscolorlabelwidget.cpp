/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <ctime>

#include <QAbstractItemModel>
#include <QLayout>
#include <QStyledItemDelegate>
#include <QSignalMapper>

#include <KSharedConfigPtr>
#include <KPushButton>
#include <KColorButton>
#include <KColorDialog>
#include <KLineEdit>
#include <KActionMenu>
#include <KMenu>

#include "file.h"
#include "bibtexeditor.h"
#include "colorlabelwidget.h"
#include "bibtexfilemodel.h"
#include "preferences.h"
#include "settingscolorlabelwidget.h"

class ColorLabelSettingsDelegate : public QStyledItemDelegate
{
public:
    ColorLabelSettingsDelegate(QWidget *parent = NULL)
            : QStyledItemDelegate(parent) {
        // nothing
    }

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const {
        if (index.column() == 0)
            return new KColorButton(parent);
        else
            return new KLineEdit(parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const {
        if (index.column() == 0) {
            KColorButton *colorButton = qobject_cast<KColorButton *>(editor);
            colorButton->setColor(index.model()->data(index, Qt::EditRole).value<QColor>());
        } else {
            KLineEdit *lineEdit = qobject_cast<KLineEdit *>(editor);
            lineEdit->setText(index.model()->data(index, Qt::EditRole).toString());
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
        if (index.column() == 0) {
            KColorButton *colorButton = qobject_cast<KColorButton *>(editor);
            if (colorButton->color() != Qt::black)
                model->setData(index, colorButton->color(), Qt::EditRole);
        } else {
            KLineEdit *lineEdit = qobject_cast<KLineEdit *>(editor);
            if (!lineEdit->text().isEmpty())
                model->setData(index, lineEdit->text(), Qt::EditRole);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        QFontMetrics fm = QFontMetrics(QFont());
        hint.setHeight(qMax(hint.height(), fm.xHeight() * 4));
        return hint;
    }
};


ColorLabelSettingsModel::ColorLabelSettingsModel(QObject *parent)
        : QAbstractItemModel(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")))
{
    loadState();
}

int ColorLabelSettingsModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? colorLabelPairs.count() : 0;
}

int ColorLabelSettingsModel::columnCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? 2 : 0;
}

QModelIndex ColorLabelSettingsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row <= colorLabelPairs.count() - 1 && column >= 0 && column <= 1 && parent == QModelIndex())
        return createIndex(row, column, row);
    else return QModelIndex();
}

QModelIndex ColorLabelSettingsModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

QVariant ColorLabelSettingsModel::data(const QModelIndex &index, int role) const
{
    if (index == QModelIndex() || index.row() < 0 || index.row() >= colorLabelPairs.count())
        return QVariant();

    if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 1)
        return colorLabelPairs[index.row()].label;
    else if ((role == Qt::DecorationRole || role == Qt::EditRole) && index.column() == 0)
        return colorLabelPairs[index.row()].color;

    return QVariant();
}

bool ColorLabelSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        const QModelIndex left = index.sibling(index.row(), 0);
        const QModelIndex right = index.sibling(index.row(), 1);
        if (index.column() == 0 && value.canConvert<QColor>()) {
            QColor color = value.value<QColor>();
            if (color != Qt::black && (color.red() > 0 || color.green() > 0 || color.blue() > 0)) {
                colorLabelPairs[index.row()].color = color;
                emit dataChanged(left, right);
                emit modified();
                return true;
            }
        } else if (index.column() == 1 && value.canConvert<QString>()) {
            QString text = value.value<QString>();
            if (!text.isEmpty()) {
                colorLabelPairs[index.row()].label = text;
                emit dataChanged(left, right);
                emit modified();
                return true;
            }
        }
    }
    return false;
}

Qt::ItemFlags ColorLabelSettingsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractItemModel::flags(index);
    result |= Qt::ItemIsEditable;
    return result;
}

QVariant ColorLabelSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0: return i18n("Color");
    case 1: return i18n("Label");
    default: return QVariant();
    }
}

void ColorLabelSettingsModel::loadState()
{
    KConfigGroup configGroup(config, Preferences::groupColor);
    QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
    QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultcolorLabels);

    colorLabelPairs.clear();
    for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
        ColorLabelPair clp;
        clp.color = QColor(*itc);
        clp.label = i18n((*itl).toUtf8().constData());
        colorLabelPairs << clp;
    }
}

void ColorLabelSettingsModel::saveState()
{
    QStringList colorCodes, colorLabels;
    foreach(const ColorLabelPair &clp, colorLabelPairs) {
        colorCodes << clp.color.name();
        colorLabels << clp.label;
    }

    KConfigGroup configGroup(config, Preferences::groupColor);
    configGroup.writeEntry(Preferences::keyColorCodes, colorCodes);
    configGroup.writeEntry(Preferences::keyColorLabels, colorLabels);
    config->sync();
}

void ColorLabelSettingsModel::resetToDefaults()
{
    colorLabelPairs.clear();
    for (QStringList::ConstIterator itc = Preferences::defaultColorCodes.constBegin(), itl = Preferences::defaultcolorLabels.constBegin(); itc != Preferences::defaultColorCodes.constEnd() && itl != Preferences::defaultcolorLabels.constEnd(); ++itc, ++itl) {
        ColorLabelPair clp;
        clp.color = QColor(*itc);
        clp.label = i18n((*itl).toUtf8().constData());
        colorLabelPairs << clp;
    }
    emit modified();
}

bool ColorLabelSettingsModel::containsLabel(const QString &label)
{
    foreach(const ColorLabelPair &clp, colorLabelPairs) {
        if (clp.label == label)
            return true;
    }
    return false;
}

void ColorLabelSettingsModel::addColorLabel(const QColor &color, const QString &label)
{
    const int newRow = colorLabelPairs.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    ColorLabelPair clp;
    clp.color = color;
    clp.label = label;
    colorLabelPairs << clp;
    endInsertRows();

    emit modified();
}

void ColorLabelSettingsModel::removeColorLabel(int row)
{
    if (row >= 0 && row < colorLabelPairs.count()) {
        beginRemoveRows(QModelIndex(), row, row);
        colorLabelPairs.removeAt(row);
        endRemoveRows();
        emit modified();
    }
}


class SettingsColorLabelWidget::SettingsColorLabelWidgetPrivate
{
private:
    SettingsColorLabelWidget *p;
    ColorLabelSettingsDelegate *delegate;

    KSharedConfigPtr config;

public:
    ColorLabelSettingsModel *model;
    KPushButton *buttonRemove;
    QTreeView *view;

    SettingsColorLabelWidgetPrivate(SettingsColorLabelWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        // nothing
    }

    void loadState() {
        model->loadState();
    }

    void saveState() {
        model->saveState();
    }

    void resetToDefaults() {
        model->resetToDefaults();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setMargin(0);

        view = new QTreeView(p);
        layout->addWidget(view, 0, 0, 3, 1);
        view->setRootIsDecorated(false);
        connect(view, SIGNAL(pressed(QModelIndex)), p, SLOT(enableRemoveButton()));

        model = new ColorLabelSettingsModel(view);
        view->setModel(model);
        connect(model, SIGNAL(modified()), p, SIGNAL(changed()));

        delegate = new ColorLabelSettingsDelegate(view);
        view->setItemDelegate(delegate);

        KPushButton *buttonAdd = new KPushButton(KIcon("list-add"), i18n("Add ..."), p);
        layout->addWidget(buttonAdd, 0, 1, 1, 1);
        connect(buttonAdd, SIGNAL(clicked()), p, SLOT(addColor()));

        buttonRemove = new KPushButton(KIcon("list-remove"), i18n("Remove"), p);
        layout->addWidget(buttonRemove, 1, 1, 1, 1);
        buttonRemove->setEnabled(false);
        connect(buttonRemove, SIGNAL(clicked()), p, SLOT(removeColor()));
    }
};


SettingsColorLabelWidget::SettingsColorLabelWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsColorLabelWidgetPrivate(this))
{
    qsrand(time(NULL));
    d->setupGUI();
}

SettingsColorLabelWidget::~SettingsColorLabelWidget()
{
    delete d;
}

QString SettingsColorLabelWidget::label() const
{
    return i18n("Color & Labels");
}

KIcon SettingsColorLabelWidget::icon() const
{
    return KIcon("preferences-desktop-color");
}

void SettingsColorLabelWidget::loadState()
{
    d->loadState();
}

void SettingsColorLabelWidget::saveState()
{
    d->saveState();
}

void SettingsColorLabelWidget::resetToDefaults()
{
    d->resetToDefaults();
}

void SettingsColorLabelWidget::addColor()
{
    const QColor newColor((qrand() & 0xff) | 0x30, (qrand() & 0xff) | 0x30, (qrand() & 0xff) | 0x30);
    const QString newColorName(newColor.name().replace(QLatin1Char('#'), QLatin1String("")));
    d->model->addColorLabel(newColor, i18nc("Label for a new color; placeholder is for a 6-digit hex string", "NewColor%1", newColorName));
}

void SettingsColorLabelWidget::removeColor()
{
    int row = d->view->selectionModel()->selectedIndexes().first().row();
    d->model->removeColorLabel(row);
    d->buttonRemove->setEnabled(!d->view->selectionModel()->selectedIndexes().isEmpty());
}

void SettingsColorLabelWidget::enableRemoveButton()
{
    d->buttonRemove->setEnabled(!d->view->selectionModel()->selectedIndexes().isEmpty());
}


ColorLabelContextMenu::ColorLabelContextMenu(BibTeXEditor *widget)
        : QObject(widget), m_tv(widget)
{
    m_sm = new QSignalMapper(this);
    connect(m_sm, SIGNAL(mapped(QString)), this, SLOT(colorActivated(QString)));

    m_menu = new KActionMenu(KIcon("preferences-desktop-color"), i18n("Color"), widget);
    widget->addAction(m_menu);

    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);

    rebuildMenu();
}

KActionMenu *ColorLabelContextMenu::menuAction()
{
    return m_menu;
}

void ColorLabelContextMenu::setEnabled(bool enabled)
{
    m_menu->setEnabled(enabled);
}

void ColorLabelContextMenu::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged)
        rebuildMenu();
}

void ColorLabelContextMenu::colorActivated(const QString &colorString)
{
    SortFilterBibTeXFileModel *sfbfm = dynamic_cast<SortFilterBibTeXFileModel *>(m_tv->model());
    Q_ASSERT_X(sfbfm != NULL, "ColorLabelContextMenu::colorActivated(const QString &colorString)", "SortFilterBibTeXFileModel *sfbfm is NULL");
    BibTeXFileModel *model = sfbfm->bibTeXSourceModel();
    Q_ASSERT_X(model != NULL, "ColorLabelContextMenu::colorActivated(const QString &colorString)", "BibTeXFileModel *model is NULL");
    File *file = model->bibTeXFile();
    Q_ASSERT_X(file != NULL, "ColorLabelContextMenu::colorActivated(const QString &colorString)", "File *file is NULL");

    bool modifying = false;
    QModelIndexList list = m_tv->selectionModel()->selectedIndexes();
    foreach(const QModelIndex &index, list) {
        const QModelIndex mappedIndex = sfbfm->mapToSource(index);
        if (mappedIndex.column() == 1) {
            QSharedPointer<Entry> entry = file->at(mappedIndex.row()).dynamicCast<Entry>();
            if (!entry.isNull()) {
                entry->remove(Entry::ftColor);
                if (colorString != QLatin1String("#000000")) {
                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(colorString)));
                    entry->insert(Entry::ftColor, v);
                }
                modifying = true;
            }
        }
    }

    if (modifying)
        m_tv->externalModification();
}

void ColorLabelContextMenu::rebuildMenu()
{
    m_menu->menu()->clear();

    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    KConfigGroup configGroup(config, Preferences::groupColor);
    QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
    QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultcolorLabels);
    for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
        KAction *action = new KAction(KIcon(ColorLabelWidget::createSolidIcon(*itc)), i18n((*itl).toUtf8().constData()), m_menu);
        m_menu->addAction(action);
        m_sm->setMapping(action, *itc);
        connect(action, SIGNAL(triggered()), m_sm, SLOT(map()));
    }

    KAction *action = new KAction(m_menu);
    action->setSeparator(true);
    m_menu->addAction(action);

    action = new KAction(i18n("No color"), m_menu);
    m_menu->addAction(action);
    m_sm->setMapping(action, QLatin1String("#000000"));
    connect(action, SIGNAL(triggered()), m_sm, SLOT(map()));
}
