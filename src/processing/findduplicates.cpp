/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <KDebug>
#include <KProgressDialog>
#include <KLocale>
#include <KApplication>

#include "file.h"
#include "entry.h"
#include "findduplicates.h"

#define getText(entry, fieldname) PlainTextValue::text((entry)->value((fieldname)))

EntryClique::EntryClique()
{
    // nothing
}

int EntryClique::entryCount() const
{
    return checkedEntries.count();
}

QList<Entry*> EntryClique::entryList() const
{
    return checkedEntries.keys();
}

bool EntryClique::isEntryChecked(Entry *entry) const
{
    return checkedEntries[entry];
}

void EntryClique::setEntryChecked(Entry *entry, bool isChecked)
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

Value EntryClique::chosenValue(const QString &field) const
{
    Q_ASSERT(chosenValueMap[field].count() == 1);
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
        foreach(Value value, chosenValueMap[field])
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

void EntryClique::addEntry(Entry* entry)
{
    checkedEntries.insert(entry, false); /// remember to call recalculateValueMap later
}

void EntryClique::recalculateValueMap()
{
    valueMap.clear();
    chosenValueMap.clear();

    /// go through each and every entry ...
    const QList<Entry*> el = entryList();
    foreach(Entry *entry, el)
    if (isEntryChecked(entry)) {

        /// cover entry type
        Value v;
        v.append(new VerbatimText(entry->type()));
        insertKeyValueToValueMap(QLatin1String("^type"), v, entry->type());

        /// cover entry id
        v.clear();
        v.append(new VerbatimText(entry->id()));
        insertKeyValueToValueMap(QLatin1String("^id"), v, entry->id());

        /// go through each and every field of this entry
        for (Entry::ConstIterator fieldIt = entry->constBegin(); fieldIt != entry->constEnd(); ++fieldIt) {
            /// store both field name and value for later reference
            const QString fieldName = fieldIt.key().toLower();
            const Value fieldValue = fieldIt.value();

            if (fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl) {
                foreach(ValueItem* vi, fieldValue) {
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

    QList<QString> fl = fieldList();
    foreach(QString fieldName, fl)
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
        foreach(Value v, alternatives)
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
    FindDuplicates *p;
    const int maxDistance;

public:
    bool gotCanceled;
    int sensitivity;
    QWidget *widget;

    FindDuplicatesPrivate(FindDuplicates *parent, int sens, QWidget *w)
            : p(parent), maxDistance(10000), gotCanceled(false), sensitivity(sens), widget(w) {
        // nothing
    }

    /**
      * Determine the Levenshtein distance between two words.
      * See also http://en.wikipedia.org/wiki/Levenshtein_distance
      * @param s first word
      * @param t second word
      * @return distance between both words
      */
    double levenshteinDistanceWord(const QString &s, const QString &t) {
        QString mys = s.toLower(), myt = t.toLower();
        int m = s.length(), n = t.length();
        if (m < 1 && n < 1) return 0.0;
        if (m < 1 || n < 1) return 1.0;

        int **d = new int*[m+1];
        for (int i = 0; i <= m; ++i) {
            d[i] = new int[n+1]; d[i][0] = i;
        }
        for (int i = 0; i <= n; ++i) d[0][i] = i;

        for (int i = 1; i <= m;++i)
            for (int j = 1; j <= n;++j) {
                d[i][j] = d[i-1][j] + 1;
                int c = d[i][j-1] + 1;
                if (c < d[i][j]) d[i][j] = c;
                c = d[i-1][j-1] + (mys[i-1] == myt[j-1] ? 0 : 1);
                if (c < d[i][j]) d[i][j] = c;
            }

        double result = d[m][n];
        for (int i = 0; i <= m; ++i) delete[] d[i];
        delete [] d;

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

        double **d = new double*[m+1];
        for (int i = 0; i <= m; ++i) {
            d[i] = new double[n+1]; d[i][0] = i;
        }
        for (int i = 0; i <= n; ++i) d[0][i] = i;

        for (int i = 1; i <= m;++i)
            for (int j = 1; j <= n;++j) {
                d[i][j] = d[i-1][j] + 1;
                double c = d[i][j-1] + 1;
                if (c < d[i][j]) d[i][j] = c;
                c = d[i-1][j-1] + levenshteinDistanceWord(s[i-1], t[j-1]);
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
        const QRegExp nonWordRegExp("[^a-z']+", Qt::CaseInsensitive);
        if (s.isEmpty() || t.isEmpty()) return 1.0;
        return levenshteinDistance(s.split(nonWordRegExp, QString::SkipEmptyParts), t.split(nonWordRegExp, QString::SkipEmptyParts));
    }

    /**
     * Distance between two BibTeX entries, scaled by maxDistance.
     */
    int entryDistance(Entry *entryA, Entry *entryB) {
        double titleValue = levenshteinDistance(getText(entryA, Entry::ftTitle), getText(entryB, Entry::ftTitle));
        double authorValue = levenshteinDistance(getText(entryA, Entry::ftAuthor), getText(entryB, Entry::ftAuthor));
        bool ok1 = false, ok2 = false;
        double yearValue = QString(getText(entryA, Entry::ftYear)).toInt(&ok1) - QString(getText(entryB, Entry::ftYear)).toInt(&ok2);
        yearValue = ok1 && ok2 ? qMin(1.0, yearValue * yearValue / 100.0) : 100.0;
        int distance = (unsigned int)(maxDistance * (titleValue * 0.6 + authorValue * 0.3 + yearValue * 0.1));

        return distance;
    }

};


FindDuplicates::FindDuplicates(QWidget *parent, int sensitivity)
        : d(new FindDuplicatesPrivate(this, sensitivity, parent))
{
    // nothing
}

bool FindDuplicates::findDuplicateEntries(File *file, QList<EntryClique*> &entryCliqueList)
{
    KApplication::setOverrideCursor(Qt::WaitCursor);
    KProgressDialog *progressDlg = new KProgressDialog(d->widget, i18n("Finding Duplicates"));
    progressDlg->setModal(true);
    progressDlg->setLabelText(i18n("Searching ..."));
    progressDlg->setMinimumWidth(d->widget->fontMetrics().averageCharWidth()*48);
    progressDlg->setAllowCancel(true);
    connect(progressDlg, SIGNAL(cancelClicked()), this, SLOT(gotCanceled()));

    entryCliqueList.clear();
    d->gotCanceled = false;

    /// assemble list of entries only (ignoring comments, macros, ...)
    QList<Entry*> listOfEntries;
    for (File::ConstIterator it = file->constBegin(); it != file->constEnd(); ++it) {
        Entry *e = dynamic_cast<Entry*>(*it);
        if (e != NULL && !e->isEmpty())
            listOfEntries << e;
    }

    if (listOfEntries.isEmpty()) {
        /// no entries to compare found
        entryCliqueList.clear();
        progressDlg->deleteLater();
        KApplication::restoreOverrideCursor();
        return d->gotCanceled;
    }

    int curProgress = 0, maxProgress = listOfEntries.count() * (listOfEntries.count() - 1) / 2;
    int progressDelta = 1;

    progressDlg->progressBar()->setMaximum(maxProgress);
    progressDlg->show();
    progressDlg->setLabelText(i18n("Searching ..."));

    /// go through all entries ...
    for (QList<Entry*>::ConstIterator eit = listOfEntries.constBegin(); eit != listOfEntries.constEnd(); ++eit) {
        KApplication::instance()->processEvents();
        if (d->gotCanceled) {
            entryCliqueList.clear();
            break;
        }

        progressDlg->progressBar()->setValue(curProgress);
        /// ... and find a "clique" of entries where it will match, i.e. distance is below sensitivity

        /// assume current entry will match in no clique
        bool foundClique = false;

        /// go through all existing cliques
        for (QList<EntryClique*>::Iterator cit = entryCliqueList.begin(); cit != entryCliqueList.end(); ++cit) {
            /// check distance between current entry and clique's first entry
            if (d->entryDistance(*eit, (*cit)->entryList().first()) < d->sensitivity) {
                /// if distance is below sensitivity, add current entry to clique
                foundClique = true;
                (*cit)->addEntry(*eit);
                break;
            }

            KApplication::instance()->processEvents();
            if (d->gotCanceled) {
                entryCliqueList.clear();
                break;
            }
        }

        if (!d->gotCanceled && !foundClique) {
            /// no clique matched to current entry, so create and add new clique
            /// consisting only of the current entry
            EntryClique *newClique = new EntryClique();
            newClique->addEntry(*eit);
            entryCliqueList << newClique;
        }

        curProgress += progressDelta;
        ++progressDelta;
        progressDlg->progressBar()->setValue(curProgress);
    }

    progressDlg->progressBar()->setValue(progressDlg->progressBar()->maximum());
    progressDlg->close();

    /// remove cliques with only one element (nothing to merge here) from the list of cliques
    for (QList<EntryClique*>::Iterator cit = entryCliqueList.begin(); cit != entryCliqueList.end();)
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

    progressDlg->deleteLater();
    KApplication::restoreOverrideCursor();
    return d->gotCanceled;
}

void FindDuplicates::gotCanceled()
{
    d->gotCanceled = true;
}


class MergeDuplicates::MergeDuplicatesPrivate
{
private:
    MergeDuplicates *p;

public:
    QWidget *widget;

    MergeDuplicatesPrivate(MergeDuplicates *parent, QWidget *w)
            : p(parent), widget(w) {
        // nothing
    }
};

MergeDuplicates::MergeDuplicates(QWidget *parent)
        : d(new MergeDuplicatesPrivate(this, parent))
{
    // TODO
}

bool MergeDuplicates::mergeDuplicateEntries(const QList<EntryClique*> &entryCliques, File *file)
{
    foreach(EntryClique *entryClique, entryCliques) {
        Entry *mergedEntry = new Entry(QString::null, QString::null);
        foreach(QString field, entryClique->fieldList()) {
            if (field == QLatin1String("^id"))
                mergedEntry->setId(PlainTextValue::text(entryClique->chosenValue(field)));
            else if (field == QLatin1String("^type"))
                mergedEntry->setType(PlainTextValue::text(entryClique->chosenValue(field)));
            else {
                Value combined;
                foreach(Value v, entryClique->chosenValues(field)) {
                    combined.merge(v);
                }
                if (!combined.isEmpty())
                    mergedEntry->insert(field, combined);
            }
        }

        bool actuallyMerged = false;
        foreach(Entry *entry, entryClique->entryList()) {
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
                    if (!mergedEntry->contains(it.key()))
                        mergedEntry->insert(it.key(), it.value());
                file->removeOne(entry);
            }
        }

        if (actuallyMerged)
            file->append(mergedEntry);
        else
            delete mergedEntry;
    }

    return true;
}
