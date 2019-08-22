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

#include "settingscolorlabelwidget.h"
#include "settingscolorlabelwidget_p.h"

#include <ctime>

#include <QLayout>
#include <QStyledItemDelegate>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>

#include <KColorButton>
#include <KActionMenu>
#include <KLocalizedString>

#include <Preferences>
#include <File>
#include <Entry>
#include "file/fileview.h"
#include "field/colorlabelwidget.h"

class ColorLabelSettingsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ColorLabelSettingsDelegate(QWidget *parent = nullptr)
            : QStyledItemDelegate(parent) {
        /// nothing
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override {
        if (index.column() == 0)
            /// Colors are to be edited in a color button
            return new KColorButton(parent);
        else
            /// Text strings are to be edited in a line edit
            return new QLineEdit(parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        if (index.column() == 0) {
            KColorButton *colorButton = qobject_cast<KColorButton *>(editor);
            /// Initialized color button with row's current color
            colorButton->setColor(index.model()->data(index, Qt::EditRole).value<QColor>());
        } else {
            QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
            /// Initialized line edit with row's current color's label
            lineEdit->setText(index.model()->data(index, Qt::EditRole).toString());
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
        if (index.column() == 0) {
            KColorButton *colorButton = qobject_cast<KColorButton *>(editor);
            if (colorButton->color() != Qt::black)
                /// Assign color button's color back to model
                model->setData(index, colorButton->color(), Qt::EditRole);
        } else if (index.column() == 1) {
            QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
            if (!lineEdit->text().isEmpty())
                /// Assign line edit's text back to model
                model->setData(index, lineEdit->text(), Qt::EditRole);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        QFontMetrics fm = QFontMetrics(QFont());
        /// Enforce minimum height of 4 times x-height
        hint.setHeight(qMax(hint.height(), fm.xHeight() * 4));
        return hint;
    }
};


ColorLabelSettingsModel::ColorLabelSettingsModel(QObject *parent)
        : QAbstractItemModel(parent)
{
    /// Load stored color-label pairs
    loadState();
}

int ColorLabelSettingsModel::rowCount(const QModelIndex &parent) const
{
    /// Single-level list of color-label pairs has as many rows as pairs
    return parent == QModelIndex() ? colorLabelPairs.size() : 0;
}

int ColorLabelSettingsModel::columnCount(const QModelIndex &parent) const
{
    /// Single-level list of color-label pairs has as 2 columns
    /// (one of color, one for label)
    return parent == QModelIndex() ? 2 : 0;
}

QModelIndex ColorLabelSettingsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row <= colorLabelPairs.size() - 1 && column >= 0 && column <= 1 && parent == QModelIndex())
        /// Create index for valid combinations of row, column, and parent
        return createIndex(row, column, static_cast<quintptr>(row));
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
    if (index == QModelIndex() || index.row() < 0 || index.row() >= colorLabelPairs.size())
        return QVariant();

    if ((role == Qt::DecorationRole || role == Qt::EditRole) && index.column() == 0)
        /// First column has colors only (no text)
        return colorLabelPairs[index.row()].first;
    else if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 1)
        /// Second column has colors' labels
        return colorLabelPairs[index.row()].second;

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
                colorLabelPairs[index.row()].first = color;
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
                colorLabelPairs[index.row()].second = text;
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
    colorLabelPairs = Preferences::instance().colorCodes();
}

/**
 * Save list of color-label pairs in user's configuration file.
 */
bool ColorLabelSettingsModel::saveState()
{
    const QVector<QPair<QColor, QString>> oldColorCodes = Preferences::instance().colorCodes();
    Preferences::instance().setColorCodes(colorLabelPairs);

    QVector<QPair<QColor, QString>>::ConstIterator itOld = oldColorCodes.constBegin();
    QVector<QPair<QColor, QString>>::ConstIterator itNew = colorLabelPairs.constBegin();
    for (; itOld != oldColorCodes.constEnd() && itNew != colorLabelPairs.constEnd(); ++itOld, ++itNew)
        if (itOld->first != itNew->first || itOld->second != itNew->second)
            return true;
    return !(itOld == oldColorCodes.constEnd() && itNew == colorLabelPairs.constEnd());
}

/**
 * Revert in-memory data structure (list of color-label pairs) to defaults.
 * Does not touch user's configuration file (would require an Apply operation).
 */
void ColorLabelSettingsModel::resetToDefaults()
{
    colorLabelPairs = Preferences::instance().defaultColorCodes;
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
    colorLabelPairs.append(qMakePair(color, label));
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
    if (row >= 0 && row < colorLabelPairs.size()) {
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

public:
    ColorLabelSettingsModel *model;
    QPushButton *buttonRemove;
    QTreeView *view;

    Private(SettingsColorLabelWidget *parent)
            : p(parent), delegate(nullptr), model(nullptr), buttonRemove(nullptr), view(nullptr)
    {
        /// nothing
    }

    void loadState() {
        /// Delegate state maintenance to model
        if (model != nullptr)
            model->loadState();
    }

    bool saveState() {
        /// Delegate state maintenance to model
        if (model != nullptr)
            return model->saveState();
        else
            return false;
    }

    void resetToDefaults() {
        /// Delegate state maintenance to model
        if (model != nullptr)
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
        /// and label (throuh QLineEdit)
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
    qsrand(time(nullptr));

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

bool SettingsColorLabelWidget::saveState()
{
    return d->saveState();
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
    ColorLabelContextMenu *p;

public:
    /// Tree view to show this context menu in
    FileView *fileView;
    /// Actual menu to show
    KActionMenu *menu;

    Private(FileView *fv, ColorLabelContextMenu *parent)
            : p(parent), fileView(fv)
    {
        menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("preferences-desktop-color")), i18n("Color"), fileView);
        /// Let menu be a sub menu to the tree view's context menu
        fileView->addAction(menu);
    }

    void rebuildMenu()  {
        menu->menu()->clear();

        /// Add color-label pairs to menu as stored
        /// in the user's configuration file
        for (QVector<QPair<QColor, QString>>::ConstIterator it = Preferences::instance().colorCodes().constBegin(); it != Preferences::instance().colorCodes().constEnd(); ++it) {
            QAction *action = new QAction(QIcon(ColorLabelWidget::createSolidIcon(it->first)), it->second, menu);
            menu->addAction(action);
            const QString colorCode = it->first.name();
            connect(action, &QAction::triggered, p, [this, colorCode]() {
                colorActivated(colorCode);
            });
        }

        QAction *action = new QAction(menu);
        action->setSeparator(true);
        menu->addAction(action);

        /// Special action that removes any color
        /// from a BibTeX entry by setting the color to black
        action = new QAction(i18n("No color"), menu);
        menu->addAction(action);
        connect(action, &QAction::triggered, p, [this]() {
            colorActivated(QStringLiteral("#000000"));
        });
    }


    void colorActivated(const QString &colorString)
    {
        /// User activated some color from the menu, so apply this color
        /// code to the currently selected item in the tree view

        SortFilterFileModel *sfbfm = qobject_cast<SortFilterFileModel *>(fileView->model());
        if (sfbfm == nullptr) {
            /// Something went horribly wrong
            return;
        }
        FileModel *model = sfbfm->fileSourceModel();
        if (model == nullptr) {
            /// Something went horribly wrong
            return;
        }

        /// Apply color change to all selected rows
        const QModelIndexList list = fileView->selectionModel()->selectedIndexes();
        for (const QModelIndex &index : list) {
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
                        // TODO notification of modification does not propagate?
                        model->elementChanged(row);
                }
            }
        }
    }
};


ColorLabelContextMenu::ColorLabelContextMenu(FileView *widget)
        : QObject(widget), d(new Private(widget, this))
{
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

#include "settingscolorlabelwidget.moc"
