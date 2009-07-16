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

#ifndef KBIBTEX_PROGRAM_MAINWINDOW_H
#define KBIBTEX_PROGRAM_MAINWINDOW_H

#include <kparts/mainwindow.h>
#include <KConfigGroup>


#include "documentlist.h"

class QDockWidget;
//class DocumentList;

namespace KBibTeX
{
namespace Program {

class KBibTeXProgram;

class KBibTeXMainWindow : public KParts::MainWindow
{
    Q_OBJECT

public:
    explicit KBibTeXMainWindow(KBibTeXProgram *program);
    virtual ~KBibTeXMainWindow();

public slots:
    void openDocument(const KUrl& url, const QString& encoding);

protected: // KMainWindow API
    virtual void saveProperties(KConfigGroup &configGroup);
    virtual void readProperties(const KConfigGroup &configGroup);

protected:
    void setupControllers();

protected:
    KBibTeXProgram *m_program;
    QDockWidget *m_dockDocumentList;
    DocumentList *m_listDocumentList;
    KParts::ReadWritePart *m_part;

protected slots:
    void openDocumentDialog();
};

}
}

#endif // KBIBTEX_PROGRAM_MAINWINDOW_H
