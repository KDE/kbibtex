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

#ifndef KBIBTEX_GUI_FINDPDFUI_H
#define KBIBTEX_GUI_FINDPDFUI_H

#include "kbibtexgui_export.h"

#include <QWidget>
#include <QLabel>

#include <KWidgetItemDelegate>

#include "entry.h"
#include "findpdf.h"

class QListView;

class FindPDF;


class PDFItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

private:
    QListView *m_parent;

public:
    PDFItemDelegate(QListView *itemView, QObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    /// get the list of widgets
    virtual QList<QWidget *> createItemWidgets() const;

    /// update the widgets
    virtual void updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const;

private slots:
    void slotViewPDF();
    void slotRadioNoDownloadToggled(bool);
    void slotRadioDownloadToggled(bool);
    void slotRadioURLonlyToggled(bool);
};

/**
@author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT FindPDFUI : public QWidget
{
    Q_OBJECT

public:
    FindPDFUI(Entry &entry, QWidget *parent);
    ~FindPDFUI();

    static void interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent);
    void apply(Entry &entry, const File &bibtexFile);

signals:
    void resultAvailable(bool);

private:
    QListView *m_listViewResult;
    QLabel *m_labelMessage;
    QList<FindPDF::ResultItem> m_resultList;
    FindPDF *m_findpdf;

    void createGUI();

private slots:
    void searchFinished();
    void searchProgress(int visitedPages, int runningJobs, int foundDocuments);
};


class PDFListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PDFListModel(QList<FindPDF::ResultItem> &resultList, QObject *parent = NULL);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    QList<FindPDF::ResultItem> &m_resultList;
};


#endif // KBIBTEX_GUI_FINDPDFUI_H
