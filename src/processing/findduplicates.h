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

#ifndef KBIBTEX_PROC_FINDDUPLICATES_H
#define KBIBTEX_PROC_FINDDUPLICATES_H

#include "kbibtexproc_export.h"

#include <QObject>
#include <QMap>

#include "value.h"

class Entry;
class File;

class KBIBTEXPROC_EXPORT FindDuplicates;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROC_EXPORT EntryClique
{
    friend class FindDuplicates;
public:
    EntryClique();

    enum ValueOperation { SetValue, AddValue, RemoveValue };

    int entryCount() const;
    QList<QSharedPointer<Entry> > entryList() const;
    bool isEntryChecked(QSharedPointer<Entry> entry) const;
    void setEntryChecked(QSharedPointer<Entry> entry, bool isChecked);

    int fieldCount() const;
    QList<QString> fieldList() const;
    QList<Value> values(const QString &field) const;
    Value chosenValue(const QString &field) const;
    QList<Value> chosenValues(const QString &field) const;
    void setChosenValue(const QString &field, Value &value, ValueOperation valueOperation = SetValue);

    QString dump() const;

protected:
    void addEntry(QSharedPointer<Entry> entry);

private:
    QMap<QSharedPointer<Entry>, bool> checkedEntries;
    QMap<QString, QList<Value> > valueMap;
    QMap<QString, QList<Value> > chosenValueMap;

    void recalculateValueMap();
    void insertKeyValueToValueMap(const QString &fieldName, const Value &fieldValue, const QString &fieldValueText);
};

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROC_EXPORT FindDuplicates : public QObject
{
    Q_OBJECT

public:
    FindDuplicates(QWidget *parent, int sensitivity = 4000);
    ~FindDuplicates();

    bool findDuplicateEntries(File *file, QList<EntryClique*> &entryCliqueList);

signals:
    void maximumProgress(int maxProgress);
    void currentProgress(int progress);

private slots:
    void gotCanceled();

private:
    class FindDuplicatesPrivate;
    FindDuplicatesPrivate *d;
};

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROC_EXPORT MergeDuplicates
{
public:
    enum MergePriority { None = 0, ISODateEarlier = 1, ISODateLater = 2, NumericHigher = 3, NumericLower = 4, LexicallyEarlier = 5, LexicallyLater = 6, LexicallyEarlierCaseInsensitive = 7, LexicallyLaterCaseInsensitive = 8 };

    MergeDuplicates(QWidget *parent);

    bool mergeDuplicateEntries(const QList<EntryClique*> &entryCliques, File *file);

    /**
     * Goes through the list of Entry cliques and automatically merge
     * those cliques, i.e. remove all Entries in a clique and replace
     * it by a single new Entry. This new Entry will contain values
     * from the clique, but values will be selected from Entries as
     * chosen by the sorting criterium field and the merge priority.
     */
    bool mergeDuplicateEntriesAuto(const QList<EntryClique*> &entryCliques, File *file, const QString &sortCriteriumField, MergePriority mergePriority);

private:
    class MergeDuplicatesPrivate;
    MergeDuplicatesPrivate *d;
};

#endif // KBIBTEX_PROC_FINDDUPLICATES_H
