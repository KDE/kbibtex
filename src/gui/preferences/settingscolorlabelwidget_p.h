/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_P_H
#define KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_P_H

#include <QAbstractItemModel>
#include <QColor>

/**
 * This model maintains a list of label-color pairs.
 * @author Thomas Fischer
 */
class ColorLabelSettingsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ColorLabelSettingsModel(QObject *parent);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void loadState();
    void saveState();
    void resetToDefaults();

    void addColorLabel(const QColor &color, const QString &label);
    void removeColorLabel(int row);

signals:
    void modified();

private:
    QVector<QPair<QColor, QString>> colorLabelPairs;
};

#endif // KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_P_H
