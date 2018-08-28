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

#ifndef KBIBTEX_PROC_FINDDUPLICATES_H
#define KBIBTEX_PROC_FINDDUPLICATES_H

#include "kbibtexproc_export.h"

#include <QObject>
#include <QMap>

#include "value.h"

class Entry;
class File;
class FileModel;

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
    QVector<Value> values(const QString &field) const;
    QVector<Value> &values(const QString &field);
    Value chosenValue(const QString &field) const;
    QVector<Value> chosenValues(const QString &field) const;
    void setChosenValue(const QString &field, const Value &value, ValueOperation valueOperation = SetValue);

    QString dump() const;

protected:
    void addEntry(QSharedPointer<Entry> entry);

private:
    QMap<QSharedPointer<Entry>, bool> checkedEntries;
    QMap<QString, QVector<Value> > valueMap;
    QMap<QString, QVector<Value> > chosenValueMap;

    void recalculateValueMap();
    void insertKeyValueToValueMap(const QString &fieldName, const Value &fieldValue, const QString &fieldValueText, const Qt::CaseSensitivity = Qt::CaseSensitive);
};

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROC_EXPORT FindDuplicates : public QObject
{
    Q_OBJECT

public:
    explicit FindDuplicates(QWidget *parent, int sensitivity = 4000);
    ~FindDuplicates() override;

    bool findDuplicateEntries(File *file, QVector<EntryClique *> &entryCliqueList);

signals:
    void maximumProgress(int maxProgress);
    void currentProgress(int progress);

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
    static bool mergeDuplicateEntries(const QVector<EntryClique *> &entryCliques, FileModel *fileModel);

private:
    explicit MergeDuplicates();
};

#endif // KBIBTEX_PROC_FINDDUPLICATES_H
