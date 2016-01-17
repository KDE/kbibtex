/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMouseEvent>
#include <QDrag>
#include <QRegExp>

#include "fileview.h"
#include "fileoperation.h"

const QString Clipboard::keyCopyReferenceCommand = QLatin1String("copyReferenceCommand");
const QString Clipboard::defaultCopyReferenceCommand = QLatin1String("");

class Clipboard::ClipboardPrivate
{
private:
    // UNUSED Clipboard *parent;

public:
    FileOperation *fileOpr;
    FileView *fileView;
    QPoint previousPosition;

    ClipboardPrivate(FileView *fv, Clipboard */* UNUSED p*/)
        : /* UNUSED parent(p),*/ fileOpr(new FileOperation(fv)), fileView(fv) {
        // nothing
    }

    void insertText(const QString &text, const QModelIndex &mi) {
        const QRegExp withProtocolChecker("^[a-zA-Z]+:");

        if (withProtocolChecker.indexIn(text) == 0) {
            int entryIndex = -1;
            if (mi.isValid())
                entryIndex = fileView->sortFilterProxyModel()->mapToSource(mi).row();
            fileOpr->insertUrl(text, entryIndex);

        } else {
            fileOpr->insertEntries(text);
        }
    }
};

Clipboard::Clipboard(FileView *fileView)
        : QObject(fileView), d(new ClipboardPrivate(fileView, this))
{
    connect(fileView, SIGNAL(editorMouseEvent(QMouseEvent*)), this, SLOT(editorMouseEvent(QMouseEvent*)));
    connect(fileView, SIGNAL(editorDragEnterEvent(QDragEnterEvent*)), this, SLOT(editorDragEnterEvent(QDragEnterEvent*)));
    connect(fileView, SIGNAL(editorDragMoveEvent(QDragMoveEvent*)), this, SLOT(editorDragMoveEvent(QDragMoveEvent*)));
    connect(fileView, SIGNAL(editorDropEvent(QDropEvent*)), this, SLOT(editorDropEvent(QDropEvent*)));
    fileView->setAcceptDrops(!fileView->isReadOnly());
}

Clipboard::~Clipboard()
{
    delete d;
}

void Clipboard::cut()
{
    copy();
    d->fileView->selectionDelete();
}

void Clipboard::copy()
{
    QString text = d->fileOpr->entryIndexesToText(d->fileOpr->selectedEntryIndexes());
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Clipboard::copyReferences()
{
    QString text = d->fileOpr->entryIndexesToReferences(d->fileOpr->selectedEntryIndexes());
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Clipboard::paste()
{
    QClipboard *clipboard = QApplication::clipboard();
    d->insertText(clipboard->text(), d->fileView->currentIndex());
}


void Clipboard::editorMouseEvent(QMouseEvent *event)
{
    if (!(event->buttons()&Qt::LeftButton))
        return;

    if (d->previousPosition.x() > -1 && (event->pos() - d->previousPosition).manhattanLength() >= QApplication::startDragDistance()) {
        QString text = d->fileOpr->entryIndexesToText(d->fileOpr->selectedEntryIndexes());

        QDrag *drag = new QDrag(d->fileView);
        QMimeData *mimeData = new QMimeData();
        QByteArray data = text.toLatin1();
        mimeData->setData("text/plain", data);
        drag->setMimeData(mimeData);

        drag->exec(Qt::CopyAction);
    }

    d->previousPosition = event->pos();
}

void Clipboard::editorDragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void Clipboard::editorDragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void Clipboard::editorDropEvent(QDropEvent *event)
{
    QString text = event->mimeData()->text();
    if (!text.isEmpty()) {
        /// Locate element drop was performed on (if dropped, and not some copy&paste)
        QModelIndex dropIndex = d->fileView->indexAt(event->pos());
        d->insertText(text, dropIndex);
    }
}
