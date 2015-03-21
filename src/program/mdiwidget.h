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

#ifndef KBIBTEX_PROGRAM_MDIWIDGET_H
#define KBIBTEX_PROGRAM_MDIWIDGET_H

#include <QStackedWidget>
#include <QModelIndex>

#include <QUrl>
#include <KService>

#include "openfileinfo.h"
#include <fileview.h>

namespace KParts
{
class Part;
}

class OpenFileInfo;

class MDIWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit MDIWidget(QWidget *parent);
    ~MDIWidget();

    FileView *fileView();
    OpenFileInfo *currentFile();
    OpenFileInfoManager *getOpenFileInfoManager();

public slots:
    void setFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr = KService::Ptr());

signals:
    void setCaption(const QString &);
    void documentSwitch(FileView *, FileView *);
    void activePartChanged(KParts::Part *);
    void documentNew();
    void documentOpen();
    void documentOpenURL(QUrl);

private:
    class MDIWidgetPrivate;
    MDIWidgetPrivate *d;

private slots:
    void slotCompleted(QObject *);
    void slotStatusFlagsChanged(OpenFileInfo::StatusFlags);
    void slotOpenLRU(const QModelIndex &);
};

#endif // KBIBTEX_PROGRAM_MDIWIDGET_H
