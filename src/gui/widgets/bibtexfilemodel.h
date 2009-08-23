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
#ifndef KBIBTEX_GUI_BIBTEXFILEMODEL_H
#define KBIBTEX_GUI_BIBTEXFILEMODEL_H

#include <QAbstractItemModel>
#include <QLatin1String>
#include <QList>
#include <QRegExp>

#include <kbibtexgui_export.h>

#include <file.h>
#include <bibtexfields.h>

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXFileModel : public QAbstractItemModel
{
public:
    BibTeXFileModel(QObject * parent = 0);
    virtual ~BibTeXFileModel();

    void setBibTeXFile(KBibTeX::IO::File *bibtexFile);

    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex & index) const;
    virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    KBibTeX::IO::File *m_bibtexFile;
    KBibTeX::GUI::Config::BibTeXFields *m_bibtexFields;

    static const QRegExp whiteSpace;
};

}
}
}

#endif // KBIBTEX_GUI_BIBTEXFILEMODEL_H
