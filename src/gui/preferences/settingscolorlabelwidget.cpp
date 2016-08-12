/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "settingscolorlabelwidget.h"
#include "settingscolorlabelwidget_p.h"

#include <ctime>

#include <QLayout>
#include <QStyledItemDelegate>
#include <QSignalMapper>
#include <QPushButton>
#include <QMenu>

#include <KColorButton>
#include <KLineEdit>
#include <KActionMenu>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "file.h"
#include "fileview.h"
#include "colorlabelwidget.h"
#include "models/filemodel.h"
#include "preferences.h"

class ColorLabelSettingsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ColorLabelSettingsDelegate(QWidget *parent = NULL)
            : QStyledItemDelegate(parent) {
        /// nothing
    }

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const {
        if (index.column() == 0)
            /// Colors are to be edited in a color button
            return new KColorButton(parent);
        else
            /// Text strings are to be edited in a line edit
            return new KLineEdit(parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const {
        if (index.column() == 0) {
            KColorButton *colorButton = qobject_cast<KColorButton *>(editor);
            /// Initialized color button with row's current color
            colorButton->setColor(index.model()->data(index, Qt::EditRole).value<QColor>());
        } else {
            KLineEdit *lineEdit = qobject_cast<KLineEdit *>(editor);
            /// Initialized line edit with row's current color's label
            lineEdit->setText(index.model()->data(index, Qt::EditRole).toString());
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
        if (index.column() == 0) {
            KColorButton *colorButton = qobject_cast<KColorButton *>(editor);
            if (colorButton->color() != Qt::black)
                /// Assign color button's color back to model
                model->setData(index, colorButton->color(), Qt::EditRole);
        } else if (index.column() == 1) {
            KLineEdit *lineEdit = qobject_cast<KLineEdit *>(editor);
            if (!lineEdit->text().isEmpty())
                /// Assign line edit's text back to model
                model->setData(index, lineEdit->text(), Qt::EditRole);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        QFontMetrics fm = QFontMetrics(QFont());
        /// Enforce minimum height of 4 times x-height
        hint.setHeight(qMax(hint.height(), fm.xHeight() * 4));
        return hint;
    }
};


ColorLabelSettingsModel::ColorLabelSettingsModel(QObject *parent)
        : QAbstractItemModel(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")))
{
    /// Load stored color-label pairs
    loadState();
}

int ColorLabelSettingsModel::rowCount(const QModelIndex &parent) const
{
    /// Single-level list of color-label pairs has as many rows as pairs
    return parent == QModelIndex() ? colorLabelPairs.count() : 0;
}

int ColorLabelSettingsModel::columnCount(const QModelIndex &parent) const
{
    /// Single-level list of color-label pairs has as 2 columns
    /// (one of color, one for label)
    return parent == QModelIndex() ? 2 : 0;
}

QModelIndex ColorLabelSettingsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row <= colorLabelPairs.count() - 1 && column >= 0 && column <= 1 && parent == QModelIndex())
        /// Create index for valid combinations of row, column, and parent
        return createIndex(row, column, row);
    else
        return QModelIndex();
}

QModelIndex ColorLabelSettingsModel::parent(const QModelIndex &) const
{
    /// Single-level list's indices have no other index as parent
    return QModelIndex();
}

QVariant ColorLabelSettingsModel::data(const QModelIndex &index, int role) const
{
    /// Skip invalid model indices
    if (index == QModelIndex() || index.row() < 0 || index.row() >= colorLabelPairs.count())
        return QVariant();

    if ((role == Qt::DecorationRole || role == Qt::EditRole) && index.column() == 0)
        /// First column has colors only (no text)
        return colorLabelPairs[index.row()].color;
    else if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 1)
        /// Second column has colors' labels
        return colorLabelPairs[index.row()].label;

    return QVariant();
}

bool ColorLabelSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        const QModelIndex left = index.sibling(index.row(), 0);
        const QModelIndex right = index.sibling(index.row(), 1);
        if (index.column() == 0 && value.canConvert<QColor>()) {
            /// For first column if a valid color is to be set ...
            const QColor color = value.value<QColor>();
            if (color != Qt::black && (color.red() > 0 || color.green() > 0 || color.blue() > 0)) {
                /// ... store this color in the data structure
                colorLabelPairs[index.row()].color = color;
                /// Notify everyone about the changes
                emit dataChanged(left, right);
                emit modified();
                return true;
            }
        } else if (index.column() == 1 && value.canConvert<QString>()) {
            /// For second column if a label text is to be set ...
            const QString text = value.toString();
            if (!text.isEmpty()) {
                /// ... store this text in the data structure
                colorLabelPairs[index.row()].label = text;
                /// Notify everyone about the changes
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
    result |= Qt::ItemIsEditable; ///< all cells can be edited (color or text label)
    return result;
}

QVariant ColorLabelSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    /// Only vertical lists supported
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0: return i18n("Color");
    case 1: return i18n("Label");
    default: return QVariant();
    }
}

/**
 * Load list of color-label pairs from user's configuration file.
 * Fall back on pre-defined colors and labels if no user settings exist.
 */
void ColorLabelSettingsModel::loadState()
{
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

/**
 * Save list of color-label pairs in user's configuration file.
 */
void ColorLabelSettingsModel::saveState()
{
    QStringList colorCodes, colorLabels;
    foreach (const ColorLabelPair &clp, colorLabelPairs) {
        colorCodes << clp.color.name();
        colorLabels << clp.label;
    }

    KConfigGroup configGroup(config, Preferences::groupColor);
    configGroup.writeEntry(Preferences::keyColorCodes, colorCodes);
    configGroup.writeEntry(Preferences::keyColorLabels, colorLabels);
    config->sync();
}

/**
 * Revert in-memory data structure (list of color-label pairs) to defaults.
 * Does not touch user's configuration file (would require an Apply operation).
 */
void ColorLabelSettingsModel::resetToDefaults()
{
    colorLabelPairs.clear();
    for (QStringList::ConstIterator itc = Preferences::defaultColorCodes.constBegin(), itl = Preferences::defaultColorLabels.constBegin(); itc != Preferences::defaultColorCodes.constEnd() && itl != Preferences::defaultColorLabels.constEnd(); ++itc, ++itl) {
        ColorLabelPair clp;
        clp.color = QColor(*itc);
        clp.label = i18n((*itl).toUtf8().constData());
        colorLabelPairs << clp;
    }
    emit modified();
}

/**
 * Add a new color-label pair to this model.
 * The pair will be appended to the list's end.
 * No check is performed if a similar color or label is already in use.
 *
 * @param color Color of the color-label pair
 * @param label Label of the color-label pair
 */
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

/**
 * Remove a color-label pair from this model.
 * The pair is identified by the row number.
 *
 * @param row Row number of the pair to be removed
 */
void ColorLabelSettingsModel::removeColorLabel(int row)
{
    if (row >= 0 && row < colorLabelPairs.count()) {
        beginRemoveRows(QModelIndex(), row, row);
        colorLabelPairs.removeAt(row);
        endRemoveRows();
        emit modified();
    }
}


class SettingsColorLabelWidget::Private
{
private:
    SettingsColorLabelWidget *p;
    ColorLabelSettingsDelegate *delegate;

    KSharedConfigPtr config;

public:
    ColorLabelSettingsModel *model;
    QPushButton *buttonRemove;
    QTreeView *view;

    Private(SettingsColorLabelWidget *parent)
            : p(parent), delegate(NULL), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))),
          model(NULL), buttonRemove(NULL), view(NULL) {
        /// nothing
    }

    void loadState() {
        /// Delegate state maintenance to model
        if (model != NULL)
            model->loadState();
    }

    void saveState() {
        /// Delegate state maintenance to model
        if (model != NULL)
            model->saveState();
    }

    void resetToDefaults() {
        /// Delegate state maintenance to model
        if (model != NULL)
            model->resetToDefaults();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setMargin(0);

        /// Central element in the main widget
        /// is a tree view for color-label pairs
        view = new QTreeView(p);
        layout->addWidget(view, 0, 0, 3, 1);
        view->setRootIsDecorated(false);

        /// Tree view's model maintains color-label pairs
        model = new ColorLabelSettingsModel(view);
        view->setModel(model);
        /// Changes in the model (e.g. through setData(..))
        /// get propagated as this widget's changed() signal
        connect(model, &ColorLabelSettingsModel::modified, p, &SettingsColorLabelWidget::changed);

        /// Delegate to handle changes of color (through KColorButton)
        /// and label (throuh KLineEdit)
        delegate = new ColorLabelSettingsDelegate(view);
        view->setItemDelegate(delegate);

        /// Button to add a new randomized color
        QPushButton *buttonAdd = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add..."), p);
        layout->addWidget(buttonAdd, 0, 1, 1, 1);
        connect(buttonAdd, &QPushButton::clicked, p, &SettingsColorLabelWidget::addColor);

        /// Remove selected color-label pair; button is disabled
        /// if no row is selected in tree view
        buttonRemove = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove"), p);
        layout->addWidget(buttonRemove, 1, 1, 1, 1);
        buttonRemove->setEnabled(false);
        connect(buttonRemove, &QPushButton::clicked, p, &SettingsColorLabelWidget::removeColor);
    }
};


SettingsColorLabelWidget::SettingsColorLabelWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new Private(this))
{
    /// Seed random number generator
    qsrand(time(NULL));

    /// Setup GUI elements
    d->setupGUI();
    /// Connect signals
    connect(d->view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SettingsColorLabelWidget::updateRemoveButtonStatus);
}

SettingsColorLabelWidget::~SettingsColorLabelWidget()
{
    delete d;
}

QString SettingsColorLabelWidget::label() const
{
    return i18n("Color & Labels");
}

QIcon SettingsColorLabelWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("preferences-desktop-color"));
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
    /// Create a randomized color, but guarantee
    /// some minimum value for each color component
    const QColor newColor((qrand() & 0xff) | 0x30, (qrand() & 0xff) | 0x30, (qrand() & 0xff) | 0x30);
    /// Set the new label to be the color's hex string
    const QString newColorName(newColor.name().remove(QLatin1Char('#')));
    /// Add new color-label pair to model's data
    d->model->addColorLabel(newColor, i18nc("Label for a new color; placeholder is for a 6-digit hex string", "NewColor%1", newColorName));
}

void SettingsColorLabelWidget::removeColor()
{
    if (!d->view->selectionModel()->selectedIndexes().isEmpty()) {
        /// Determine which row is selected
        const int row = d->view->selectionModel()->selectedIndexes().first().row();
        /// Remove row from model
        d->model->removeColorLabel(row);
        updateRemoveButtonStatus();
    }
}

void SettingsColorLabelWidget::updateRemoveButtonStatus()
{
    /// Enable remove button iff tree view's selection is not empty
    d->buttonRemove->setEnabled(!d->view->selectionModel()->selectedIndexes().isEmpty());
}


class ColorLabelContextMenu::Private
{
private:
    // UNUSED ColorLabelContextMenu *p;

public:
    /// Tree view to show this context menu in
    FileView *fileView;
    /// Actual menu to show
    KActionMenu *menu;
    /// Signal handle to react to calls to items in menu
    QSignalMapper *sm;

    Private(FileView *fv, ColorLabelContextMenu *parent)
        : /* UNUSED p(parent),*/ fileView(fv)
    {
        sm = new QSignalMapper(parent);
        menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("preferences-desktop-color")), i18n("Color"), fileView);
        /// Let menu be a sub menu to the tree view's context menu
        fileView->addAction(menu);
    }

    void rebuildMenu()  {
        menu->menu()->clear();

        /// Add color-label pairs to menu as stored
        /// in the user's configuration file
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, Preferences::groupColor);
        QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
        QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultColorLabels);
        for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
            QAction *action = new QAction(QIcon(ColorLabelWidget::createSolidIcon(*itc)), i18n((*itl).toUtf8().constData()), menu);
            menu->addAction(action);
            sm->setMapping(action, *itc);
            connect(action, &QAction::triggered, sm, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        }

        QAction *action = new QAction(menu);
        action->setSeparator(true);
        menu->addAction(action);

        /// Special action that removes any color
        /// from a BibTeX entry by setting the color to black
        action = new QAction(i18n("No color"), menu);
        menu->addAction(action);
        sm->setMapping(action, QStringLiteral("#000000"));
        connect(action, &QAction::triggered, sm, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
    }
};


ColorLabelContextMenu::ColorLabelContextMenu(FileView *widget)
        : QObject(widget), d(new Private(widget, this))
{
    connect(d->sm, static_cast<void(QSignalMapper::*)(const QString &)>(&QSignalMapper::mapped), this, &ColorLabelContextMenu::colorActivated);

    /// Listen to changes in the configuration files
    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);

    d->rebuildMenu();
}

ColorLabelContextMenu::~ColorLabelContextMenu() {
    delete d;
}

KActionMenu *ColorLabelContextMenu::menuAction()
{
    return d->menu;
}

void ColorLabelContextMenu::setEnabled(bool enabled)
{
    d->menu->setEnabled(enabled);
}

void ColorLabelContextMenu::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged)
        d->rebuildMenu();
}

void ColorLabelContextMenu::colorActivated(const QString &colorString)
{
    /// User activated some color from the menu,
    /// so apply this color code to the currently
    /// selected item in the tree view

    SortFilterFileModel *sfbfm = qobject_cast<SortFilterFileModel *>(d->fileView->model());
    Q_ASSERT_X(sfbfm != NULL, "ColorLabelContextMenu::colorActivated(const QString &colorString)", "SortFilterFileModel *sfbfm is NULL");
    FileModel *model = sfbfm->fileSourceModel();
    Q_ASSERT_X(model != NULL, "ColorLabelContextMenu::colorActivated(const QString &colorString)", "FileModel *model is NULL");

    /// Apply color change to all selected rows
    QModelIndexList list = d->fileView->selectionModel()->selectedIndexes();
    foreach (const QModelIndex &index, list) {
        const QModelIndex mappedIndex = sfbfm->mapToSource(index);
        /// Selection may span over multiple columns;
        /// to avoid duplicate assignments, consider only column 1
        if (mappedIndex.column() == 1) {
            const int row = mappedIndex.row();
            QSharedPointer<Entry> entry = model->element(row).dynamicCast<Entry>();
            if (!entry.isNull()) {
                /// Clear old color entry
                bool modifying = entry->remove(Entry::ftColor) > 0;
                if (colorString != QStringLiteral("#000000")) { ///< black is a special color that means "no color"
                    /// Only if valid color was selected, set this color
                    Value v;
                    v.append(QSharedPointer<VerbatimText>(new VerbatimText(colorString)));
                    entry->insert(Entry::ftColor, v);
                    modifying = true;
                }
                if (modifying)
                    model->elementChanged(row);
            }
        }
    }
}

#include "settingscolorlabelwidget.moc"
