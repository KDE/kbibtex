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
#include <bibtexfields.h>

#include "bibtexfileview.h"

using namespace KBibTeX::GUI::Widgets;

BibTeXFileView::BibTeXFileView(QWidget * parent)
        : QTreeView(parent)
{
// TODO
}

BibTeXFileView::~BibTeXFileView()
{
// TODO
}

void BibTeXFileView::resizeEvent(QResizeEvent *event)
{
    KBibTeX::GUI::Config::BibTeXFields *bibtexFields = KBibTeX::GUI::Config::BibTeXFields::self();
    int sum = 0, widgetWidth = size().width() - 64;
    for (KBibTeX::GUI::Config::BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it)
        sum += (*it).width;
    int col = 0;
    for (KBibTeX::GUI::Config::BibTeXFields::Iterator it = bibtexFields->begin(); it != bibtexFields->end(); ++it, ++col)
        setColumnWidth(col, (*it).width*widgetWidth / sum);
}
