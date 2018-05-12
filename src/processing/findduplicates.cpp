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

#include "findduplicates.h"

#include <typeinfo>

#include <QLinkedList>
#include <QProgressDialog>
#include <QApplication>
#include <QDate>
#include <QRegularExpression>

#include <KLocalizedString>

#include "file.h"
#include "models/filemodel.h"
#include "entry.h"

EntryClique::EntryClique()
{
    /// nothing
}

int EntryClique::entryCount() const
{
    return checkedEntries.count();
}

QList<QSharedPointer<Entry> > EntryClique::entryList() const
{
    return checkedEntries.keys();
}

bool EntryClique::isEntryChecked(QSharedPointer<Entry> entry) const
{
    return checkedEntries[entry];
}

void EntryClique::setEntryChecked(QSharedPointer<Entry> entry, bool isChecked)
{
    checkedEntries[entry] = isChecked;
    recalculateValueMap();
}

int EntryClique::fieldCount() const
{
    return valueMap.count();
}

QList<QString> EntryClique::fieldList() const
{
    return valueMap.keys();
}

QList<Value> EntryClique::values(const QString &field) const
{
    return valueMap[field];
}

QList<Value> &EntryClique::values(const QString &field)
{
    return valueMap[field];
}

Value EntryClique::chosenValue(const QString &field) const
{
    Q_ASSERT_X(chosenValueMap[field].count() == 1, "Value EntryClique::chosenValue(const QString &field) const", "Exactly one value expected in chosenValueMap");
    return chosenValueMap[field].first();
}

QList<Value> EntryClique::chosenValues(const QString &field) const
{
    return chosenValueMap[field];
}

void EntryClique::setChosenValue(const QString &field, Value &value, ValueOperation valueOperation)
{
    switch (valueOperation) {
    case SetValue: {
        chosenValueMap[field].clear();
        chosenValueMap[field] << value;
        break;
    }
    case AddValue: {
        QString text = PlainTextValue::text(value);
        for (const Value &value : const_cast<const QList<Value> &>(chosenValueMap[field]))
            if (PlainTextValue::text(value) == text)
                return;
        chosenValueMap[field] << value;
        break;
    }
    case RemoveValue: {
        QString text = PlainTextValue::text(value);
        for (QList<Value>::Iterator it = chosenValueMap[field].begin(); it != chosenValueMap[field].end(); ++it)
            if (PlainTextValue::text(*it) == text) {
                chosenValueMap[field].erase(it);
                return;
            }
        break;
    }
    }
}

void EntryClique::addEntry(QSharedPointer<Entry> entry)
{
    checkedEntries.insert(entry, false); /// remember to call recalculateValueMap later
}

void EntryClique::recalculateValueMap()
{
    valueMap.clear();
    chosenValueMap.clear();

    /// go through each and every entry ...
    const QList<QSharedPointer<Entry> > el = entryList();
    for (const auto &entry : el)
        if (isEntryChecked(entry)) {

            /// cover entry type
            Value v;
            v.append(QSharedPointer<VerbatimText>(new VerbatimText(entry->type())));
            insertKeyValueToValueMap(QStringLiteral("^type"), v, entry->type());

            /// cover entry id
            v.clear();
            v.append(QSharedPointer<VerbatimText>(new VerbatimText(entry->id())));
            insertKeyValueToValueMap(QStringLiteral("^id"), v, entry->id());

            /// go through each and every field of this entry
            for (Entry::ConstIterator fieldIt = entry->constBegin(); fieldIt != entry->constEnd(); ++fieldIt) {
                /// store both field name and value for later reference
                const QString fieldName = fieldIt.key().toLower();
                const Value fieldValue = fieldIt.value();

                if (fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl) {
                    for (const auto &vi : fieldValue) {
                        const QString text = PlainTextValue::text(*vi);
                        Value v;
                        v << vi;
                        insertKeyValueToValueMap(fieldName, v, text);
                    }
                } else {
                    const QString fieldValueText = PlainTextValue::text(fieldValue);
                    insertKeyValueToValueMap(fieldName, fieldValue, fieldValueText);
                }
            }
        }

    const QList<QString> fl = fieldList();
    for (const QString &fieldName : fl)
        if (valueMap[fieldName].count() < 2) {
            valueMap.remove(fieldName);
            chosenValueMap.remove(fieldName);
        }
}

void EntryClique::insertKeyValueToValueMap(const QString &fieldName, const Value &fieldValue, const QString &fieldValueText)
{
    if (fieldValueText.isEmpty()) return;

    if (valueMap.contains(fieldName)) {
        /// in the list of alternatives, search of a value identical
        /// to the current (as of fieldIt) value (to avoid duplicates)

        bool alreadyContained = false;
        QList<Value> alternatives = valueMap[fieldName];
        for (const Value &v : const_cast<const QList<Value> &>(alternatives))
            if (PlainTextValue::text(v) == fieldValueText) {
                alreadyContained = true;
                break;
            }

        if (!alreadyContained) {
            alternatives << fieldValue;
            valueMap[fieldName] = alternatives;
        }
    } else {
        QList<Value> alternatives = valueMap[fieldName];
        alternatives << fieldValue;
        valueMap.insert(fieldName, alternatives);
        QList<Value> chosen;
        chosen << fieldValue;
        chosenValueMap.insert(fieldName, chosen);
    }
}

class FindDuplicates::FindDuplicatesPrivate
{
private:
    const unsigned int maxDistance;
    int **d;
    static const int dsize;

public:
    int sensitivity;
    QWidget *widget;

    FindDuplicatesPrivate(int sens, QWidget *w)
            : maxDistance(10000), sensitivity(sens), widget(w == nullptr ? qApp->activeWindow() : w) {
        d = new int *[dsize];
        for (int i = 0; i < dsize; ++i)
            d[i] = new int[dsize];
    }

    ~FindDuplicatesPrivate() {
        for (int i = 0; i < dsize; ++i) delete[] d[i];
        delete [] d;
    }

    /**
      * Determine the Levenshtein distance between two words.
      * See also http://en.wikipedia.org/wiki/Levenshtein_distance
      * @param s first word, all chars already in lower case
      * @param t second word, all chars already in lower case
      * @return distance between both words
      */
    double levenshteinDistanceWord(const QString &s, const QString &t) {
        int m = qMin(s.length(), dsize - 1), n = qMin(t.length(), dsize - 1);
        if (m < 1 && n < 1) return 0.0;
        if (m < 1 || n < 1) return 1.0;

        for (int i = 0; i <= m; ++i)
            d[i][0] = i;

        for (int i = 0; i <= n; ++i) d[0][i] = i;

        for (int i = 1; i <= m; ++i)
            for (int j = 1; j <= n; ++j) {
                d[i][j] = d[i - 1][j] + 1;
                int c = d[i][j - 1] + 1;
                if (c < d[i][j]) d[i][j] = c;
                c = d[i - 1][j - 1] + (s[i - 1] == t[j - 1] ? 0 : 1);
                if (c < d[i][j]) d[i][j] = c;
            }

        double result = d[m][n];

        result = result / (double)qMax(m, n);
        result *= result;
        return result;
    }

    /**
     * Determine the Levenshtein distance between two sentences (list of words).
     * See also http://en.wikipedia.org/wiki/Levenshtein_distance
     * @param s first sentence
     * @param t second sentence
     * @return distance between both sentences
     */
    double levenshteinDistance(const QStringList &s, const QStringList &t) {
        int m = s.size(), n = t.size();
        if (m < 1 && n < 1) return 0.0;
        if (m < 1 || n < 1) return 1.0;

        double **d = new double*[m + 1];
        for (int i = 0; i <= m; ++i) {
            d[i] = new double[n + 1]; d[i][0] = i;
        }
        for (int i = 0; i <= n; ++i) d[0][i] = i;

        for (int i = 1; i <= m; ++i)
            for (int j = 1; j <= n; ++j) {
                d[i][j] = d[i - 1][j] + 1;
                double c = d[i][j - 1] + 1;
                if (c < d[i][j]) d[i][j] = c;
                c = d[i - 1][j - 1] + levenshteinDistanceWord(s[i - 1], t[j - 1]);
                if (c < d[i][j]) d[i][j] = c;
            }

        double result = d[m][n];
        for (int i = 0; i <= m; ++i) delete[] d[i];
        delete [] d;

        result = result / (double)qMax(m, n);

        return result;
    }


    /**
     * Determine the Levenshtein distance between two sentences,
     * where each sentence is in a string (not split into single words).
     * See also http://en.wikipedia.org/wiki/Levenshtein_distance
     * @param s first sentence
     * @param t second sentence
     * @return distance between both sentences
     */
    double levenshteinDistance(const QString &s, const QString &t) {
        static const QRegularExpression nonWordRegExp(QStringLiteral("[^a-z']+"), QRegularExpression::CaseInsensitiveOption);
        if (s.isEmpty() || t.isEmpty()) return 1.0;
        return levenshteinDistance(s.toLower().split(nonWordRegExp, QString::SkipEmptyParts), t.toLower().split(nonWordRegExp, QString::SkipEmptyParts));
    }

    /**
     * Distance between two BibTeX entries, scaled by maxDistance.
     */
    int entryDistance(Entry *entryA, Entry *entryB) {
        /// "distance" to be used if no value for a field is given
        const double neutralDistance = 0.05;

        /**
         * Get both entries' titles. If both are empty, use a "neutral
         * distance" otherwise compute levenshtein distance (0.0 .. 1.0).
         */
        const QString titleA = PlainTextValue::text(entryA->value(Entry::ftTitle));
        const QString titleB = PlainTextValue::text(entryB->value(Entry::ftTitle));
        double titleDistance = titleA.isEmpty() && titleB.isEmpty() ? neutralDistance : levenshteinDistance(titleA, titleB);

        /**
         * Get both entries' author names. If both are empty, use a
         * "neutral distance" otherwise compute levenshtein distance
         * (0.0 .. 1.0).
         */
        const QString authorA = PlainTextValue::text(entryA->value(Entry::ftAuthor));
        const QString authorB = PlainTextValue::text(entryB->value(Entry::ftAuthor));
        double authorDistance = authorA.isEmpty() && authorB.isEmpty() ? neutralDistance : levenshteinDistance(authorA, authorB);

        /**
         * Get both entries' years. If both are empty, use a
         * "neutral distance" otherwise compute distance as follows:
         * take square of difference between both years, but impose
         * a maximum of 100. Divide value by 100.0 to get a distance
         * value of 0.0 .. 1.0.
         */
        const QString yearA = PlainTextValue::text(entryA->value(Entry::ftYear));
        const QString yearB = PlainTextValue::text(entryB->value(Entry::ftYear));
        bool yearAok = false, yearBok = false;
        int yearAint = yearA.toInt(&yearAok);
        int yearBint = yearB.toInt(&yearBok);
        double yearDistance = yearAok && yearBok ? qMin((yearBint - yearAint) * (yearBint - yearAint), 100) / 100.0 : neutralDistance;

        /**
         * Compute total distance by taking individual distances for
         * author, title, and year. Weight each individual distance as
         * follows: title => 60%, author => 30%, year => 10%
         * Scale distance by maximum distance and round to int; result
         * will be in range 0 .. maxDistance.
         */
        int distance = (unsigned int)(maxDistance * (titleDistance * 0.6 + authorDistance * 0.3 + yearDistance * 0.1) + 0.5);

        return distance;
    }

};

const int FindDuplicates::FindDuplicatesPrivate::dsize = 32;


FindDuplicates::FindDuplicates(QWidget *parent, int sensitivity)
        : QObject(parent), d(new FindDuplicatesPrivate(sensitivity, parent))
{
    /// nothing
}

FindDuplicates::~FindDuplicates()
{
    delete d;
}

bool FindDuplicates::findDuplicateEntries(File *file, QList<EntryClique *> &entryCliqueList)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QScopedPointer<QProgressDialog> progressDlg(new QProgressDialog(i18n("Searching ..."), i18n("Cancel"), 0, 100000 /* to be set later to actual value */, d->widget));
    progressDlg->setModal(true);
    progressDlg->setWindowTitle(i18n("Finding Duplicates"));
    progressDlg->setMinimumWidth(d->widget->fontMetrics().averageCharWidth() * 48);
    progressDlg->setAutoReset(false);
    entryCliqueList.clear();

    /// assemble list of entries only (ignoring comments, macros, ...)
    QList<QSharedPointer<Entry> > listOfEntries;
    listOfEntries.reserve(file->size());
    for (const auto &element : const_cast<const File &>(*file)) {
        QSharedPointer<Entry> e = element.dynamicCast<Entry>();
        if (!e.isNull() && !e->isEmpty())
            listOfEntries << e;
    }

    if (listOfEntries.isEmpty()) {
        /// no entries to compare found
        entryCliqueList.clear();
        QApplication::restoreOverrideCursor();
        return progressDlg->wasCanceled();
    }

    int curProgress = 0, maxProgress = listOfEntries.count() * (listOfEntries.count() - 1) / 2;
    int progressDelta = 1;

    progressDlg->setMaximum(maxProgress);
    progressDlg->show();

    emit maximumProgress(maxProgress);

    /// go through all entries ...
    for (const auto &entry : const_cast<const QList<QSharedPointer<Entry> > &>(listOfEntries)) {
        QApplication::instance()->processEvents();
        if (progressDlg->wasCanceled()) {
            entryCliqueList.clear();
            break;
        }

        progressDlg->setValue(curProgress);
        emit currentProgress(curProgress);
        /// ... and find a "clique" of entries where it will match, i.e. distance is below sensitivity

        /// assume current entry will match in no clique
        bool foundClique = false;

        /// go through all existing cliques
        for (QList<EntryClique *>::Iterator cit = entryCliqueList.begin(); cit != entryCliqueList.end(); ++cit) {
            /// check distance between current entry and clique's first entry
            if (d->entryDistance(entry.data(), (*cit)->entryList().first().data()) < d->sensitivity) {
                /// if distance is below sensitivity, add current entry to clique
                foundClique = true;
                (*cit)->addEntry(entry);
                break;
            }

            QApplication::instance()->processEvents();
            if (progressDlg->wasCanceled()) {
                entryCliqueList.clear();
                break;
            }
        }

        if (!progressDlg->wasCanceled() && !foundClique) {
            /// no clique matched to current entry, so create and add new clique
            /// consisting only of the current entry
            EntryClique *newClique = new EntryClique();
            newClique->addEntry(entry);
            entryCliqueList << newClique;
        }

        curProgress += progressDelta;
        ++progressDelta;
        progressDlg->setValue(curProgress);

        emit currentProgress(curProgress);
    }

    progressDlg->setValue(progressDlg->maximum());

    /// remove cliques with only one element (nothing to merge here) from the list of cliques
    for (QList<EntryClique *>::Iterator cit = entryCliqueList.begin(); cit != entryCliqueList.end();)
        if ((*cit)->entryCount() < 2) {
            EntryClique *ec = *cit;
            cit = entryCliqueList.erase(cit);
            delete ec;
        } else {
            /// entries have been inserted as checked,
            /// therefore recalculate alternatives
            (*cit)->recalculateValueMap();

            ++cit;
        }

    QApplication::restoreOverrideCursor();
    return progressDlg->wasCanceled();
}


MergeDuplicates::MergeDuplicates()
{
    /// nothing
}

bool MergeDuplicates::mergeDuplicateEntries(const QList<EntryClique *> &entryCliques, FileModel *fileModel)
{
    bool didMerge = false;

    for (EntryClique *entryClique : entryCliques) {
        /// Avoid adding fields 20 lines below
        /// which have been remove (not added) 10 lines below
        QSet<QString> coveredFields;

        Entry *mergedEntry = new Entry(QString(), QString());
        const auto fieldList = entryClique->fieldList();
        coveredFields.reserve(fieldList.size());
        for (const auto &field : fieldList) {
            coveredFields << field;
            if (field == QStringLiteral("^id"))
                mergedEntry->setId(PlainTextValue::text(entryClique->chosenValue(field)));
            else if (field == QStringLiteral("^type"))
                mergedEntry->setType(PlainTextValue::text(entryClique->chosenValue(field)));
            else {
                Value combined;
                const auto chosenValues = entryClique->chosenValues(field);
                for (const Value &v : chosenValues) {
                    combined.append(v);
                }
                if (!combined.isEmpty())
                    mergedEntry->insert(field, combined);
            }
        }

        bool actuallyMerged = false;
        int preferredInsertionRow = -1;
        const auto entryList = entryClique->entryList();
        for (const auto &entry : entryList) {
            /// if merging entries with identical ids, the merged entry will not yet have an id (is null)
            if (mergedEntry->id().isEmpty())
                mergedEntry->setId(entry->id());
            /// if merging entries with identical types, the merged entry will not yet have an type (is null)
            if (mergedEntry->type().isEmpty())
                mergedEntry->setType(entry->type());

            /// add all other fields not covered by user selection
            /// those fields did only occur in one entry (no conflict)
            /// may add a lot of bloat to merged entry
            if (entryClique->isEntryChecked(entry)) {
                actuallyMerged = true;
                for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
                    if (!mergedEntry->contains(it.key()) && !coveredFields.contains(it.key())) {
                        mergedEntry->insert(it.key(), it.value());
                        coveredFields << it.key();
                    }
                const int row = fileModel->row(entry);
                if (preferredInsertionRow < 0) preferredInsertionRow = row;
                fileModel->removeRow(row);
            }
        }

        if (actuallyMerged) {
            if (preferredInsertionRow < 0) preferredInsertionRow = fileModel->rowCount();
            fileModel->insertRow(QSharedPointer<Entry>(mergedEntry), preferredInsertionRow);
        } else
            delete mergedEntry;
        didMerge |= actuallyMerged;
    }

    return didMerge;
}
