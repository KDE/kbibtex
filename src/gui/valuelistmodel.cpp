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

#include "valuelistmodel.h"

#include <typeinfo>

#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QListView>
#include <QGridLayout>
#include <QStringListModel>
#include <QPainter>
#include <QFrame>
#include <QLayout>
#include <QHeaderView>

#include <KComboBox>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KColorScheme>
#include <KLineEdit>

#include "fieldlineedit.h"
#include "bibtexfields.h"
#include "entry.h"
#include "preferences.h"
#include "models/filemodel.h"
#include "logging_gui.h"

QWidget *ValueListDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &sovi, const QModelIndex &index) const
{
    if (index.column() == 0) {
        const FieldDescription &fd = BibTeXFields::self()->find(m_fieldName);
        FieldLineEdit *fieldLineEdit = new FieldLineEdit(fd.preferredTypeFlag, fd.typeFlags, false, parent);
        fieldLineEdit->setAutoFillBackground(true);
        return fieldLineEdit;
    } else
        return QStyledItemDelegate::createEditor(parent, sovi, index);
}

void ValueListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 0) {
        FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit *>(editor);
        if (fieldLineEdit != nullptr)
            fieldLineEdit->reset(index.model()->data(index, Qt::EditRole).value<Value>());
    }
}

void ValueListDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit *>(editor);
    if (fieldLineEdit != nullptr) {
        Value v;
        fieldLineEdit->apply(v);
        if (v.count() == 1) /// field should contain exactly one value item (no zero, not two or more)
            model->setData(index, QVariant::fromValue(v));
    }
}

QSize ValueListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), option.fontMetrics.height() * 3 / 2));   // TODO calculate height better
    return size;
}

void ValueListDelegate::commitAndCloseEditor()
{
    KLineEdit *editor = qobject_cast<KLineEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

void ValueListDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    if (option->decorationPosition != QStyleOptionViewItem::Top) {
        /// remove text from style (do not draw text)
        option->text.clear();
    }

}

void ValueListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &_option, const QModelIndex &index) const
{
    QStyleOptionViewItem option = _option;

    /// code heavily inspired by kdepimlibs-4.6.3/akonadi/collectionstatisticsdelegate.cpp

    /// save painter's state, restored before leaving this function
    painter->save();

    /// first, paint the basic, but without the text. We remove the text
    /// in initStyleOption(), which gets called by QStyledItemDelegate::paint().
    QStyledItemDelegate::paint(painter, option, index);

    /// now, we retrieve the correct style option by calling intiStyleOption from
    /// the superclass.
    QStyledItemDelegate::initStyleOption(&option, index);
    QString field = option.text;

    /// now calculate the rectangle for the text
    QStyle *s = m_parent->style();
    const QWidget *widget = option.widget;
    const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &option, widget);

    if (option.state & QStyle::State_Selected) {
        /// selected lines are drawn with different color
        painter->setPen(option.palette.highlightedText().color());
    }

    /// count will be empty unless only one column is shown
    const QString count = index.column() == 0 && index.model()->columnCount() == 1 ? QString(QStringLiteral(" (%1)")).arg(index.data(ValueListModel::CountRole).toInt()) : QString();

    /// squeeze the folder text if it is to big and calculate the rectangles
    /// where the folder text and the unread count will be drawn to
    QFontMetrics fm(painter->fontMetrics());
    int countWidth = fm.width(count);
    int fieldWidth = fm.width(field);
    if (countWidth + fieldWidth > textRect.width()) {
        /// text plus count is too wide for column, cut text and insert "..."
        field = fm.elidedText(field, Qt::ElideRight, textRect.width() - countWidth - 8);
        fieldWidth = textRect.width() - countWidth - 12;
    }

    /// determine rects to draw field
    int top = textRect.top() + (textRect.height() - fm.height()) / 2;
    QRect fieldRect = textRect;
    QRect countRect = textRect;
    fieldRect.setTop(top);
    fieldRect.setHeight(fm.height());

    if (m_parent->header()->visualIndex(index.column()) == 0) {
        /// left-align text
        fieldRect.setLeft(fieldRect.left() + 4); ///< hm, indent necessary?
        fieldRect.setRight(fieldRect.left() + fieldWidth);
    } else {
        /// right-align text
        fieldRect.setRight(fieldRect.right() - 4); ///< hm, indent necessary?
        fieldRect.setLeft(fieldRect.right() - fieldWidth); ///< hm, indent necessary?
    }

    /// draw field name
    painter->drawText(fieldRect, Qt::AlignLeft, field);

    if (!count.isEmpty()) {
        /// determine rects to draw count
        countRect.setTop(top);
        countRect.setHeight(fm.height());
        countRect.setLeft(fieldRect.right());

        /// use bold font
        QFont font = painter->font();
        font.setBold(true);
        painter->setFont(font);
        /// determine color for count number
        const QColor countColor = (option.state & QStyle::State_Selected) ? KColorScheme(QPalette::Active, KColorScheme::Selection).foreground(KColorScheme::LinkText).color() : KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::LinkText).color();
        painter->setPen(countColor);

        /// draw count
        painter->drawText(countRect, Qt::AlignLeft, count);
    }

    /// restore painter's state
    painter->restore();
}

ValueListModel::ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent)
        : QAbstractTableModel(parent), file(bibtexFile), fName(fieldName.toLower()), showCountColumn(true), sortBy(SortByText)
{
    readConfiguration();
    updateValues();
    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
}

int ValueListModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? values.count() : 0;
}

int ValueListModel::columnCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? (showCountColumn ? 2 : 1) : 0;
}

QVariant ValueListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= values.count() || index.column() >= 2)
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            if (fName == Entry::ftColor) {
                QString text = values[index.row()].text;
                if (text.isEmpty()) return QVariant();
                QString colorText = colorToLabel[text];
                if (colorText.isEmpty()) return QVariant(text);
                return QVariant(colorText);
            } else
                return QVariant(values[index.row()].text);
        } else
            return QVariant(values[index.row()].count);
    } else if (role == SortRole) {
        static const QRegularExpression ignoredInSorting(QStringLiteral("[{}\\\\]+"));

        QString buffer = values[index.row()].sortBy.isEmpty() ? values[index.row()].text : values[index.row()].sortBy;
        buffer = buffer.remove(ignoredInSorting).toLower();

        if ((showCountColumn && index.column() == 1) || (!showCountColumn && sortBy == SortByCount)) {
            /// Sort by string consisting of a zero-padded count and the lower-case text,
            /// for example "0000000051keyword"
            /// Used if (a) two columns are shown (showCountColumn is true) and column 1
            /// (the count column) is to be sorted or (b) if only one column is shown
            /// (showCountColumn is false) and this single column is to be sorted by count.
            return QString(QStringLiteral("%1%2")).arg(values[index.row()].count, 10, 10, QLatin1Char('0')).arg(buffer);
        } else {
            /// Otherwise use lower-case text for sorting
            return QVariant(buffer);
        }
    } else if (role == SearchTextRole) {
        return QVariant(values[index.row()].text);
    } else if (role == Qt::EditRole)
        return QVariant::fromValue(values[index.row()].value);
    else if (role == CountRole)
        return QVariant(values[index.row()].count);
    else
        return QVariant();
}

bool ValueListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT_X(file != nullptr, "ValueListModel::setData", "You cannot set data if there is no BibTeX file associated with this value list.");

    /// Continue only if in edit role and first column is to be changed
    if (role == Qt::EditRole && index.column() == 0) {
        /// Fetch the string as it was shown before the editing started
        QString origText = data(index, Qt::DisplayRole).toString();
        /// Special treatment for colors
        if (fName == Entry::ftColor) {
            /// for colors, convert color (RGB) to the associated label
            QString color = colorToLabel.key(origText);
            if (!color.isEmpty()) origText = color;
        }

        /// Retrieve the Value object containing the user-entered data
        Value newValue = value.value<Value>(); /// nice variable names ... ;-)
        if (newValue.isEmpty()) {
            qCWarning(LOG_KBIBTEX_GUI) << "Cannot replace with empty value";
            return false;
        }

        /// Fetch the string representing the new, user-entered value
        const QString newText = PlainTextValue::text(newValue);
        if (newText == origText) {
            qCWarning(LOG_KBIBTEX_GUI) << "Skipping to replace value with itself";
            return false;
        }

        bool success = searchAndReplaceValueInEntries(index, newValue) && searchAndReplaceValueInModel(index, newValue);
        return success;
    }
    return false;
}

Qt::ItemFlags ValueListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    /// make first column editable
    if (index.column() == 0)
        result |= Qt::ItemIsEditable;
    return result;
}

QVariant ValueListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section >= 2 || orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    else if ((section == 0 && columnCount() == 2) || (columnCount() == 1 && sortBy == SortByText))
        return QVariant(i18n("Value"));
    else
        return QVariant(i18n("Count"));
}

void ValueListModel::removeValue(const QModelIndex &index)
{
    removeValueFromEntries(index);
    removeValueFromModel(index);
}

void ValueListModel::setShowCountColumn(bool showCountColumn)
{
    beginResetModel();
    this->showCountColumn = showCountColumn;
    endResetModel();
}

void ValueListModel::setSortBy(SortBy sortBy)
{
    beginResetModel();
    this->sortBy = sortBy;
    endResetModel();
}

void ValueListModel::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged) {
        beginResetModel();
        readConfiguration();
        endResetModel();
    }
}

void ValueListModel::readConfiguration()
{
    /// load mapping from color value to label
    KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
    KConfigGroup configGroup(config, Preferences::groupColor);
    QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
    QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultColorLabels);
    colorToLabel.clear();
    for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
        colorToLabel.insert(*itc, i18n((*itl).toUtf8().constData()));
    }
}

void ValueListModel::updateValues()
{
    values.clear();
    if (file == nullptr) return;

    for (const auto &element : const_cast<const File &>(*file)) {
        QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
        if (!entry.isNull()) {
            for (Entry::ConstIterator eit = entry->constBegin(); eit != entry->constEnd(); ++eit) {
                QString key = eit.key().toLower();
                if (key == fName) {
                    insertValue(eit.value());
                    break;
                }
                if (eit.value().isEmpty())
                    qCWarning(LOG_KBIBTEX_GUI) << "value for key" << key << "in entry" << entry->id() << "is empty";
            }
        }
    }
}

void ValueListModel::insertValue(const Value &value)
{
    for (const QSharedPointer<ValueItem> &item : value) {
        const QString text = PlainTextValue::text(*item);
        if (text.isEmpty()) continue; ///< skip empty values

        int index = indexOf(text);
        if (index < 0) {
            /// previously unknown text
            ValueLine newValueLine;
            newValueLine.text = text;
            newValueLine.count = 1;
            newValueLine.value.append(item);

            /// memorize sorting criterium:
            /// * for persons, use last name first
            /// * in any case, use lower case
            const QSharedPointer<Person> person = item.dynamicCast<Person>();
            newValueLine.sortBy = person.isNull() ? text.toLower() : person->lastName().toLower() + QStringLiteral(" ") + person->firstName().toLower();

            values << newValueLine;
        } else {
            ++values[index].count;
        }
    }
}

int ValueListModel::indexOf(const QString &text)
{
    QString color;
    QString cmpText = text;
    if (fName == Entry::ftColor && !(color = colorToLabel.key(text, QString())).isEmpty())
        cmpText = color;
    if (cmpText.isEmpty())
        qCWarning(LOG_KBIBTEX_GUI) << "Should never happen";

    int i = 0;
    /// this is really slow for large data sets: O(n^2)
    /// maybe use a hash table instead?
    for (const ValueLine &valueLine : const_cast<const ValueLineList &>(values)) {
        if (valueLine.text == cmpText)
            return i;
        ++i;
    }
    return -1;
}

bool ValueListModel::searchAndReplaceValueInEntries(const QModelIndex &index, const Value &newValue)
{
    /// Fetch the string representing the new, user-entered value
    const QString newText = PlainTextValue::text(newValue);

    if (newText.isEmpty())
        return false;

    /// Fetch the string as it was shown before the editing started
    QString origText = data(index, Qt::DisplayRole).toString();
    /// Special treatment for colors
    if (fName == Entry::ftColor) {
        /// for colors, convert color (RGB) to the associated label
        QString color = colorToLabel.key(origText);
        if (!color.isEmpty()) origText = color;
    }

    /// Go through all elements in the current file
    for (const QSharedPointer<Element> &element : const_cast<const File &>(*file)) {
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        /// Process only Entry objects
        if (!entry.isNull()) {
            /// Go through every key-value pair in entry (author, title, ...)
            for (Entry::Iterator eit = entry->begin(); eit != entry->end(); ++eit) {
                /// Fetch key-value pair's key
                const QString key = eit.key().toLower();
                /// Process only key-value pairs that are filtered for (e.g. only keywords)
                if (key == fName) {
                    eit.value().replace(origText, newValue.first());
                    break;
                }
            }
        }
    }

    return true;
}

bool ValueListModel::searchAndReplaceValueInModel(const QModelIndex &index, const Value &newValue)
{
    /// Fetch the string representing the new, user-entered value
    const QString newText = PlainTextValue::text(newValue);
    if (newText.isEmpty())
        return false;

    const int row = index.row();

    /// Test if user-entered text exists already in model's data
    /// newTextAlreadyInListIndex will be row of duplicate or
    /// -1 if new text is unique
    int newTextAlreadyInListIndex = -1;
    for (int r = values.count() - 1; newTextAlreadyInListIndex < 0 && r >= 0; --r) {
        if (row != r && values[r].text == newText)
            newTextAlreadyInListIndex = r;
    }

    if (newTextAlreadyInListIndex < 0) {
        /// User-entered text is unique, so simply replace
        /// old text with new text
        values[row].text = newText;
        values[row].value = newValue;
        const QSharedPointer<Person> person = newValue.first().dynamicCast<Person>();
        values[row].sortBy = person.isNull() ? QString() : person->lastName() + QStringLiteral(" ") + person->firstName();
    } else {
        /// The user-entered text existed before

        const int lastRow = values.count() - 1;
        if (row != lastRow) {
            /// Unless duplicate is last one in list,
            /// overwrite edited row with last row's value
            values[row].text = values[lastRow].text;
            values[row].value = values[lastRow].value;
            values[row].sortBy = values[lastRow].sortBy;
        }

        /// Remove last row, which is no longer used
        beginRemoveRows(QModelIndex(), lastRow, lastRow);
        values.remove(lastRow);
        endRemoveRows();
    }

    /// Notify Qt about data changed
    emit dataChanged(index, index);

    return true;
}

void ValueListModel::removeValueFromEntries(const QModelIndex &index)
{
    /// Retrieve the Value object containing the user-entered data
    const Value toBeDeletedValue = values[index.row()].value;
    if (toBeDeletedValue.isEmpty()) {
        return;
    }
    const QString toBeDeletedText = PlainTextValue::text(toBeDeletedValue);
    if (toBeDeletedText.isEmpty()) {
        return;
    }

    /// Go through all elements in the current file
    for (const QSharedPointer<Element> &element : const_cast<const File &>(*file)) {
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        /// Process only Entry objects
        if (!entry.isNull()) {
            /// Go through every key-value pair in entry (author, title, ...)
            for (Entry::Iterator eit = entry->begin(); eit != entry->end(); ++eit) {
                /// Fetch key-value pair's key
                const QString key = eit.key().toLower();
                /// Process only key-value pairs that are filtered for (e.g. only keywords)
                if (key == fName) {
                    /// Fetch the key-value pair's value's textual representation
                    const QString valueFullText = PlainTextValue::text(eit.value());
                    if (valueFullText == toBeDeletedText) {
                        /// If the key-value pair's value's textual representation is the same
                        /// as the value to be delted, remove this key-value pair
                        /// This test is usually true for keys like title, year, or edition.
                        entry->remove(key); /// This would break the Iterator, but code "breakes" from loop anyways
                    } else {
                        /// The test above failed, but the delete operation may have
                        /// to be applied to a ValueItem inside the value.
                        /// Possible keys for such a case include author, editor, or keywords.

                        /// Process each ValueItem inside this Value
                        for (Value::Iterator vit = eit.value().begin(); vit != eit.value().end();) {
                            /// Similar procedure as for full values above:
                            /// If a ValueItem's textual representation is the same
                            /// as the shown string which has be deleted, remove the
                            /// ValueItem from this Value. If the Value becomes empty,
                            /// remove Value as well.
                            const QString valueItemText = PlainTextValue::text(* (*vit));
                            if (valueItemText == toBeDeletedText) {
                                /// Erase old ValueItem from this Value
                                vit = eit.value().erase(vit);
                            } else
                                ++vit;
                        }

                        if (eit.value().isEmpty()) {
                            /// This value does no longer contain any ValueItems.
                            entry->remove(key); /// This would break the Iterator, but code "breakes" from loop anyways
                        }
                    }
                    break;
                }
            }
        }
    }
}

void ValueListModel::removeValueFromModel(const QModelIndex &index)
{
    const int row = index.row();
    const int lastRow = values.count() - 1;

    if (row != lastRow) {
        /// Unless duplicate is last one in list,
        /// overwrite edited row with last row's value
        values[row].text = values[lastRow].text;
        values[row].value = values[lastRow].value;
        values[row].sortBy = values[lastRow].sortBy;

        emit dataChanged(index, index);
    }

    /// Remove last row, which is no longer used
    beginRemoveRows(QModelIndex(), lastRow, lastRow);
    values.remove(lastRow);
    endRemoveRows();
}
