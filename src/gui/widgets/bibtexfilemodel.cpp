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
#include <QFile>
#include <QString>

#include <KLocale>
#include <KDebug>

#include <element.h>
#include <entry.h>
#include <macro.h>
#include <comment.h>
#include <preamble.h>

#include "bibtexfilemodel.h"

static const QRegExp curlyRegExp("[{}]+");

void SortFilterBibTeXFileModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);
    m_internalModel = dynamic_cast<BibTeXFileModel*>(model);
    m_bibtexFields = BibTeXFields::self();
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
    if (left.column() == right.column() && (m_bibtexFields->at(left.column()).upperCamelCase == QLatin1String("Author") || m_bibtexFields->at(left.column()).upperCamelCase == QLatin1String("Editor"))) {
        /// special sorting for authors or editors: check all names, compare last and then first names
        Entry *entryA = dynamic_cast<Entry*>(m_internalModel->element(left.row()));
        Entry *entryB = dynamic_cast<Entry*>(m_internalModel->element(right.row()));
        if (entryA == NULL || entryB == NULL)
            return QSortFilterProxyModel::lessThan(left, right);

        Value valueA = entryA->value(Entry::ftAuthor);
        Value valueB = entryB->value(Entry::ftAuthor);
        if (valueA.isEmpty() && m_bibtexFields->at(left.column()).upperCamelCaseAlt == ("editor"))
            valueA = entryA->value(Entry::ftEditor);
        if (valueB.isEmpty() && m_bibtexFields->at(right.column()).upperCamelCaseAlt == ("editor"))
            valueB = entryB->value(Entry::ftEditor);

        if (valueA.isEmpty() || valueB.isEmpty()) return QSortFilterProxyModel::lessThan(left, right);

        for (Value::Iterator itA = valueA.begin(), itB = valueB.begin(); itA != valueA.end() &&  itB != valueB.end(); ++itA, ++itB) {
            Person *personA = dynamic_cast<Person *>(*itA);
            Person *personB = dynamic_cast<Person *>(*itB);
            if (personA == NULL || personB == NULL) return QSortFilterProxyModel::lessThan(left, right);

            QString nameA = personA->lastName().replace(curlyRegExp, "");
            QString nameB = personB->lastName().replace(curlyRegExp, "");
            int cmp = QString::compare(nameA, nameB, Qt::CaseInsensitive);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            nameA = personA->firstName().replace(curlyRegExp, "");
            nameB = personB->firstName().replace(curlyRegExp, "");
            cmp = QString::compare(nameA, nameB, Qt::CaseInsensitive);
            if (cmp < 0) return true;
            if (cmp > 0) return false;
        }

        return QSortFilterProxyModel::lessThan(left, right);
    } else
        return QSortFilterProxyModel::lessThan(left, right);
}

bool SortFilterBibTeXFileModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    Q_UNUSED(source_parent)

    if (m_filterQuery.terms.isEmpty()) return true; /// empty filter query

    Element *rowElement = m_internalModel->element(source_row);
    Q_ASSERT(rowElement != NULL);

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



const QRegExp BibTeXFileModel::whiteSpace = QRegExp("(\\s\\n\\r\\t)+");

BibTeXFileModel::BibTeXFileModel(QObject * parent)
        : QAbstractTableModel(parent), m_bibtexFile(NULL)
{
    m_bibtexFields = BibTeXFields::self();
// TODO
}

BibTeXFileModel::~BibTeXFileModel()
{
    if (m_bibtexFile != NULL) delete m_bibtexFile;
// TODO
}

File *BibTeXFileModel::bibTeXFile()
{
    if (m_bibtexFile == NULL) m_bibtexFile = new File();
    return m_bibtexFile;
}

void BibTeXFileModel::setBibTeXFile(File *bibtexFile)
{
    m_bibtexFile = bibtexFile;
    reset(); // TODO necessary here?
}

/*
QModelIndex BibTeXFileModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column, (void*)NULL); // parent == QModelIndex() ? createIndex(row, column, (void*)NULL) : QModelIndex();
}
*/

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
    return m_bibtexFields->count();
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
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return QVariant();

    if (index.row() < m_bibtexFile->count() && index.column() < m_bibtexFields->count()) {
        QString raw = m_bibtexFields->at(index.column()).upperCamelCase;
        QString rawAlt = m_bibtexFields->at(index.column()).upperCamelCaseAlt;
        Element* element = (*m_bibtexFile)[index.row()];
        Entry* entry = dynamic_cast<Entry*>(element);
        if (entry != NULL) {
            if (raw == "^id") // FIXME: Use constant here?
                return QVariant(entry->id());
            else if (raw == "^type")
                return QVariant(entry->type());
            else {
                if (entry->contains(raw)) {
                    QString text = PlainTextValue::text(entry->value(raw), m_bibtexFile);
                    text = text.replace(whiteSpace, " ");
                    return QVariant(text);
                } else if (!rawAlt.isNull() && entry->contains(rawAlt)) {
                    QString text = PlainTextValue::text(entry->value(rawAlt), m_bibtexFile);
                    text = text.replace(whiteSpace, " ");
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
                    QString text = PlainTextValue::text(macro->value(), m_bibtexFile);
                    text = text.replace(whiteSpace, " ");
                    return QVariant(text);
                } else
                    return QVariant();
            } else {
                Comment* comment = dynamic_cast<Comment*>(element);
                if (comment != NULL) {
                    if (raw == "^type")
                        return QVariant(i18n("Comment"));
                    else if (raw == Entry::ftTitle) {
                        QString text = comment->text().replace(QRegExp("[\\s\\n\\r\\t]+"), " ");
                        return QVariant(text);
                    } else
                        return QVariant();
                } else {
                    Preamble* preamble = dynamic_cast<Preamble*>(element);
                    if (preamble != NULL) {
                        if (raw == "^type")
                            return QVariant(i18n("Preamble"));
                        else if (raw == Entry::ftTitle) {
                            QString text = PlainTextValue::text(preamble->value(), m_bibtexFile);
                            text = text.replace(QRegExp("[\\s\\n\\r\\t]+"), " ");
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
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= m_bibtexFields->count())
        return QVariant();

    return m_bibtexFields->at(section).label;
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
