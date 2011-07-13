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

#include <QColor>
#include <QFile>
#include <QString>

#include <KLocale>
#include <KDebug>
#include <KConfigGroup>

#include <element.h>
#include <entry.h>
#include <macro.h>
#include <comment.h>
#include <preamble.h>
#include <bibtexentries.h>
#include <preferences.h>

#include "bibtexfilemodel.h"

static const QRegExp curlyRegExp("[{}]+");

const QString SortFilterBibTeXFileModel::configGroupName = QLatin1String("User Interface");

SortFilterBibTeXFileModel::SortFilterBibTeXFileModel(QObject * parent)
        : QSortFilterProxyModel(parent), m_internalModel(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")))
{
    loadState();
};

void SortFilterBibTeXFileModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);
    m_internalModel = dynamic_cast<BibTeXFileModel*>(model);
}

BibTeXFileModel *SortFilterBibTeXFileModel::bibTeXSourceModel()
{
    return m_internalModel;
}

void SortFilterBibTeXFileModel::updateFilter(SortFilterBibTeXFileModel::FilterQuery filterQuery)
{
    m_filterQuery = filterQuery;
    m_filterQuery.field = filterQuery.field.toLower(); /// required for comparison in filter code
    invalidateFilter();
}

bool SortFilterBibTeXFileModel::lessThan(const QModelIndex & left, const QModelIndex & right) const
{
    int column = left.column();
    Q_ASSERT(left.column() == right.column()); ///< assume that we only sort by column

    BibTeXFields *bibtexFields = BibTeXFields::self();
    const FieldDescription &fd = bibtexFields->at(column);

    if (column == right.column() && (fd.upperCamelCase == QLatin1String("Author") || fd.upperCamelCase == QLatin1String("Editor"))) {
        /// special sorting for authors or editors: check all names,
        /// compare last and then first names

        /// first, check if two entries (and not e.g. comments) are to be compared
        Entry *entryA = dynamic_cast<Entry*>(m_internalModel->element(left.row()));
        Entry *entryB = dynamic_cast<Entry*>(m_internalModel->element(right.row()));
        if (entryA == NULL || entryB == NULL)
            return QSortFilterProxyModel::lessThan(left, right);

        /// retrieve values of both cells
        Value valueA = entryA->value(fd.upperCamelCase);
        Value valueB = entryB->value(fd.upperCamelCase);
        if (valueA.isEmpty())
            valueA = entryA->value(fd.upperCamelCaseAlt);
        if (valueB.isEmpty())
            valueB = entryB->value(fd.upperCamelCaseAlt);

        /// if either value is empty, use default implementation
        if (valueA.isEmpty() || valueB.isEmpty())
            return QSortFilterProxyModel::lessThan(left, right);

        /// compare each person in both values
        for (Value::Iterator itA = valueA.begin(), itB = valueB.begin(); itA != valueA.end() &&  itB != valueB.end(); ++itA, ++itB) {
            Person *personA = dynamic_cast<Person *>(*itA);
            Person *personB = dynamic_cast<Person *>(*itB);
            /// not a Person object in value? fall back to default implementation
            if (personA == NULL || personB == NULL) return QSortFilterProxyModel::lessThan(left, right);

            /// get both values' next persons' last names for comparison
            QString nameA = personA->lastName().replace(curlyRegExp, "");
            QString nameB = personB->lastName().replace(curlyRegExp, "");
            int cmp = QString::compare(nameA, nameB, Qt::CaseInsensitive);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            /// if last names were inconclusive ...
            /// get both values' next persons' first names for comparison
            nameA = personA->firstName().replace(curlyRegExp, "");
            nameB = personB->firstName().replace(curlyRegExp, "");
            cmp = QString::compare(nameA, nameB, Qt::CaseInsensitive);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            // TODO Check for suffix and prefix?
        }

        /// comparison by names did not work (was not conclusive)
        /// fall back to default implementation
        return QSortFilterProxyModel::lessThan(left, right);
    } else {
        /// everything else can be sorted by default implementation
        /// (i.e. alphabetically or lexicographically)
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

bool SortFilterBibTeXFileModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    Q_UNUSED(source_parent)

    Element *rowElement = m_internalModel->element(source_row);
    Q_ASSERT(rowElement != NULL);

    /// check if showing comments is disabled
    if (!m_showComments && typeid(*rowElement) == typeid(Comment))
        return false;
    /// check if showing macros is disabled
    if (!m_showMacros && typeid(*rowElement) == typeid(Macro))
        return false;

    if (m_filterQuery.terms.isEmpty()) return true; /// empty filter query

    Entry *entry = dynamic_cast<Entry*>(rowElement);
    if (entry != NULL) {
        /// if current row contains an Entry ...

        bool any = false;
        bool *all = new bool[m_filterQuery.terms.count()];
        for (int i = m_filterQuery.terms.count() - 1; i >= 0; --i)
            all[i] = false;

        for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
            if (m_filterQuery.field.isEmpty() || m_filterQuery.field == it.key().toLower()) {
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i) {
                    bool contains = it.value().containsPattern(*itsl);
                    any |= contains;
                    all[i] |= contains;
                }
            }

        int i = 0;
        if (m_filterQuery.field.isEmpty())
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i) {
                bool contains = entry->id().contains(*itsl);
                any |= contains;
                all[i] |= contains;
            }

        bool every = true;
        for (i = m_filterQuery.terms.count() - 1; i >= 0; --i) every &= all[i];
        delete[] all;

        if (m_filterQuery.combination == SortFilterBibTeXFileModel::AnyTerm)
            return any;
        else
            return every;
    } else {
        Macro *macro = dynamic_cast<Macro*>(rowElement);
        if (macro != NULL) {
            bool all = true;
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl) {
                bool contains = macro->value().containsPattern(*itsl) || macro->key().contains(*itsl, Qt::CaseInsensitive);
                if (m_filterQuery.combination == SortFilterBibTeXFileModel::AnyTerm && contains)
                    return true;
                all &= contains;
            }
            return all;
        } else {
            Comment *comment = dynamic_cast<Comment*>(rowElement);
            if (comment != NULL) {
                bool all = true;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl) {
                    bool contains = comment->text().contains(*itsl, Qt::CaseInsensitive);
                    if (m_filterQuery.combination == SortFilterBibTeXFileModel::AnyTerm && contains)
                        return true;
                    all &= contains;
                }
                return all;
            } else {
                Preamble *preamble = dynamic_cast<Preamble*>(rowElement);
                if (preamble != NULL) {
                    bool all = true;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl) {
                        bool contains = preamble->value().containsPattern(*itsl);
                        if (m_filterQuery.combination == SortFilterBibTeXFileModel::AnyTerm && contains)
                            return true;
                        all &= contains;
                    }
                    return all;
                }
            }
        }
    }

    return false;
}

void SortFilterBibTeXFileModel::loadState()
{
    KConfigGroup configGroup(config, configGroupName);
    m_showComments = configGroup.readEntry(BibTeXFileModel::keyShowComments, BibTeXFileModel::defaultShowComments);
    m_showMacros = configGroup.readEntry(BibTeXFileModel::keyShowMacros, BibTeXFileModel::defaultShowMacros);
}


const QString BibTeXFileModel::keyShowComments = QLatin1String("showComments");
const bool BibTeXFileModel::defaultShowComments = true;
const QString BibTeXFileModel::keyShowMacros = QLatin1String("showMacros");
const bool BibTeXFileModel::defaultShowMacros = true;


BibTeXFileModel::BibTeXFileModel(QObject * parent)
        : QAbstractTableModel(parent), m_bibtexFile(NULL)
{
    /// load mapping from color value to label
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    KConfigGroup configGroup(config, Preferences::groupColor);
    QStringList colorCodes = configGroup.readEntry(Preferences::keyColorCodes, Preferences::defaultColorCodes);
    QStringList colorLabels = configGroup.readEntry(Preferences::keyColorLabels, Preferences::defaultcolorLabels);
    for (QStringList::ConstIterator itc = colorCodes.constBegin(), itl = colorLabels.constBegin(); itc != colorCodes.constEnd() && itl != colorLabels.constEnd(); ++itc, ++itl) {
        colorToLabel.insert(*itc, *itl);
    }
}

BibTeXFileModel::~BibTeXFileModel()
{
    if (m_bibtexFile != NULL) delete m_bibtexFile;
}

File *BibTeXFileModel::bibTeXFile()
{
    if (m_bibtexFile == NULL) m_bibtexFile = new File();
    return m_bibtexFile;
}

void BibTeXFileModel::setBibTeXFile(File *bibtexFile)
{
    // FIXME delete old m_bibtexFile before overwriting it?
    m_bibtexFile = bibtexFile;
    reset(); // TODO necessary here?
}

QModelIndex BibTeXFileModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool BibTeXFileModel::hasChildren(const QModelIndex & parent) const
{
    return parent == QModelIndex();
}

int BibTeXFileModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_bibtexFile != NULL ? m_bibtexFile->count() : 0;
}

int BibTeXFileModel::columnCount(const QModelIndex & /*parent*/) const
{
    return BibTeXFields::self()->count();
}

QVariant BibTeXFileModel::data(const QModelIndex &index, int role) const
{
    /// do not accept invalid indices
    if (!index.isValid())
        return QVariant();

    /// check backend storage (File object)
    if (m_bibtexFile == NULL)
        return QVariant();

    /// for now, only display data (no editing or icons etc)
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole)
        return QVariant();

    BibTeXFields *bibtexFields = BibTeXFields::self();
    if (index.row() < m_bibtexFile->count() && index.column() < bibtexFields->count()) {
        const FieldDescription &fd = bibtexFields->at(index.column());
        QString raw = fd.upperCamelCase;
        QString rawAlt = fd.upperCamelCaseAlt;
        Element* element = (*m_bibtexFile)[index.row()];
        Entry* entry = dynamic_cast<Entry*>(element);

        /// if BibTeX entry has a "x-color" field, use that color to highlight row
        if (role == Qt::BackgroundRole) {
            QString colorName;
            if (entry == NULL || (colorName = PlainTextValue::text(entry->value("x-color"), m_bibtexFile)) == "#000000" || colorName.isEmpty())
                return QVariant();
            else {
                QColor color(colorName);
                color.setAlphaF(0.75);
                return QVariant(color);
            }
        }

        if (entry != NULL) {
            if (raw == "^id") // FIXME: Use constant here?
                return QVariant(entry->id());
            else if (raw == "^type") { // FIXME: Use constant here?
                /// try to beautify type, e.g. translate "proceedings" into
                /// "Conference or Workshop Proceedings"
                QString label = BibTeXEntries::self()->label(entry->type());
                if (label.isEmpty()) {
                    /// fall-back to entry type as it is
                    return QVariant(entry->type());
                } else
                    return QVariant(label);
            } else if (raw.toLower() == Entry::ftColor) {
                QString text = PlainTextValue::text(entry->value(raw), m_bibtexFile);
                if (text.isEmpty()) return QVariant();
                QString colorText = colorToLabel[text];
                if (colorText.isEmpty()) return QVariant(text);
                return QVariant(colorText);
            } else {
                if (entry->contains(raw)) {
                    const QString text = PlainTextValue::text(entry->value(raw), m_bibtexFile).simplified();
                    return QVariant(text);
                } else if (!rawAlt.isNull() && entry->contains(rawAlt)) {
                    const QString text = PlainTextValue::text(entry->value(rawAlt), m_bibtexFile).simplified();
                    return QVariant(text);
                } else
                    return QVariant();
            }
        } else {
            Macro* macro = dynamic_cast<Macro*>(element);
            if (macro != NULL) {
                if (raw == "^id")
                    return QVariant(macro->key());
                else if (raw == "^type")
                    return QVariant(i18n("Macro"));
                else if (raw == "Title") {
                    const QString text = PlainTextValue::text(macro->value(), m_bibtexFile).simplified();
                    return QVariant(text);
                } else
                    return QVariant();
            } else {
                Comment* comment = dynamic_cast<Comment*>(element);
                if (comment != NULL) {
                    if (raw == "^type")
                        return QVariant(i18n("Comment"));
                    else if (raw == Entry::ftTitle) {
                        const QString text = comment->text().simplified();
                        return QVariant(text);
                    } else
                        return QVariant();
                } else {
                    Preamble* preamble = dynamic_cast<Preamble*>(element);
                    if (preamble != NULL) {
                        if (raw == "^type")
                            return QVariant(i18n("Preamble"));
                        else if (raw == Entry::ftTitle) {
                            const QString text = PlainTextValue::text(preamble->value(), m_bibtexFile).simplified();
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

QVariant BibTeXFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const BibTeXFields *bibtexFields = BibTeXFields::self();
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= bibtexFields->count())
        return QVariant();
    return bibtexFields->at(section).label;
}

Qt::ItemFlags BibTeXFileModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // FIXME: What about drag'n'drop?
}

bool BibTeXFileModel::removeRow(int row, const QModelIndex & parent)
{
    if (row < 0 || row >= rowCount() || row >= m_bibtexFile->count())
        return false;
    if (parent != QModelIndex())
        return false;

    m_bibtexFile->removeAt(row);

    reset();

    return true;
}

bool BibTeXFileModel::removeRowList(const QList<int> &rows)
{
    QList<int> internalRows = rows;
    qSort(internalRows.begin(), internalRows.end(), qGreater<int>());

    foreach(int row, internalRows) {
        if (row < 0 || row >= rowCount() || row >= m_bibtexFile->count())
            return false;
        m_bibtexFile->removeAt(row);
    }

    reset();

    return true;
}

bool BibTeXFileModel::insertRow(Element *element, int row, const QModelIndex & parent)
{
    if (row < 0 || row > rowCount())
        return false;
    if (parent != QModelIndex())
        return false;

    m_bibtexFile->insert(row, element);

    reset();

    return true;
}

Element* BibTeXFileModel::element(int row) const
{
    if (m_bibtexFile == NULL || row < 0 || row >= m_bibtexFile->count()) return NULL;

    return (*m_bibtexFile)[row];
}

int BibTeXFileModel::row(Element *element) const
{
    return m_bibtexFile->indexOf(element);
}
