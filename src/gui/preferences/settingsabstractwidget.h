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

#ifndef KBIBTEX_GUI_SETTINGSABSTRACTWIDGET_H
#define KBIBTEX_GUI_SETTINGSABSTRACTWIDGET_H

#include <kbibtexgui_export.h>

#include <QWidget>

class KComboBox;

class ItalicTextItemModel : public QAbstractItemModel
{
public:
    ItalicTextItemModel(QObject *parent = NULL);

    void addItem(const QString &a, const QString &b);
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex&) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &) const;
    int columnCount(const QModelIndex &) const;

private:
    QList<QPair<QString, QString> > m_data;
};


/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT SettingsAbstractWidget : public QWidget
{
    Q_OBJECT

public:
    SettingsAbstractWidget(QWidget *parent);

signals:
    void changed();

public slots:
    virtual void loadState() = 0;
    virtual void saveState() = 0;
    virtual void resetToDefaults() = 0;

protected:
    void selectValue(KComboBox *comboBox, const QString &value, int role = Qt::DisplayRole);
};

#endif // KBIBTEX_GUI_SETTINGSABSTRACTWIDGET_H
