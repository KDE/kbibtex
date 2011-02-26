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

#include "file.h"
#include "entry.h"
#include "findduplicates.h"

#define getText(entry, fieldname) PlainTextValue::text((entry)->value((fieldname)))

class FindDuplicates::FindDuplicatesPrivate
{
private:
    FindDuplicates *p;
    const int maxDistance;

public:
    int sensitivity;

    FindDuplicatesPrivate(FindDuplicates *parent, int sens)
            : p(parent), maxDistance(10000), sensitivity(sens) {
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
        const QRegExp nonWordRegExp("[^a-zA-Z']+");
        if (s.isNull() || t.isNull()) return 1.0;
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


FindDuplicates::FindDuplicates(int sensitivity)
        : d(new FindDuplicatesPrivate(this, sensitivity))
{
    // nothing
}

QList<QList<Entry*> > FindDuplicates::findDuplicateEntries(File *file)
{
    QList<QList<Entry*> > result;

    /// assemble list of entries only (ignoring comments, macros, ...)
    QList<Entry*> listOfEntries;
    for (File::ConstIterator it = file->constBegin(); it != file->constEnd(); ++it) {
        Entry *e = dynamic_cast<Entry*>(*it);
        if (e != NULL && !e->isEmpty())
            listOfEntries << e;
    }

    if (listOfEntries.isEmpty()) {
        /// no entries to compare found
        return result;
    }

    /// go through all entries ...
    for (QList<Entry*>::ConstIterator eit = listOfEntries.constBegin(); eit != listOfEntries.constEnd(); ++eit) {
        /// ... and find a "clique" of entries where it will match, i.e. distance is below sensitivity

        /// assume current entry will match in no clique
        bool foundClique = false;

        /// go through all existing cliques
        for (QList<QList<Entry*> >::Iterator cit = result.begin(); cit != result.end(); ++cit)
            /// check distance between current entry and clique's first entry
            if (d->entryDistance(*eit, (*cit).first()) < d->sensitivity) {
                /// if distance is below sensitivity, add current entry to clique
                foundClique = true;
                (*cit) << *eit;
                break;
            }

        if (!foundClique) {
            /// no clique matched to current entry, so create and add new clique
            /// consisting only of the current entry
            QList<Entry*> newClique = QList<Entry*>() << *eit;
            result << newClique;
        }
    }

    /// remove cliques with only one element (nothing to merge here) from the list of cliques
    for (QList<QList<Entry*> >::Iterator cit = result.begin(); cit != result.end();)
        if ((*cit).count() < 2)
            cit = result.erase(cit);
        else
            ++cit;

    for (QList<QList<Entry*> >::Iterator cit = result.begin(); cit != result.end(); ++cit) {
        kDebug() << "BEGIN clique " << (*cit).count();

        for (QList<Entry*>::ConstIterator eit = (*cit).constBegin(); eit != (*cit).constEnd(); ++eit) {
            kDebug() << "  " << (*eit)->id();
        }

        kDebug() << "END clique";
    }

    return result;
}
