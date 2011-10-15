/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

QSize ValueListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), option.fontMetrics.height()*3 / 2)); // TODO calculate height better
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

int ValueListModel::rowCount(const QModelIndex & parent) const
{
    return parent == QModelIndex() ? values.count() : 0;
}

int ValueListModel::columnCount(const QModelIndex & parent) const
{
    return parent == QModelIndex() ? (showCountColumn ? 2 : 1) : 0;
}

QVariant ValueListModel::data(const QModelIndex & index, int role) const
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

bool ValueListModel::setData(const QModelIndex & index, const QVariant &value, int role)
{
    Q_ASSERT_X(file != NULL, "ValueListModel::setData", "You cannot set data if there is no BibTeX file associated with this value list.");

    if (role == Qt::EditRole && index.column() == 0) {
        Value newValue = value.value<Value>(); /// nice names ... ;-)
        QString origText = data(index, Qt::DisplayRole).toString();
        if (fName == Entry::ftColor) {
            QString color = colorToLabel.key(origText);
            if (!color.isEmpty()) origText = color;
        }
        int index = indexOf(origText); Q_ASSERT(index >= 0);
        const QString newText = PlainTextValue::text(newValue);
        kDebug() << "replacing" << origText << "with" << newText << "for field" << fName;

        foreach(Element *element, *file) {
            Entry *entry = dynamic_cast< Entry*>(element);
            if (entry != NULL) {
                for (Entry::Iterator eit = entry->begin(); eit != entry->end(); ++eit) {
                    QString key = eit.key().toLower();
                    if (key == fName) {
                        const QString valueFullText = PlainTextValue::text(eit.value());
                        if (valueFullText == origText)
                            entry->insert(key, newValue);
                        else {
                            for (Value::Iterator vit = eit.value().begin(); vit != eit.value().end(); ++vit) {
                                const QString valueItemText = PlainTextValue::text(*(*vit));
                                if (valueItemText == origText) {
                                    vit = eit.value().erase(vit);
                                    vit = eit.value().insert(vit, newValue.first());
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        values[index].text = newText;
        values[index].value = newValue;
        const Person *person = dynamic_cast<const Person*>(newValue.first());
        values[index].sortBy = person == NULL ? QString::null : person->lastName() + QLatin1String(" ") + person->firstName();
        reset();

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
        const Entry *entry = dynamic_cast<const Entry*>(*fit);
        if (entry != NULL) {
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
    foreach(ValueItem *item, value) {
        const QString text = PlainTextValue::text(*item, file);
        if (text.isEmpty()) continue; ///< skip empty values

        int index = indexOf(text);
        if (index < 0) {
            /// previously unknown text
            ValueLine newValueLine;
            newValueLine.text = text;
            newValueLine.count = 1;
            Value v;
            v.append(item);
            newValueLine.value = v;

            /// memorize sorting criterium:
            /// * for persons, use last name first
            /// * in any case, use lower case
            const Person *person = dynamic_cast<const Person*>(item);
            newValueLine.sortBy = person == NULL ? text.toLower() : person->lastName().toLower() + QLatin1String(" ") + person->firstName().toLower();

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
