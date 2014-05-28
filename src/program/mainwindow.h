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

#ifndef KBIBTEX_PROGRAM_MAINWINDOW_H
#define KBIBTEX_PROGRAM_MAINWINDOW_H

#include <kparts/mainwindow.h>
#include <KConfigGroup>

#include "openfileinfo.h"

class QTextEdit;
class QDragEnterEvent;
class QDropEvent;
class QCloseEvent;

class ReferencePreview;
class FileView;

class KBibTeXMainWindow : public KParts::MainWindow
{
    Q_OBJECT

public:
    explicit KBibTeXMainWindow();
    virtual ~KBibTeXMainWindow();

public slots:
    void openDocument(const KUrl &url);

protected: // KMainWindow API
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void closeEvent(QCloseEvent *event);

protected:
    void setupControllers();

protected slots:
    void newDocument();
    void openDocumentDialog();
    void closeDocument();
    void showPreferences();
    void documentSwitched(FileView *, FileView *);

private slots:
    void showSearchResults();
    void documentListsChanged(OpenFileInfo::StatusFlags statusFlags);
    void openRecentFile();
    void queryCloseAll();
    void delayed();

private:
    class KBibTeXMainWindowPrivate;
    KBibTeXMainWindowPrivate *d;
};

#endif // KBIBTEX_PROGRAM_MAINWINDOW_H
