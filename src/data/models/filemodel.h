/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_DATA_FILEMODEL_H
#define KBIBTEX_DATA_FILEMODEL_H

#include <QAbstractItemModel>
#include <QLatin1String>
#include <QList>
#include <QStringList>

#include <NotificationHub>
#include <Element>

#ifdef HAVE_KF5
#include "kbibtexdata_export.h"
#endif // HAVE_KF5

class File;
class Entry;

/**
@author Thomas Fischer
 */
class KBIBTEXDATA_EXPORT FileModel : public QAbstractTableModel, private NotificationListener
{
    Q_OBJECT

public:
    static const int NumberRole;
    static const int SortRole;

    explicit FileModel(QObject *parent = nullptr);

    File *bibliographyFile() const;
    virtual void setBibliographyFile(File *file);

    QModelIndex parent(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clear();
    virtual bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    bool removeRowList(const QList<int> &rows);
    bool insertRow(QSharedPointer<Element> element, int row, const QModelIndex &parent = QModelIndex());

    QSharedPointer<Element> element(int row) const;
    int row(QSharedPointer<Element> element) const;
    /// Notifies the model that a given element has been modifed
    void elementChanged(int row);

    void notificationEvent(int eventId) override;

private:
    File *m_file;
    QMap<QString, QString> colorToLabel;

    void readConfiguration();
    static QString leftSqueezeText(const QString &text, int n);
    QString entryText(const Entry *entry, const QString &raw, const QString &rawAlt, const QStringList &rawAliases, int role, bool followCrossRef) const;
};

#endif // KBIBTEX_DATA_FILEMODEL_H
