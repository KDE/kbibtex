/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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
#include <QModelIndex>

#include <KUrl>
#include <KService>

#include "openfileinfo.h"
#include <bibtexeditor.h>

namespace KParts
{
class Part;
}

class OpenFileInfo;

class MDIWidget : public QStackedWidget
{
    Q_OBJECT

public:
    MDIWidget(QWidget *parent);
    ~MDIWidget();

    BibTeXEditor *editor();
    OpenFileInfo *currentFile();
    OpenFileInfoManager *getOpenFileInfoManager();

public slots:
    void setFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr = KService::Ptr());

signals:
    void setCaption(const QString &);
    void documentSwitch(BibTeXEditor *, BibTeXEditor *);
    void activePartChanged(KParts::Part *);
    void documentNew();
    void documentOpen();
    void documentOpenURL(KUrl);

private:
    class MDIWidgetPrivate;
    MDIWidgetPrivate *d;

private slots:
    void slotCompleted(QObject *);
    void slotStatusFlagsChanged(OpenFileInfo::StatusFlags);
    void slotOpenLRU(const QModelIndex &);
};

#endif // KBIBTEX_PROGRAM_MDIWIDGET_H
