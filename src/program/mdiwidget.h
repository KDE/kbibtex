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

#ifndef KBIBTEX_PROGRAM_MDIWIDGET_H
#define KBIBTEX_PROGRAM_MDIWIDGET_H

#include <QStackedWidget>

#include <KUrl>

#include <bibtexeditor.h>

namespace KBibTeX
{
namespace Program {

class MDIWidget : public QStackedWidget
{
    Q_OBJECT

public:
    MDIWidget(QWidget *parent);

    bool setUrl(const KUrl &url, const QString &encoding);
    bool closeUrl(const KUrl &url);

    KUrl currentUrl() const;
    KBibTeX::GUI::BibTeXEditor *editor();

signals:
    void documentSwitch(KBibTeX::GUI::BibTeXEditor *, KBibTeX::GUI::BibTeXEditor *);

private:
    class MDIWidgetPrivate;
    MDIWidgetPrivate *d;
};
}
}

#endif // KBIBTEX_PROGRAM_MDIWIDGET_H
