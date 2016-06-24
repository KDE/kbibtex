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
#ifndef KBIBTEX_GUI_FILEMODEL_H
#define KBIBTEX_GUI_FILEMODEL_H

#include <QAbstractItemModel>
#include <QLatin1String>
#include <QList>
#include <QStringList>

#include <KSharedConfig>

#include "kbibtexdata_export.h"

#include "notificationhub.h"
#include "file.h"
#include "entry.h"

class FileModel;

/**
@author Thomas Fischer
 */
class KBIBTEXDATA_EXPORT FileModel : public QAbstractTableModel, private NotificationListener
{
    Q_OBJECT

public:
    static const int NumberRole;
    static const int SortRole;
    static const QString keyShowComments;
    static const bool defaultShowComments;
    static const QString keyShowMacros;
    static const bool defaultShowMacros;

    explicit FileModel(QObject *parent = 0);

    File *bibliographyFile() const;
    virtual void setBibliographyFile(File *file);

    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    void clear();
    virtual bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    bool removeRowList(const QList<int> &rows);
    bool insertRow(QSharedPointer<Element> element, int row, const QModelIndex &parent = QModelIndex());

QSharedPointer<Element> element(int row) const;
    int row(QSharedPointer<Element> element) const;
    /// Notifies the model that a given element has been modifed
    void elementChanged(int row);

    void notificationEvent(int eventId);

private:
    File *m_file;
    QMap<QString, QString> colorToLabel;

    void readConfiguration();

    QVariant entryData(const Entry *entry, const QString &raw, const QString &rawAlt, int role, bool followCrossRef) const;
};

#endif // KBIBTEX_GUI_FILEMODEL_H
