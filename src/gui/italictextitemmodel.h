/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2017 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_ITALICTEXTITEMMODEL_H
#define KBIBTEX_GUI_ITALICTEXTITEMMODEL_H

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

    explicit ItalicTextItemModel(QObject *parent = nullptr);
    ~ItalicTextItemModel() override;

    /**
     * Add a new entry (pair of shown text and identifier) to this model.
     * @param shownText i18n'ized text to be shown to the user
     * @param identifier internal identifier (if empty, shownText will be set in italics)
     */
    void addItem(const QString &shownText, const QString &identifier);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &) const override;
    QModelIndex parent(const QModelIndex &) const override;
    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_GUI_ITALICTEXTITEMMODEL_H
