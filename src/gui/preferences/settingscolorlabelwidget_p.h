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

#ifndef KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_P_H
#define KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_P_H

#include <QAbstractItemModel>
#include <QColor>

#include <KSharedConfig>

/**
 * This model maintains a list of label-color pairs.
 * @author Thomas Fischer
 */
class ColorLabelSettingsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ColorLabelSettingsModel(QObject *parent);

    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void loadState();
    void saveState();
    void resetToDefaults();

    void addColorLabel(const QColor &color, const QString &label);
    void removeColorLabel(int row);

signals:
    void modified();

private:
    struct ColorLabelPair {
        QColor color;
        QString label;
    };

    QList<ColorLabelPair> colorLabelPairs;
    KSharedConfigPtr config;

};

#endif // KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_P_H
