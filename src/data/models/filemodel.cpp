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

#include "filemodel.h"

#include <algorithm>

#include <QColor>
#include <QFile>
#include <QString>

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>

#include "element.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"
#include "preamble.h"
#include "bibtexentries.h"
#include "bibtexfields.h"
#include "preferences.h"

const int FileModel::NumberRole = Qt::UserRole + 9581;
const int FileModel::SortRole = Qt::UserRole + 236; /// see also MDIWidget's SortRole

const QString FileModel::keyShowComments = QStringLiteral("showComments");
const bool FileModel::defaultShowComments = true;
const QString FileModel::keyShowMacros = QStringLiteral("showMacros");
const bool FileModel::defaultShowMacros = true;


FileModel::FileModel(QObject *parent)
        : QAbstractTableModel(parent), m_file(nullptr)
{
    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    readConfiguration();
}

void FileModel::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged) {
        readConfiguration();
        int column = 0;
        const BibTeXFields *bf = BibTeXFields::self();
        for (const auto &fd : const_cast<const BibTeXFields &>(*bf)) {
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
    }
}

void FileModel::readConfiguration()
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

QVariant FileModel::entryData(const Entry *entry, const QString &raw, const QString &rawAlt, int role, bool followCrossRef) const
{
    if (raw == QStringLiteral("^id")) // FIXME: Use constant here?
        return QVariant(entry->id());
    else if (raw == QStringLiteral("^type")) { // FIXME: Use constant here?
        /// try to beautify type, e.g. translate "proceedings" into
        /// "Conference or Workshop Proceedings"
        QString label = BibTeXEntries::self()->label(entry->type());
        if (label.isEmpty()) {
            /// fall-back to entry type as it is
            return QVariant(entry->type());
        } else
            return QVariant(label);
    } else if (raw.toLower() == Entry::ftStarRating) {
        return QVariant();
    } else if (raw.toLower() == Entry::ftColor) {
        QString text = PlainTextValue::text(entry->value(raw));
        if (text.isEmpty()) return QVariant();
        QString colorText = colorToLabel[text];
        if (colorText.isEmpty()) return QVariant(text);
        return QVariant(colorText);
    } else {
        QString text;
        if (entry->contains(raw))
            text = PlainTextValue::text(entry->value(raw)).simplified();
        else if (!rawAlt.isEmpty() && entry->contains(rawAlt))
            text = PlainTextValue::text(entry->value(rawAlt)).simplified();

        if (followCrossRef && text.isEmpty() && entry->contains(Entry::ftCrossRef)) {
            // TODO do not only follow "crossref", but other files from Biber/Biblatex as well
            Entry *completedEntry = entry->resolveCrossref(m_file);
            QVariant result = entryData(completedEntry, raw, rawAlt, role, false);
            delete completedEntry;
            return result;
        }

        if (text.isEmpty())
            return QVariant();
        else if (role == FileModel::SortRole)
            return QVariant(text.toLower());
        else if (role == Qt::ToolTipRole) {
            // TODO: find a better solution, such as line-wrapping tooltips
            return QVariant(KBibTeX::leftSqueezeText(text, 128));
        } else
            return QVariant(text);
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
    return BibTeXFields::self()->count();
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

    const BibTeXFields *bibtexFields = BibTeXFields::self();
    if (index.row() < m_file->count() && index.column() < bibtexFields->count()) {
        const FieldDescription &fd = bibtexFields->at(index.column());
        QString raw = fd.upperCamelCase;
        QString rawAlt = fd.upperCamelCaseAlt;
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

        if (!entry.isNull()) {
            return entryData(entry.data(), raw, rawAlt, role, true);
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
    const BibTeXFields *bibtexFields = BibTeXFields::self();
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= bibtexFields->count())
        return QVariant();
    return bibtexFields->at(section).label;
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
