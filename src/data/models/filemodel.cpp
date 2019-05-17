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

#include "filemodel.h"

#include <algorithm>

#include <QColor>
#include <QFile>
#include <QString>

#include <KLocalizedString>

#include <BibTeXEntries>
#include <BibTeXFields>
#include <Preferences>
#include "file.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"
#include "preamble.h"

const int FileModel::NumberRole = Qt::UserRole + 9581;
const int FileModel::SortRole = Qt::UserRole + 236; /// see also MDIWidget's SortRole


FileModel::FileModel(QObject *parent)
        : QAbstractTableModel(parent), m_file(nullptr)
{
    NotificationHub::registerNotificationListener(this);
    readConfiguration();
}

void FileModel::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged) {
        readConfiguration();
        int column = 0;
        for (const auto &fd : const_cast<const BibTeXFields &>(BibTeXFields::instance())) {
            /// Colors may have changed
            bool columnChanged = fd.upperCamelCase.toLower() == Entry::ftColor;
            /// Person name formatting may has changed
            columnChanged |= fd.upperCamelCase.toLower() == Entry::ftAuthor || fd.upperCamelCase.toLower() == Entry::ftEditor;
            columnChanged |= fd.upperCamelCaseAlt.toLower() == Entry::ftAuthor || fd.upperCamelCaseAlt.toLower() == Entry::ftEditor;
            /// Changes necessary for this column? Publish update
            if (columnChanged)
                emit dataChanged(index(0, column), index(rowCount() - 1, column));
            ++column;
        }
    } else if (eventId == NotificationHub::EventBibliographySystemChanged)
        emit headerDataChanged(Qt::Horizontal, 0, 0xffff);
}

void FileModel::readConfiguration()
{
    colorToLabel.clear();
    for (QVector<QPair<QColor, QString>>::ConstIterator it = Preferences::instance().colorCodes().constBegin(); it != Preferences::instance().colorCodes().constEnd(); ++it)
        colorToLabel.insert(it->first.name(), it->second);
}

QString FileModel::entryText(const Entry *entry, const QString &raw, const QString &rawAlt, const QStringList &rawAliases, int role, bool followCrossRef) const
{
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != SortRole)
        return QString();

    if (raw == QStringLiteral("^id")) {
        return entry->id();
    } else if (raw == QStringLiteral("^type")) { // FIXME: Use constant here?
        /// Try to beautify type, e.g. translate "proceedings" into
        /// "Conference or Workshop Proceedings"
        const QString label = BibTeXEntries::instance().label(entry->type());
        if (label.isEmpty()) {
            /// Fall-back to entry type as it is
            return entry->type();
        } else
            return label;
    } else if (raw.toLower() == Entry::ftStarRating) {
        return QString(); /// Stars have no string representation
    } else if (raw.toLower() == Entry::ftColor) {
        const QString text = PlainTextValue::text(entry->value(raw));
        if (text.isEmpty()) return QString();
        const QString colorText = colorToLabel[text];
        if (colorText.isEmpty()) return text;
        return colorText;
    } else {
        QString text;
        if (entry->contains(raw))
            text = PlainTextValue::text(entry->value(raw)).simplified();
        else if (!rawAlt.isEmpty() && entry->contains(rawAlt))
            text = PlainTextValue::text(entry->value(rawAlt)).simplified();
        if (text.isEmpty())
            for (const QString &alias : rawAliases) {
                if (entry->contains(alias)) {
                    text = PlainTextValue::text(entry->value(alias)).simplified();
                    if (!text.isEmpty()) break;
                }
            }

        if (followCrossRef && text.isEmpty() && entry->contains(Entry::ftCrossRef)) {
            QScopedPointer<const Entry> completedEntry(entry->resolveCrossref(m_file));
            return entryText(completedEntry.data(), raw, rawAlt, rawAliases, role, false);
        }

        if (text.isEmpty())
            return QString();
        else if (role == FileModel::SortRole)
            return text.toLower();
        else if (role == Qt::ToolTipRole) {
            // TODO: find a better solution, such as line-wrapping tooltips
            return KBibTeX::leftSqueezeText(text, 128);
        } else
            return text;
    }

}

File *FileModel::bibliographyFile() const
{
    return m_file;
}

void FileModel::setBibliographyFile(File *file)
{
    bool resetNecessary = m_file != file;
    if (resetNecessary) {
        beginResetModel();
        m_file = file;
        endResetModel();
    }
}

QModelIndex FileModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool FileModel::hasChildren(const QModelIndex &parent) const
{
    return parent == QModelIndex();
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_file != nullptr ? m_file->count() : 0;
}

int FileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return BibTeXFields::instance().count();
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    /// do not accept invalid indices
    if (!index.isValid())
        return QVariant();

    /// check backend storage (File object)
    if (m_file == nullptr)
        return QVariant();

    /// for now, only display data (no editing or icons etc)
    if (role != NumberRole && role != SortRole && role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
        return QVariant();

    if (index.row() < m_file->count() && index.column() < BibTeXFields::instance().count()) {
        const FieldDescription &fd = BibTeXFields::instance().at(index.column());
        const QString &raw = fd.upperCamelCase;
        const QString &rawAlt = fd.upperCamelCaseAlt;
        const QStringList &rawAliases = fd.upperCamelCaseAliases;
        QSharedPointer<Element> element = (*m_file)[index.row()];
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();

        /// if BibTeX entry has a "x-color" field, use that color to highlight row
        if (role == Qt::BackgroundRole) {
            /// Retrieve "color"
            QString colorName;
            if (entry.isNull() || (colorName = PlainTextValue::text(entry->value(Entry::ftColor))) == QStringLiteral("#000000") || colorName.isEmpty())
                return QVariant();
            else {
                /// There is a valid color, set it as background
                QColor color(colorName);
                /// Use slightly different colors for even and odd rows
                color.setAlphaF(index.row() % 2 == 0 ? 0.75 : 1.0);
                return QVariant(color);
            }
        } else if (role == Qt::ForegroundRole) {
            /// Retrieve "color"
            QString colorName;
            if (entry.isNull() || (colorName = PlainTextValue::text(entry->value(Entry::ftColor))) == QStringLiteral("#000000") || colorName.isEmpty())
                return QVariant();
            else {
                /// There is a valid color ...
                const QColor color(colorName);
                /// Retrieve red, green, blue, and alpha components
                int r = 0, g = 0, b = 0, a = 0;
                color.getRgb(&r, &g, &b, &a);
                /// If gray value is rather dark, return white as foreground color
                if (qGray(r, g, b) < 128) return QColor(Qt::white);
                /// For light gray values, return black as foreground color
                else return QColor(Qt::black);
            }
        } else if (role == NumberRole) {
            if (!entry.isNull() && raw.toLower() == Entry::ftStarRating) {
                const QString text = PlainTextValue::text(entry->value(raw)).simplified();
                bool ok = false;
                const double numValue = text.toDouble(&ok);
                if (ok)
                    return QVariant::fromValue<double>(numValue);
                else
                    return QVariant();
            } else
                return QVariant();
        }

        /// The only roles left at this point shall be SortRole, Qt::DisplayRole, and Qt::ToolTipRole

        if (!entry.isNull()) {
            return QVariant(entryText(entry.data(), raw, rawAlt, rawAliases, role, true));
        } else {
            QSharedPointer<Macro> macro = element.dynamicCast<Macro>();
            if (!macro.isNull()) {
                if (raw == QStringLiteral("^id"))
                    return QVariant(macro->key());
                else if (raw == QStringLiteral("^type"))
                    return QVariant(i18n("Macro"));
                else if (raw == QStringLiteral("Title")) {
                    const QString text = PlainTextValue::text(macro->value()).simplified();
                    return QVariant(text);
                } else
                    return QVariant();
            } else {
                QSharedPointer<Comment> comment = element.dynamicCast<Comment>();
                if (!comment.isNull()) {
                    if (raw == QStringLiteral("^type"))
                        return QVariant(i18n("Comment"));
                    else if (raw == Entry::ftTitle) {
                        const QString text = comment->text().simplified();
                        return QVariant(text);
                    } else
                        return QVariant();
                } else {
                    QSharedPointer<Preamble> preamble = element.dynamicCast<Preamble>();
                    if (!preamble.isNull()) {
                        if (raw == QStringLiteral("^type"))
                            return QVariant(i18n("Preamble"));
                        else if (raw == Entry::ftTitle) {
                            const QString text = PlainTextValue::text(preamble->value()).simplified();
                            return QVariant(text);
                        } else
                            return QVariant();
                    } else
                        return QVariant("?");
                }
            }
        }
    } else
        return QVariant("?");
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= BibTeXFields::instance().count())
        return QVariant();
    return BibTeXFields::instance().at(section).label;
}

Qt::ItemFlags FileModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // FIXME: What about drag'n'drop?
}

void FileModel::clear() {
    beginResetModel();
    m_file->clear();
    endResetModel();
}

bool FileModel::removeRow(int row, const QModelIndex &parent)
{
    if (row < 0 || m_file == nullptr || row >= rowCount() || row >= m_file->count())
        return false;
    if (parent != QModelIndex())
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    m_file->removeAt(row);
    endRemoveRows();

    return true;
}

bool FileModel::removeRowList(const QList<int> &rows)
{
    if (m_file == nullptr) return false;

    QList<int> internalRows = rows;
    std::sort(internalRows.begin(), internalRows.end(), std::greater<int>());

    beginRemoveRows(QModelIndex(), internalRows.last(), internalRows.first());
    for (int row : const_cast<const QList<int> &>(internalRows)) {
        if (row < 0 || row >= rowCount() || row >= m_file->count())
            return false;
        m_file->removeAt(row);
    }
    endRemoveRows();

    return true;
}

bool FileModel::insertRow(QSharedPointer<Element> element, int row, const QModelIndex &parent)
{
    if (m_file == nullptr || row < 0 || row > rowCount() || parent != QModelIndex())
        return false;

    /// Check for duplicate ids or keys when inserting a new element
    /// First, check entries
    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (!entry.isNull()) {
        /// Fetch current entry's id
        const QString id = entry->id();
        if (!m_file->containsKey(id).isNull()) {
            /// Same entry id used for an existing entry or macro
            int overflow = 2;
            static const QString pattern = QStringLiteral("%1_%2");
            /// Test alternative ids with increasing "overflow" counter:
            /// id_2, id_3, id_4 ,...
            QString newId = pattern.arg(id).arg(overflow);
            while (!m_file->containsKey(newId).isNull()) {
                ++overflow;
                newId = pattern.arg(id).arg(overflow);
            }
            /// Guaranteed to find an alternative, apply it to entry
            entry->setId(newId);
        }
    } else {
        /// Next, check macros
        QSharedPointer<Macro> macro = element.dynamicCast<Macro>();
        if (!macro.isNull()) {
            /// Fetch current macro's key
            const QString key = macro->key();
            if (!m_file->containsKey(key).isNull()) {
                /// Same entry key used for an existing entry or macro
                int overflow = 2;
                static const QString pattern = QStringLiteral("%1_%2");
                /// Test alternative keys with increasing "overflow" counter:
                /// key_2, key_3, key_4 ,...
                QString newKey = pattern.arg(key).arg(overflow);
                while (!m_file->containsKey(newKey).isNull()) {
                    ++overflow;
                    newKey = pattern.arg(key).arg(overflow);
                }
                /// Guaranteed to find an alternative, apply it to macro
                macro->setKey(newKey);
            }
        }
    }

    beginInsertRows(QModelIndex(), row, row);
    m_file->insert(row, element);
    endInsertRows();

    return true;
}

QSharedPointer<Element> FileModel::element(int row) const
{
    if (m_file == nullptr || row < 0 || row >= m_file->count()) return QSharedPointer<Element>();

    return (*m_file)[row];
}

int FileModel::row(QSharedPointer<Element> element) const
{
    if (m_file == nullptr) return -1;
    return m_file->indexOf(element);
}

void FileModel::elementChanged(int row) {
    emit dataChanged(createIndex(row, 0), createIndex(row, columnCount() - 1));
}
