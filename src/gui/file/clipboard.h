/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KBIBTEX_GUI_CLIPBOARD_H
#define KBIBTEX_GUI_CLIPBOARD_H

#include <QObject>

#include "kbibtexgui_export.h"

class QMouseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;

class FileView;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT Clipboard : public QObject
{
    Q_OBJECT

public:
    explicit Clipboard(FileView *fileView);
    ~Clipboard() override;

public slots:
    void cut();
    void copy();
    void copyReferences();
    void paste();

public:
    void editorMouseEvent(QMouseEvent *event);
    void editorDragEnterEvent(QDragEnterEvent *event);
    void editorDragMoveEvent(QDragMoveEvent *event);
    void editorDropEvent(QDropEvent *event);

    static QSet<QUrl> urlsToOpen(const QMimeData *mimeData);

private:
    class ClipboardPrivate;
    Clipboard::ClipboardPrivate *d;
};

#endif // KBIBTEX_GUI_CLIPBOARD_H
