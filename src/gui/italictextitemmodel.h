/*****************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#ifndef GUI_ITALICTEXTITEMMODEL_H
#define GUI_ITALICTEXTITEMMODEL_H

#include <QAbstractItemModel>

#include "kbibtexgui_export.h"

/**
 * The ItalicTextItemModel allows to maintain a list of value pairs:
 * The first element of the pair will be the shown text (should be
 * i18n'ized), the second value is supposed to be a string to identify
 * the shown text. If this identifier is empty or null, the first pair
 * element will be drawn in italic text.
 *
 * @author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT ItalicTextItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ItalicTextItemModelRole {
        /// Role to retrieve identifier for a row
        IdentifierRole = Qt::UserRole + 9672
    };

    explicit ItalicTextItemModel(QObject *parent = NULL);
    ~ItalicTextItemModel();

    /**
     * Add a new entry (pair of shown text and identifier) to this model.
     * @param shownText i18n'ized text to be shown to the user
     * @param identifier internal identifier (if empty, shownText will be set in italics)
     */
    void addItem(const QString &shownText, const QString &identifier);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &) const;
    int columnCount(const QModelIndex &) const;

private:
    class Private;
    Private *const d;
};

#endif // GUI_ITALICTEXTITEMMODEL_H
