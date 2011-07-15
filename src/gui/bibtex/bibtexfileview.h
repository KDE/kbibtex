/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#ifndef KBIBTEX_GUI_BIBTEXFILEVIEW_H
#define KBIBTEX_GUI_BIBTEXFILEVIEW_H

#include <QTreeView>

#include <kbibtexgui_export.h>

class QSignalMapper;

class BibTeXFileModel;
class QSortFilterProxyModel;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXFileView : public QTreeView
{
    Q_OBJECT
public:
    BibTeXFileView(const QString &name, QWidget *parent = 0);

    virtual void setModel(QAbstractItemModel * model);
    BibTeXFileModel *bibTeXModel();
    QSortFilterProxyModel *sortFilterProxyModel();

protected:
    const QString m_name;

    void resizeEvent(QResizeEvent *event);

protected slots:
    void columnResized(int column, int oldSize, int newSize);

private:
    QSignalMapper *m_signalMapperBibTeXFields;
    BibTeXFileModel *m_bibTeXFileModel;
    QSortFilterProxyModel *m_sortFilterProxyModel;

    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configHeaderState;

    void syncBibTeXFields();

private slots:
    void columnsChanged();
    void headerActionToggled(QObject *action);
    void headerResetToDefaults();
    void sort(int, Qt::SortOrder);
};


#endif // KBIBTEX_GUI_BIBTEXFILEVIEW_H
