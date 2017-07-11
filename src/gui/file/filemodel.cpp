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

#include "filemodel.h"

#include <typeinfo>

#include <QColor>
#include <QFile>
#include <QString>

#include <KLocale>
#include <KConfigGroup>

#include "starrating.h"
#include "element.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"
#include "preamble.h"
#include "bibtexentries.h"
#include "bibtexfields.h"
#include "fileinfo.h"
#include "preferences.h"

const QString SortFilterFileModel::configGroupName = QLatin1String("User Interface");

SortFilterFileModel::SortFilterFileModel(QObject *parent)
        : QSortFilterProxyModel(parent), m_internalModel(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")))
{
    m_filterQuery.combination = AnyTerm;
    loadState();
    setSortRole(FileModel::SortRole);
}

void SortFilterFileModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);
    m_internalModel = dynamic_cast<FileModel *>(model);
}

FileModel *SortFilterFileModel::fileSourceModel() const
{
    return m_internalModel;
}

void SortFilterFileModel::updateFilter(SortFilterFileModel::FilterQuery filterQuery)
{
    m_filterQuery = filterQuery;
    m_filterQuery.field = filterQuery.field.toLower(); /// required for comparison in filter code
    invalidate();
}

bool SortFilterFileModel::simpleLessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QString leftString = left.data(Qt::DisplayRole).toString().toLower();
    const QString rightString = right.data(Qt::DisplayRole).toString().toLower();
    const int cmp = QString::localeAwareCompare(leftString, rightString);
    if (cmp == 0)
        return left.row() < right.row();
    else
        return cmp < 0;
}

bool SortFilterFileModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    int column = left.column();
    Q_ASSERT_X(left.column() == right.column(), "bool SortFilterFileModel::lessThan(const QModelIndex &left, const QModelIndex &right) const", "Not comparing items in same column"); ///< assume that we only sort by column

    const BibTeXFields *bibtexFields = BibTeXFields::self();
    const FieldDescription *fd = bibtexFields->at(column);

    if (column == right.column() && (fd->upperCamelCase == QLatin1String("Author") || fd->upperCamelCase == QLatin1String("Editor"))) {
        /// special sorting for authors or editors: check all names,
        /// compare last and then first names

        /// first, check if two entries (and not e.g. comments) are to be compared
        QSharedPointer<Entry> entryA = m_internalModel->element(left.row()).dynamicCast<Entry>();
        QSharedPointer<Entry> entryB = m_internalModel->element(right.row()).dynamicCast<Entry>();
        if (entryA.isNull() || entryB.isNull())
            return simpleLessThan(left, right);

        /// retrieve values of both cells
        Value valueA = entryA->value(fd->upperCamelCase);
        Value valueB = entryB->value(fd->upperCamelCase);
        if (valueA.isEmpty())
            valueA = entryA->value(fd->upperCamelCaseAlt);
        if (valueB.isEmpty())
            valueB = entryB->value(fd->upperCamelCaseAlt);

        /// if either value is empty, use default implementation
        if (valueA.isEmpty() || valueB.isEmpty())
            return simpleLessThan(left, right);

        /// compare each person in both values
        for (Value::Iterator itA = valueA.begin(), itB = valueB.begin(); itA != valueA.end() &&  itB != valueB.end(); ++itA, ++itB) {
            static const QRegExp curlyRegExp(QLatin1String("[{}]+"));

            QSharedPointer<Person>  personA = (*itA).dynamicCast<Person>();
            QSharedPointer<Person>  personB = (*itB).dynamicCast<Person>();
            /// not a Person object in value? fall back to default implementation
            if (personA.isNull() || personB.isNull()) return QSortFilterProxyModel::lessThan(left, right);

            /// get both values' next persons' last names for comparison
            QString nameA = personA->lastName().remove(curlyRegExp).toLower();
            QString nameB = personB->lastName().remove(curlyRegExp).toLower();
            int cmp = QString::localeAwareCompare(nameA, nameB);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            /// if last names were inconclusive ...
            /// get both values' next persons' first names for comparison
            nameA = personA->firstName().remove(curlyRegExp).toLower();
            nameB = personB->firstName().remove(curlyRegExp).toLower();
            cmp = QString::localeAwareCompare(nameA, nameB);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            // TODO Check for suffix and prefix?
        }

        /// comparison by names did not work (was not conclusive)
        /// fall back to default implementation
        return simpleLessThan(left, right);
    } else {
        /// if comparing two numbers, do not perform lexicographical sorting (i.e. 13 < 2),
        /// but numerical sorting instead (i.e. 13 > 2)
        const QString textLeft = left.data(Qt::DisplayRole).toString();
        const QString textRight = right.data(Qt::DisplayRole).toString();
        bool okLeft = false, okRight = false;
        int numberLeft = textLeft.toInt(&okLeft);
        int numberRight = textRight.toInt(&okRight);
        if (okLeft && okRight)
            return numberLeft < numberRight;


        /// everything else can be sorted by default implementation
        /// (i.e. alphabetically or lexicographically)
        return simpleLessThan(left, right);
    }
}

bool SortFilterFileModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)

    QSharedPointer<Element> rowElement = m_internalModel->element(source_row);
    Q_ASSERT_X(!rowElement.isNull(), "bool SortFilterFileModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const", "rowElement is NULL");

    /// check if showing comments is disabled
    if (!m_showComments && Comment::isComment(*rowElement))
        return false;
    /// check if showing macros is disabled
    if (!m_showMacros && Macro::isMacro(*rowElement))
        return false;

    if (m_filterQuery.terms.isEmpty()) return true; /// empty filter query

    bool *eachTerm = new bool[m_filterQuery.terms.count()];
    for (int i = m_filterQuery.terms.count() - 1; i >= 0; --i)
        eachTerm[i] = false;

    QSharedPointer<Entry> entry = rowElement.dynamicCast<Entry>();
    if (!entry.isNull()) {
        /// if current row contains an Entry ...

        if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^id")) {
            /// Check entry's id
            const QString id = entry->id();
            int i = 0;
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                eachTerm[i] |= (*itsl).isEmpty() ? true : id.contains(*itsl, Qt::CaseInsensitive);
        }

        if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
            /// Check entry's type
            const QString type = entry->type();
            /// Check type's description ("Journal Article")
            const QString label = BibTeXEntries::self()->label(type);
            // TODO test for internationalized variants like "Artikel" or "bok" as well?
            int i = 0;
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                eachTerm[i] |= (*itsl).isEmpty() ? true : type.contains(*itsl, Qt::CaseInsensitive) || label.contains(*itsl, Qt::CaseInsensitive);
        }

        for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
            if (m_filterQuery.field.isEmpty() || m_filterQuery.field == it.key().toLower()) {
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                    eachTerm[i] |= (*itsl).isEmpty() ? true : it.value().containsPattern(*itsl);
            }

        /// Test associated PDF files
        if (m_filterQuery.searchPDFfiles && m_filterQuery.field.isEmpty()) ///< not filtering for any specific field
            foreach (const KUrl &url, FileInfo::entryUrls(entry.data(), fileSourceModel()->bibliographyFile()->property(File::Url, QUrl()).toUrl(), FileInfo::TestExistenceYes)) {
                if (url.isLocalFile() && url.fileName().endsWith(QLatin1String(".pdf"))) {
                    const QString text = FileInfo::pdfToText(url.pathOrUrl());
                    int i = 0;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                        eachTerm[i] |= (*itsl).isEmpty() ? true : text.contains(*itsl, Qt::CaseInsensitive);
                }
            }

        int i = 0;
        if (m_filterQuery.field.isEmpty())
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                eachTerm[i] |= (*itsl).isEmpty() ? true : entry->id().contains(*itsl, Qt::CaseInsensitive);
    } else {
        QSharedPointer<Macro> macro = rowElement.dynamicCast<Macro>();
        if (!macro.isNull()) {
            if (m_filterQuery.field.isEmpty()) {
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                    eachTerm[i] |= macro->value().containsPattern(*itsl, Qt::CaseInsensitive) || macro->key().contains(*itsl, Qt::CaseInsensitive);
            }

            if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
                static const QString label = QLatin1String("macro");
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                    eachTerm[i] = eachTerm[i] || label.contains(*itsl, Qt::CaseInsensitive);
            }
        } else {
            QSharedPointer<Comment> comment = rowElement.dynamicCast<Comment>();
            if (!comment.isNull()) {
                if (m_filterQuery.field.isEmpty()) {
                    int i = 0;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                        eachTerm[i] |= (*itsl).isEmpty() ? true : comment->text().contains(*itsl, Qt::CaseInsensitive);
                }

                if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
                    static const QString label = QLatin1String("comment");
                    int i = 0;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                        eachTerm[i] = eachTerm[i] || label.contains(*itsl, Qt::CaseInsensitive);
                }
            } else {
                QSharedPointer<Preamble> preamble = rowElement.dynamicCast<Preamble>();
                if (!preamble.isNull()) {
                    if (m_filterQuery.field.isEmpty()) {
                        int i = 0;
                        for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                            eachTerm[i] |= preamble->value().containsPattern(*itsl, Qt::CaseInsensitive);
                    }

                    if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
                        static const QString label = QLatin1String("preamble");
                        int i = 0;
                        for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                            eachTerm[i] = eachTerm[i] || label.contains(*itsl, Qt::CaseInsensitive);
                    }
                }
            }
        }
    }

    bool every = true, any = false;
    for (int i = m_filterQuery.terms.count() - 1; i >= 0; --i) {
        every &= eachTerm[i];
        any |= eachTerm[i];
    }
    delete[] eachTerm;

    if (m_filterQuery.combination == SortFilterFileModel::AnyTerm)
        return any;
    else
        return every;
}

void SortFilterFileModel::loadState()
{
    KConfigGroup configGroup(config, configGroupName);
    m_showComments = configGroup.readEntry(FileModel::keyShowComments, FileModel::defaultShowComments);
    m_showMacros = configGroup.readEntry(FileModel::keyShowMacros, FileModel::defaultShowMacros);
}


void FileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    static const int numTotalStars = 8;
    bool ok = false;
    double percent = index.data(FileModel::NumberRole).toDouble(&ok);
    if (ok) {
        const BibTeXFields *bibtexFields = BibTeXFields::self();
        const FieldDescription *fd = bibtexFields->at(index.column());
        if (fd->upperCamelCase.toLower() == Entry::ftStarRating)
            StarRating::paintStars(painter, KIconLoader::DefaultState, numTotalStars, percent, option.rect);
    }
}


const int FileModel::NumberRole = Qt::UserRole + 9581;
const int FileModel::SortRole = Qt::UserRole + 236; /// see also MDIWidget's SortRole

const QString FileModel::keyShowComments = QLatin1String("showComments");
const bool FileModel::defaultShowComments = true;
const QString FileModel::keyShowMacros = QLatin1String("showMacros");
const bool FileModel::defaultShowMacros = true;


FileModel::FileModel(QObject *parent)
        : QAbstractTableModel(parent), m_file(NULL)
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
        for (BibTeXFields::ConstIterator it = bf->constBegin(); it != bf->constEnd(); ++it) {
            const FieldDescription *fd = *it;
            /// Colors may have changed
            bool columnChanged = fd->upperCamelCase.toLower() == Entry::ftColor;
            /// Person name formatting may has changed
            columnChanged |= fd->upperCamelCase.toLower() == Entry::ftAuthor || fd->upperCamelCase.toLower() == Entry::ftEditor;
            columnChanged |= fd->upperCamelCaseAlt.toLower() == Entry::ftAuthor || fd->upperCamelCaseAlt.toLower() == Entry::ftEditor;
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
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    KConfigGroup configGroup(config, Preferences::groupColor);
    QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
    QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultcolorLabels);
    colorToLabel.clear();
    for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
        colorToLabel.insert(*itc, i18n((*itl).toUtf8().constData()));
    }
}

QVariant FileModel::entryData(const Entry *entry, const QString &raw, const QString &rawAlt, int role, bool followCrossRef) const
{
    if (raw == QLatin1String("^id")) // FIXME: Use constant here?
        return QVariant(entry->id());
    else if (raw == QLatin1String("^type")) { // FIXME: Use constant here?
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
            return QVariant(leftSqueezeText(text, 128));
        } else
            return QVariant(text);
    }

}

File *FileModel::bibliographyFile()
{
    return m_file;
}

void FileModel::setBibliographyFile(File *file)
{
    bool doReset = m_file != file;
    m_file = file;
    if (doReset) reset(); // TODO necessary here?
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

int FileModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_file != NULL ? m_file->count() : 0;
}

int FileModel::columnCount(const QModelIndex & /*parent*/) const
{
    return BibTeXFields::self()->count();
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    /// do not accept invalid indices
    if (!index.isValid())
        return QVariant();

    /// check backend storage (File object)
    if (m_file == NULL)
        return QVariant();

    /// for now, only display data (no editing or icons etc)
    if (role != NumberRole && role != SortRole && role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
        return QVariant();

    const BibTeXFields *bibtexFields = BibTeXFields::self();
    if (index.row() < m_file->count() && index.column() < bibtexFields->count()) {
        const FieldDescription *fd = bibtexFields->at(index.column());
        QString raw = fd->upperCamelCase;
        QString rawAlt = fd->upperCamelCaseAlt;
        QSharedPointer<Element> element = (*m_file)[index.row()];
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();

        /// if BibTeX entry has a "x-color" field, use that color to highlight row
        if (role == Qt::BackgroundRole) {
            /// Retrieve "color"
            QString colorName;
            if (entry.isNull() || (colorName = PlainTextValue::text(entry->value(Entry::ftColor))) == QLatin1String("#000000") || colorName.isEmpty())
                return QVariant();
            else {
                QColor color(colorName);
                /// Use slightly different colors for even and odd rows
                color.setAlphaF(index.row() % 2 == 0 ? 0.75 : 1.0);
                return QVariant(color);
            }
        } else if (role == Qt::ForegroundRole) {
            /// Retrieve "color"
            QString colorName;
            if (entry.isNull() || (colorName = PlainTextValue::text(entry->value(Entry::ftColor))) == QLatin1String("#000000") || colorName.isEmpty())
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
                if (raw == QLatin1String("^id"))
                    return QVariant(macro->key());
                else if (raw == QLatin1String("^type"))
                    return QVariant(i18n("Macro"));
                else if (raw == QLatin1String("Title")) {
                    const QString text = PlainTextValue::text(macro->value()).simplified();
                    return QVariant(text);
                } else
                    return QVariant();
            } else {
                QSharedPointer<Comment> comment = element.dynamicCast<Comment>();
                if (!comment.isNull()) {
                    if (raw == QLatin1String("^type"))
                        return QVariant(i18n("Comment"));
                    else if (raw == Entry::ftTitle) {
                        const QString text = comment->text().simplified();
                        return QVariant(text);
                    } else
                        return QVariant();
                } else {
                    QSharedPointer<Preamble> preamble = element.dynamicCast<Preamble>();
                    if (!preamble.isNull()) {
                        if (raw == QLatin1String("^type"))
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
    return bibtexFields->at(section)->label;
}

Qt::ItemFlags FileModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // FIXME: What about drag'n'drop?
}

bool FileModel::removeRow(int row, const QModelIndex &parent)
{
    if (row < 0 || m_file == NULL || row >= rowCount() || row >= m_file->count())
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
    if (m_file == NULL) return false;

    QList<int> internalRows = rows;
    qSort(internalRows.begin(), internalRows.end(), qGreater<int>());

    beginRemoveRows(QModelIndex(), internalRows.last(), internalRows.first());
    foreach (int row, internalRows) {
        if (row < 0 || row >= rowCount() || row >= m_file->count())
            return false;
        m_file->removeAt(row);
    }
    endRemoveRows();

    return true;
}

bool FileModel::insertRow(QSharedPointer<Element> element, int row, const QModelIndex &parent)
{
    if (m_file == NULL || row < 0 || row > rowCount() || parent != QModelIndex())
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
            static const QString pattern = QLatin1String("%1_%2");
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
                static const QString pattern = QLatin1String("%1_%2");
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
    if (m_file == NULL || row < 0 || row >= m_file->count()) return QSharedPointer<Element>();

    return (*m_file)[row];
}

int FileModel::row(QSharedPointer<Element> element) const
{
    if (m_file == NULL) return -1;
    return m_file->indexOf(element);
}

void FileModel::reset()
{
    QAbstractTableModel::reset();
}
