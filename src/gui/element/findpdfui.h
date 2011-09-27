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

#ifndef KBIBTEX_GUI_FINDPDFUI_H
#define KBIBTEX_GUI_FINDPDFUI_H

#include <kbibtexgui_export.h>

#include <QWidget>

#include <entry.h>
#include <networking/findpdf.h>

class QListView;

class FindPDF;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FindPDFUI : public QWidget
{
    Q_OBJECT

public:
    FindPDFUI(Entry &entry, QWidget *parent);

    static void interactiveFindPDF(Entry &entry, QWidget *parent);

signals:
    void resultAvailable(bool);

protected:
    void apply();

private:
    QListView *m_listViewResult;
    FindPDF *m_findpdf;

    void createGUI();

private slots:
    void searchFinished();
};


class PDFListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    PDFListModel(const QList<FindPDF::ResultItem> &resultList, QObject *parent = NULL);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    const QList<FindPDF::ResultItem> m_resultList;
};


#endif // KBIBTEX_GUI_FINDPDFUI_H
