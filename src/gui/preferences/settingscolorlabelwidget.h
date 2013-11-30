/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H
#define KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H

#include <QAbstractItemModel>

#include <KSharedConfig>

#include <kbibtexgui_export.h>

#include "settingsabstractwidget.h"
#include "notificationhub.h"

class QSignalMapper;

class KActionMenu;

class BibTeXEditor;

/**
@author Thomas Fischer
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

    bool containsLabel(const QString &label);
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

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT SettingsColorLabelWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsColorLabelWidget(QWidget *parent);
    ~SettingsColorLabelWidget();

    virtual QString label() const;
    virtual KIcon icon() const;

public slots:
    void loadState();
    void saveState();
    void resetToDefaults();

private slots:
    void addColor();
    void removeColor();
    void enableRemoveButton();

private:
    class SettingsColorLabelWidgetPrivate;
    SettingsColorLabelWidgetPrivate *d;
};

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT ColorLabelContextMenu : public QObject, private NotificationListener
{
    Q_OBJECT

public:
    explicit ColorLabelContextMenu(BibTeXEditor *widget);

    KActionMenu *menuAction();
    void setEnabled(bool);

    void notificationEvent(int eventId);

private slots:
    void colorActivated(const QString &colorString);

private:
    BibTeXEditor *m_tv;
    KActionMenu *m_menu;
    QSignalMapper *m_sm;

    void rebuildMenu();
};

#endif // KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H
