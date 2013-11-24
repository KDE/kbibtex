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

#ifndef KBIBTEX_GUI_CLIPBOARD_H
#define KBIBTEX_GUI_CLIPBOARD_H

#include "kbibtexgui_export.h"

#include <QObject>

class QMouseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class BibTeXEditor;

class KBIBTEXGUI_EXPORT Clipboard : public QObject
{
    Q_OBJECT

public:
    static const QString keyCopyReferenceCommand;
    static const QString defaultCopyReferenceCommand;

    explicit Clipboard(BibTeXEditor *bibTeXEditor);
    ~Clipboard();

public slots:
    void cut();
    void copy();
    void copyReferences();
    void paste();

private slots:
    void editorMouseEvent(QMouseEvent *event);
    void editorDragEnterEvent(QDragEnterEvent *event);
    void editorDragMoveEvent(QDragMoveEvent *event);
    void editorDropEvent(QDropEvent *event);

private:
    class ClipboardPrivate;
    Clipboard::ClipboardPrivate *d;
};

#endif // KBIBTEX_GUI_CLIPBOARD_H
