/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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

#include <typeinfo>

#include <KComboBox>
#include <KLocale>
#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KColorScheme>

#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QListView>
#include <QGridLayout>
#include <QStringListModel>
#include <QLineEdit>
#include <QPainter>
#include <QFrame>
#include <QLayout>
#include <QHeaderView>

#include <fieldlineedit.h>
#include <bibtexfields.h>
#include <entry.h>
#include <preferences.h>
#include "valuelistmodel.h"

const int CountRole = Qt::UserRole + 611;

QWidget *ValueListDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &sovi, const QModelIndex &index) const
{
    if (index.column() == 0) {
        const FieldDescription *fd = BibTeXFields::self()->find(m_fieldName);
        FieldLineEdit *fieldLineEdit = new FieldLineEdit(fd->preferredTypeFlag, fd->typeFlags, false, parent);
        fieldLineEdit->setAutoFillBackground(true);
        return fieldLineEdit;
    } else
        return QStyledItemDelegate::createEditor(parent, sovi, index);
}

void ValueListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 0) {
        FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit*>(editor);
        if (fieldLineEdit != NULL)
            fieldLineEdit->reset(index.model()->data(index, Qt::EditRole).value<Value>());
    }
}

void ValueListDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    FieldLineEdit *fieldLineEdit = qobject_cast<FieldLineEdit*>(editor);
    if (fieldLineEdit != NULL) {
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
    QLineEdit *editor = qobject_cast<QLineEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

void ValueListDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 *noTextOption = qstyleoption_cast<QStyleOptionViewItemV4 *>(option);
    QStyledItemDelegate::initStyleOption(noTextOption, index);
    if (option->decorationPosition != QStyleOptionViewItem::Top) {
        /// remove text from style (do not draw text)
        noTextOption->text.clear();
    }

}

void ValueListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    /// code heavily inspired by kdepimlibs-4.6.3/akonadi/collectionstatisticsdelegate.cpp

    /// save painter's state, restored before leaving this function
    painter->save();

    /// first, paint the basic, but without the text. We remove the text
    /// in initStyleOption(), which gets called by QStyledItemDelegate::paint().
    QStyledItemDelegate::paint(painter, option, index);

    /// now, we retrieve the correct style option by calling intiStyleOption from
    /// the superclass.
    QStyleOptionViewItemV4 option4 = option;
    QStyledItemDelegate::initStyleOption(&option4, index);
    QString field = option4.text;

    /// now calculate the rectangle for the text
    QStyle *s = m_parent->style();
    const QWidget *widget = option4.widget;
    const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &option4, widget);

    if (option.state & QStyle::State_Selected) {
        /// selected lines are drawn with different color
        painter->setPen(option.palette.highlightedText().color());
    }

    /// count will be empty unless only one column is shown
    const QString count = index.column() == 0 && index.model()->columnCount() == 1 ? QString(QLatin1String(" (%1)")).arg(index.data(CountRole).toInt()) : QLatin1String("");

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

static QRegExp ignoredInSorting("[{}\\\\]+");

ValueListModel::ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent)
        : QAbstractTableModel(parent), file(bibtexFile), fName(fieldName.toLower()), showCountColumn(true), sortBy(SortByText)
{
    /// load mapping from color value to label
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    KConfigGroup configGroup(config, Preferences::groupColor);
    QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
    QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultcolorLabels);
    for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
        colorToLabel.insert(*itc, *itl);
    }

    updateValues();
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
        if ((showCountColumn && index.column() == 0) || (!showCountColumn && sortBy == SortByText)) {
            QString buffer = values[index.row()].sortBy.isNull() ? values[index.row()].text : values[index.row()].sortBy;
            return QVariant(buffer.replace(ignoredInSorting, ""));
        } else
            return QVariant(values[index.row()].count);
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
    Q_ASSERT_X(file != NULL, "ValueListModel::setData", "You cannot set data if there is no BibTeX file associated with this value list.");

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
        /// Memorize the current row
        const int row = index.row();

        /// Retrieve the Value object containing the user-entered data
        Value newValue = value.value<Value>(); /// nice variable names ... ;-)
        /// Fetch the string representing the new, user-entered value
        const QString newText = PlainTextValue::text(newValue);

        if (newText == origText) {
            kDebug() << "Skipping to replace value with itself";
            return false;
        }

        /**
         * Update current entry's values
         */

        /// Go through all elements in the current file
        foreach(QSharedPointer<Element> element, *file) {
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
                        if (valueFullText == origText) {
                            /// If the key-value pair's value's textual representation is the same
                            /// as the shown string which will be replaced, replace the key-value pair
                            /// in the entry with a new key-value pair containing the new value.
                            /// This test is usually true for keys like title, year, or edition.
                            entry->insert(key, newValue);
                        } else {
                            /// The test above failed, but the replacement may have to be applied to
                            /// a ValueItem inside the value.
                            /// Possible keys for such a case include author, editor, or keywords.

                            /// Keep track of unique ValueItems (as determined by their textual
                            /// representation)
                            QSet<QString> uniqueValueItemTexts;

                            /// Process each ValueItem inside this Value
                            for (Value::Iterator vit = eit.value().begin(); vit != eit.value().end();) {
                                /// Similar procedure as for full values above:
                                /// If a ValueItem's textual representation is the same
                                /// as the shown string which will be replaced, replace the
                                /// ValueItem in this Value with a new Value containing the new string.
                                const QString valueItemText = PlainTextValue::text(* (*vit));
                                if (valueItemText == origText) {
                                    /// Erase old ValueItem from this Value
                                    vit = eit.value().erase(vit);
                                    /// Insert new ValueItem (replacement text) only if
                                    /// it is unique to this Value (did not occurred before)
                                    if (!uniqueValueItemTexts.contains(newText)) {
                                        uniqueValueItemTexts.insert(newText);
                                        vit = eit.value().insert(vit, newValue.first());
                                        ++vit;
                                    }
                                } else if (uniqueValueItemTexts.contains(valueItemText)) {
                                    /// Due to a replace operation above, an old ValueItem's text
                                    /// matches a text which was inserted as a "newValue".
                                    /// Therefore, remove the old ValueItem to avoid duplicates
                                    vit = eit.value().erase(vit);
                                } else {
                                    /// Neither a replacement, nor a duplicate. Keep this
                                    /// ValueItem (memorize as unique) and continue.
                                    uniqueValueItemTexts.insert(valueItemText);
                                    ++vit;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        /**
         * Update GUI (data for this model)
         */

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
            values[row].sortBy = person.isNull() ? QString::null : person->lastName() + QLatin1String(" ") + person->firstName();
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

        /// Return true as replace operation was successful
        return true;
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

void ValueListModel::setShowCountColumn(bool showCountColumn)
{
    this->showCountColumn = showCountColumn;
    reset();
}

void ValueListModel::setSortBy(SortBy sortBy)
{
    this->sortBy = sortBy;
    reset();
}

void ValueListModel::updateValues()
{
    values.clear();
    if (file == NULL) return;

    for (File::ConstIterator fit = file->constBegin(); fit != file->constEnd(); ++fit) {
        QSharedPointer<const Entry> entry = (*fit).dynamicCast<const Entry>();
        if (!entry.isNull()) {
            for (Entry::ConstIterator eit = entry->constBegin(); eit != entry->constEnd(); ++eit) {
                QString key = eit.key().toLower();
                if (key == fName) {
                    insertValue(eit.value());
                    break;
                }
                if (eit.value().isEmpty())
                    kWarning() << "value for key" << key << "in entry" << entry->id() << "is empty";
            }
        }
    }
}

void ValueListModel::insertValue(const Value &value)
{
    foreach(QSharedPointer<ValueItem> item, value) {
        const QString text = PlainTextValue::text(*item, file);
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
            newValueLine.sortBy = person.isNull() ? text.toLower() : person->lastName().toLower() + QLatin1String(" ") + person->firstName().toLower();

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
    if (fName == Entry::ftColor && !(color = colorToLabel.key(text, QLatin1String(""))).isEmpty())
        cmpText = color;
    if (cmpText.isEmpty())
        kWarning() << "Should never happen";

    int i = 0;
    /// this is really slow for large data sets: O(n^2)
    /// maybe use a hash table instead?
    foreach(const ValueLine &valueLine, values) {
        if (valueLine.text == cmpText)
            return i;
        ++i;
    }
    return -1;
}
